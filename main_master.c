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

bool GetRequest();
void SendSync(Master*);
bool CheckOK();
int Encrypt(Master*, Temp, Hum);
void SendCiphertext();
void GetDHT11Data(int*, int*);
void delayms(int);
void delayus(int);

struct DHT_Data{
	int Temp;
	int Hum;
};

int main(void)
{
	int Req;
	int Key;
	struct DHT_Data Data = {0, 0};
	Master *master;

	CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
	USART_InitTypeDef USART_InitStructure;

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
		Req = GetRequest();
		if(Req == 1)
			SendSync(master);
		else if(Req == 2)
			Key = master->x1m;

		if(CheckOK()){
			GetDHT11Data(&Data);
			SendCiphertext(Encrypt(Key, &Data));
		}
	}
}

bool GetRequest(){
	// Use UART to check request
	char str[7];

	// if gets request, return 1
	if(URRxWriteIndex >= 6){
		memcpy(str, URRxBuf, 6);
		str[6] = '\0';
		if(strcmp(str, "Req:01") != NULL)
			return 1;
		if(strcmp(str, "Sync01") != NULL)
			return 2;
		return 0;
	}
	return 0;
}

void SendSync(Master* master){
	char x[12];
	USART_IntConfig(COM1_PORT, USART_INT_TXDE, DISABLE);
	memcpy(x, master->x1m, sizeof(x1));
	memcpy(x+4, master->x2m, sizeof(x2));
	memcpy(x+8, master->x3m, sizeof(x3));
	memcpy(URTxBuf, x, 12);
	USART_IntConfig(COM1_PORT, USART_INT_TXDE, ENABLE);
}

bool CheckOK();
void Encrypt(Master*);
void SendCiphertext();

// Source: http://www.uugear.com/portfolio/dht11-humidity-temperature-sensor-module/?fbclid=IwAR01i0nRi2Ima3vOjKyExAJkNNBw7shnxLS7Aq6wSucu_ExnubCZfM0ZNv4
void GetDHT11Data(int *Temp, int *Hum)
{
	uint8_t laststate = SET;
	uint8_t counter	= 0;
	uint8_t j = 0, i ;
	float f; /* fahrenheit */
	int dht11_dat[5] = { 0, 0, 0, 0, 0 };
	
	/* pull pin down for 18 milliseconds */
	GPIO_DirectionConfig(HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, GPIO_DIR_OUT);
	GPIO_WriteOutBits( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, RESET );
	delayms( 18 );
	/* then pull it up for 40 microseconds */
	GPIO_WriteOutBits( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, SET );
	delayus( 40 ); 
	/* prepare to read the pin */
	GPIO_DirectionConfig(HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, GPIO_DIR_IN);
	GPIO_ReadInBit( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN );
	GPIO_InputConfig( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, ENABLE);
		
	/* detect change and read data */
	for ( i = 0; i < MAXTIMINGS; i++ )
	{
		counter = 0;
		while ( GPIO_ReadInBit( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN ) == laststate )
		{
			counter++;
			delayus( 1 );
			if ( counter == 255 )
			{
				break;
			}
		}
		laststate = GPIO_ReadInBit( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN );
		if ( counter == 255 )
			break;
		/* ignore first 3 transitions */
		if ( (i >= 4) && (i % 2 == 0) )  
		{
			/* shove each bit into the storage bytes */
			dht11_dat[j / 8] <<= 1;
			if ( counter > 16 )   //16
				dht11_dat[j / 8] |= 1;
			j++;
		}
	}

	/*
	 * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
	if ((j >= 40)&&(dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
	{
		*Temp = dht11[2];
		*Hum = dht11[0];
	}
}

void delayms(int n_ms){
	int i = 0, j = 0;
	for(i= 0; i < n_ms;i++)
		for(j= 0; j<13000;j++)
			;
}

void delayus(int n_us){
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