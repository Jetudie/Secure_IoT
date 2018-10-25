/*********************************************************************************************************//**
 * @file    USART/HyperTerminal_TxRx/main.c
 * @version $Rev:: 686          $
 * @date    $Date:: 2016-05-26 #$
 * @brief   Main program.
 *************************************************************************************************************
 * @attention
 *
 * Firmware Disclaimer Information
 *
 * 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
 *    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
 *    other intellectual property laws.
 *
 * 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
 *    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
 *    other than HOLTEK and the customer.
 *
 * 3. The program technical documentation, including the code, is provided "as is" and for customer reference
 *    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
 *    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
 *    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
 *
 * <h2><center>Copyright (C) Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/

/* Includes ------------------------------------------------------------------------------------------------*/
#include "ht32.h"
#include "ht32_board.h"
#include "ht32_board_config.h"
#include "ht32f5xxxx_gpio.h"
#include <string.h>
#include <stdio.h> 
#include "usart_int.h" // for Rx interrupt
#define MAXTIMINGS	85

int dht11_dat[5] = { 0, 0, 0, 0, 0 };
CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};

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

void read_dht11_dat()
{
	uint8_t laststate	= SET;
	uint8_t counter	= 0;
	uint8_t j = 0, i ;
	float	f; /* fahrenheit */
 
	dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
	HTCFG_OUTPUT_LED1_CLK(CKCUClock) = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);
	
	/* pull pin down for 18 milliseconds */
	//GPIO_WriteOutBits( HTCFG_LED1, HTCFG_OUTPUT_LED1_GPIO_PIN, SET );
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
	//printf( "BIT = %d\n", j );
	
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
		
		if(dht11_dat[0] < 10)
			printf("%2d0%d", dht11_dat[2], dht11_dat[0]);
		else
			printf("%2d%2d", dht11_dat[2], dht11_dat[0]);
		
		if(dht11_dat[0]>=50){
			GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, SET );
		}else{
			GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, RESET );
		}
	}
}

int main(void)
{
	USART_InitTypeDef USART_InitStructure;

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

	/* Seting COM1_PORT interrupt-flag                                                                        */
	USART_IntConfig(COM1_PORT, USART_INT_RXDR, ENABLE);
	
	USART_TxCmd(COM1_PORT, ENABLE);
	USART_RxCmd(COM1_PORT, ENABLE);

	// set GPIOpin:LED2
	HTCFG_OUTPUT_LED2_CLK(CKCUClock) = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);
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
			read_dht11_dat();
	}
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