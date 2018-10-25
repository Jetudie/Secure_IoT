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
#define MAXTIMINGS	85

#include<stdlib.h>
#include<math.h>
long int p,q,n,t,flag,e[100],d[100],temp[100],j,m[100],en[100],i;
char msg[100] = "Michael";
int prime(long int);
void ce();
long int cd(long int);
void encrypt();
void decrypt();

int prime(long int pr) {
	int i;
	j=sqrt(pr);
	for (i=2;i<=j;i++) {
		if(pr%i==0)
		    return 0;
	}
	return 1;
}

void ce() {
	int k;
	k=0;
	for (i=2;i<t;i++) {
		if(t%i==0)
		    continue;

		flag=prime(i);

		if(flag==1&&i!=p&&i!=q) {
			e[k]=i;
			flag=cd(e[k]);
			if(flag>0) {
				d[k]=flag;
				k++;
			}
			if(k==99)
			        break;
		}

	}

}

long int cd(long int x) {
	long int k=1;
	while(1) {
		k=k+t;
		if(k%x==0)
		    return(k/x);
	}
}

void encrypt() {
	long int pt,ct,key=e[0],k,len;
	i=0;
	len=strlen(msg);
	while(i!=len) {
		pt=m[i];
		pt=pt-96;
		k=1;
		for (j=0;j<key;j++) {
			k=k*pt;
			k=k%n;
		}
		temp[i]=k;
		ct=k+96;
		en[i]=ct;
		i++;
	}
	en[i]=-1;

	printf("THE ENCRYPTED MESSAGE IS\n");
	for (i=0;en[i]!=-1;i++)
		printf("%c",en[i]);
	printf("\n");
}

void decrypt() {
	long int pt,ct,key=d[0],k;
	i=0;
	while(en[i]!=-1) {
		ct=temp[i];
		k=1;
		for (j=0;j<key;j++) {
			k=k*ct;
			k=k%n;
		}
		pt=k+96;
		m[i]=pt;
		i++;
	}
	m[i]=-1;
	printf("THE DECRYPTED MESSAGE IS\n");
	for (i=0;m[i]!=-1;i++)
		printf("%c",m[i]);
	printf("\n");
}


int dht11_dat[5] = { 0, 0, 0, 0, 0 };
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

CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};

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
		/*
		dht11_dat[2]*=11;
		dht11_dat[0]*=13;
		printf("%d%d", dht11_dat[2], dht11_dat[0]);
		*/
	}
	//else  {
	//	printf( "Data not good, skip\n" );
	//}
}

/* Global functions ----------------------------------------------------------------------------------------*/
/*********************************************************************************************************//**
  * @brief  Main program.
  * @retval None
  * @details Main program as following
  *  - Enable peripheral clock of AFIO, USART0.
  *  - Config AFIO mode as USART0_Rx and USART0_Tx function.
  *  - USART0 configuration:
  *     - BaudRate = 115200 baud
  *     - Word Length = 8 Bits
  *     - One Stop Bit
  *     - None parity bit
  *  - USART0 Tx "Hello World!" string 10 times.
  *  - USART0 Rx character and transform to hex.(Loop)
  *
  ***********************************************************************************************************/
int main(void)
{
  USART_InitTypeDef USART_InitStructure;
  int input = 0;
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
  USART_TxCmd(COM1_PORT, ENABLE);
  USART_RxCmd(COM1_PORT, ENABLE);

	HTCFG_OUTPUT_LED2_CLK(CKCUClock) = 1;
	CKCU_PeripClockConfig(CKCUClock, ENABLE);
	GPIO_DirectionConfig(HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, GPIO_DIR_OUT);
	GPIO_WriteOutBits( HTCFG_LED2, HTCFG_OUTPUT_LED2_GPIO_PIN, RESET );
  /* USART0 Rx character and transform to hex.(Loop)                                                        */
  while (1)
  {
		//delayms(100);
		read_dht11_dat();
		//delayms(2);
		/*printf("ENTER FIRST PRIME NUMBER\n");
		p = 7;
		printf("ENTER ANOTHER PRIME NUMBER\n");
		q = 11;
		for (i=0; msg[i] != '\0'; i++)
			m[i]=msg[i];

		n=p*q;
		t=(p-1)*(q-1);
		ce();
		encrypt();
		decrypt();*/
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