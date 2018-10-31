/* Includes ------------------------------------------------------------------------------------------------*/
#include "ht32.h"
#include "ht32_board.h"
#include "ht32_board_config.h"
#include "ht32f5xxxx_gpio.h"
#include <string.h>
#include <stdio.h> 
#include "usart_int.h" // for Rx interrupt
#include "lorenz.h"
#define MAXTIMINGS	85
#define SYNC_DONE 0
#define ONE_MORE 1
#define SKIP_REQUEST 2

typedef struct DHT_Data{
	int Temp;
	int Hum;
}DHT_Data;

void SendRequest(void);
int GetSync(Slave*);
void SendOK(void);
void CreateKey(void*, void*);
DHT_Data GetCipherText(void);
DHT_Data Decrypt(int ,DHT_Data);
//void OutputLCD(DHT_Data);
void ControlFan(DHT_Data);
void delayms(int);
void delayus(int);
void SendData(DHT_Data*);

int main(void)
{
	int Sync = ONE_MORE;
	int Key;
	DHT_Data Data = {0, 0};
	Slave *slave;

	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	USART_InitTypeDef USART_InitStructure;
	init_slave(&slave);
	HT32F_DVB_LEDInit(HT_LED1);
	HT32F_DVB_LEDInit(HT_LED2);
	/* Enable peripheral clock of AFIO, USART0                                                                */
	CKCUClock.Bit.AFIO   = 1;
	COM1_CLK(CKCUClock)  = 1;
	COM2_CLK(CKCUClock)  = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);

	/* Config AFIO mode as USART0_Rx and USART0_Tx function.                                                  */
	AFIO_GPxConfig(COM1_TX_GPIO_ID, COM1_TX_AFIO_PIN, AFIO_FUN_USART_UART);
	AFIO_GPxConfig(COM1_RX_GPIO_ID, COM1_RX_AFIO_PIN, AFIO_FUN_USART_UART);
	AFIO_GPxConfig(COM2_TX_GPIO_ID, COM2_TX_AFIO_PIN, AFIO_FUN_USART_UART);
	AFIO_GPxConfig(COM2_RX_GPIO_ID, COM2_RX_AFIO_PIN, AFIO_FUN_USART_UART);

	/* USART configuration ----------------------------------------------------------------------------------*/
	/* USART configured as follow:
			- BaudRate = 115200 baud
			- Word Length = 8 Bits
			- One Stop Bit
			- None parity bit
	*/
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
	USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
	USART_InitStructure.USART_Parity = USART_PARITY_NO;
	USART_InitStructure.USART_Mode = USART_MODE_NORMAL;
	USART_Init(COM1_PORT, &USART_InitStructure);
	USART_Init(COM2_PORT, &USART_InitStructure);
	
	/* Set COM1_PORT, COM2_PORT interrupt-flag                                                                        */
	// COM1: USART1, A4, A5 for Master
	// COM2: USART0, A2, A3 for Testing
	USART_IntConfig(COM1_PORT, USART_INT_RXDR, ENABLE);
	USART_IntConfig(COM2_PORT, USART_INT_RXDR, ENABLE);
	
	USART_TxCmd(COM1_PORT, ENABLE);
	USART_RxCmd(COM1_PORT, ENABLE);
	USART_TxCmd(COM2_PORT, ENABLE);
	USART_RxCmd(COM2_PORT, ENABLE);
	/* Configure USART0 & USART1 interrupt                                                                    */
	NVIC_EnableIRQ(COM1_IRQn);
	NVIC_EnableIRQ(COM2_IRQn);
	
	// Set GPIOpin 
	// LED1 for DHT11
	// LED2 for Fan Control
	HTCFG_OUTPUT_LED1_CLK(CKCUClock) = 1;
	HTCFG_OUTPUT_LED2_CLK(CKCUClock) = 1;
	GPIO_DirectionConfig(HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, GPIO_DIR_OUT);
	GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, RESET );

	/* USART0 Rx character and transform to hex.(Loop)                                                        */
	while (1)
	{
		if(Sync != SYNC_DONE){ // if not synchronized
			if(Sync == ONE_MORE)
				SendRequest();
			Sync = GetSync(slave);
		}
		else{ // if synchronized
			SendOK();
			CreateKey(&Key, &slave->x1s);
			Data = Decrypt(Key, GetCipherText());
			ControlFan(Data);
			SendData(&Data);
			//OutputLCD(Data);
		}
	}
}

void SendRequest()
{
	int msg = 0x12345678;
	memcpy(URTxBuf, &msg, 4);
	URTxWriteIndex = 4;
	USART_IntConfig(COM1_PORT, USART_INT_TXDE, ENABLE);
	delayms(1000);
}

int GetSync(Slave* slave)
{
	int n[2];

	// Get um and x2m from UART buffer
	if(URRxWriteIndex >= 8){
		memcpy(n, URRxBuf, 8);
		URRxWriteIndex -= 8;
		slave->sync(slave, n);
		// Return 1 if OK(converge)
		// else return 2 (asking for one more)
		if(slave->e2 > -0.000002 && slave->e2 < 0.000002 )
			return SYNC_DONE;
		else
			return ONE_MORE;
	}
	return SKIP_REQUEST;
}

void SendOK()
{
	int msg = 0x9ABCDEF0;

	USART_IntConfig(COM1_PORT, USART_INT_TXDE, DISABLE);
	memcpy(URTxBuf, &msg, 4);
	URTxWriteIndex = 4;
	USART_IntConfig(COM1_PORT, USART_INT_TXDE, ENABLE);
}

void CreateKey(void* key, void* n)
{
	memcpy(key, n, 4);
}

DHT_Data GetCipherText()
{
	DHT_Data data;
	if(URRxWriteIndex >= 8){
		memcpy(&data.Temp, URRxBuf, 4);
		memcpy(&data.Hum, URRxBuf+4, 4);
		URRxWriteIndex -= 8;	
	}
	return data;
}
DHT_Data Decrypt(int key ,DHT_Data data)
{
	int mask = 0x07f80000;
	data.Temp ^= (key & mask) >> 16;
	data.Hum ^= (key & mask) >> 16;
	return data;
}
void OutputLCD(DHT_Data);

/* Use LED2 GPIO to control fan */
void ControlFan(DHT_Data Data)
{
	if(Data.Hum >= 50)
		GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, SET );
	else
		GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, RESET );
}

void delayms(int n_ms)
{
	int i = 0, j = 0;
	for(i= 0; i < n_ms;i++)
		for(j= 0; j<13000;j++)
			;
}

void delayus(int n_us)
{
	int i = 0, j = 0;
	for(i= 0; i < n_us;i++)
		for(j= 0; j<13;j++)
			;
}

void SendData(DHT_Data* data){
	USART_IntConfig(COM2_PORT, USART_INT_TXDE, DISABLE);
	memcpy(URTxBuf2, &data->Temp, 4);
	memcpy(URTxBuf2+4, &data->Hum, 4);
	URTxWriteIndex2 = 8;
	USART_IntConfig(COM2_PORT, USART_INT_TXDE, ENABLE);
}

#if (HT32_LIB_DEBUG == 1)
/*********************************************************************************************************//**
  * @brief  Report both the error name of the source file and the source line number.
  * @param  filename: pointer to the source file name.
  * @param  uline: error line source number.
  * @retval None
  ***********************************************************************************************************/
void assert_error(u8* filename, u32 uline)
{
  /*
     This function is called by IP library that the invalid parameters has been passed to the library API.
     Debug message can be added here.
     Example: printf("Parameter Error: file %s on line %d\r\n", filename, uline);
  */

  while (1)
  {
  }
}
#endif
