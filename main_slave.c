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

int main(void)
{
	int sync = 0;
	int Key;
	int OK = 0;
	DHT_Data Data = {0, 0};
	Slave *slave;

	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	USART_InitTypeDef USART_InitStructure;
	init_slave(&slave);

	/* Enable peripheral clock of AFIO, USART0                                                                */
	CKCUClock.Bit.AFIO   = 1;
	COM1_CLK(CKCUClock)  = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);

	/* Config AFIO mode as USART0_Rx and USART0_Tx function.                                                  */
	AFIO_GPxConfig(COM1_TX_GPIO_ID, COM1_TX_AFIO_PIN, AFIO_FUN_USART_UART);
	AFIO_GPxConfig(COM1_RX_GPIO_ID, COM1_RX_AFIO_PIN, AFIO_FUN_USART_UART);

	/* USART0 configuration ----------------------------------------------------------------------------------*/
	/* USART0 configured as follow:
			- BaudRate = 115200 baud
			- Word Length = 8 Bits
			- One Stop Bit
			- None parity bit
	*/
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
	USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
	USART_InitStructure.USART_Parity = USART_PARITY_NO;
	USART_InitStructure.USART_Mode = USART_MODE_NORMAL;
	USART_Init(COM1_PORT, &USART_InitStructure);

	/* Set COM1_PORT interrupt-flag                                                                        */
	USART_IntConfig(COM1_PORT, USART_INT_RXDR, ENABLE);
	
	USART_TxCmd(COM1_PORT, ENABLE);
	USART_RxCmd(COM1_PORT, ENABLE);

	// Set GPIOpin 
	// LED1 for DHT11
	// LED2 for Fan Control
	HTCFG_OUTPUT_LED1_CLK(CKCUClock) = 1;
	HTCFG_OUTPUT_LED2_CLK(CKCUClock) = 1;
	GPIO_DirectionConfig(HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, GPIO_DIR_OUT);
	GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, RESET );

	// Rx example code
	/* COM1 Tx                                                                                                 */
	//URTxWriteIndex = sizeof(HelloString);
	//memcpy(URTxBuf, HelloString, sizeof(HelloString));
	//USART_IntConfig(COM1_PORT, USART_INT_TXDE, ENABLE);
	/* URx Tx > URx Rx
	COM1 Rx > COM1 Rx interrupt mode                                                                          */
	//while (1)
	//{
		/* COM1 Rx.waiting for receive the fifth data,
		then move date from UR1RxBuf to UR1TxBuf.                                                               */
		//if (URRxWriteIndex >= 5)
		//{
		//memcpy(URTxBuf, URRxBuf, 5);
		//URRxWriteIndex = 0;
		/* COM1 Tx                                                                                             */
		//URTxWriteIndex = 5;
		//USART_IntConfig(COM1_PORT, USART_INT_TXDE, ENABLE);
		//}
	//}

	/* USART0 Rx character and transform to hex.(Loop)                                                        */
	while (1)
	{
		if(!OK){ // if not synchronized
			SendRequest();
			sync = GetSync(slave);
			if(sync){
				SendOK();
				CreateKey(&Key, &slave->x1s);
				OK = 1;
			}
		}
		else{ // if synchronized
			Data = Decrypt(Key, GetCipherText());
			ControlFan(Data);
			//OutputLCD(Data);
		}
	}
}

void SendRequest()
{
	printf("Req:01");
}

int GetSync(Slave* slave)
{
	int n[3];
	scanf("%d%d%d", &n[0], &n[1], &n[2]);
	slave->sync(slave, n);
	if(slave->e1 < 0.00002)
		return 1;
	else
		return 0;
}

void SendOK()
{
	printf("Sync01");
}

void CreateKey(void* key, void* n)
{
	memcpy(key, n, 4);
}

DHT_Data GetCipherText()
{
	DHT_Data data;
	scanf("%d%d", &data.Temp, &data.Hum);
	return data;
}
DHT_Data Decrypt(int key ,DHT_Data data)
{
	int mask = 0x07f80000;
	data.Temp ^= key & mask >> 16;
	data.Hum ^= key & mask >> 16;
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
