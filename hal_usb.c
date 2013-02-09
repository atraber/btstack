/** 
 * @file  hal_usb.c
 * 
 * Copyright 2008 Texas Instruments, Inc.
***************************************************************************/
#include  <msp430x54x.h>
#include "hal_usb.h"


#ifdef USE_IRQ_RX
static char halUsbReceiveBuffer[255];
static unsigned char bufferSize=0;
#endif

/**********************************************************************//**
 * @brief  Initializes the serial communications peripheral and GPIO ports 
 *         to communicate with the TUSB3410.
 * 
 * @param  none
 * 
 * @return none
 **************************************************************************/
void halUsbInit(void)
{

#ifdef USE_IRQ_RX
	volatile unsigned char i;
	for (i = 0;i < 255; i++){
	halUsbReceiveBuffer[i]='\0';
	}
	bufferSize = 0;
#endif

	USB_PORT_SEL |= USB_PIN_RXD + USB_PIN_TXD;
	USB_PORT_DIR |= USB_PIN_TXD;
	USB_PORT_DIR &= ~USB_PIN_RXD;

	UCA0CTL1 = UCSWRST;                          //Reset State
	UCA0CTL0 = UCMODE_0;

	// set 115200 baud
	UCA0CTL1 |= UCSSEL_2;
	UCA0BR0 = 138 % 256;                             // 8Mhz/57600=138
	UCA0BR1 = 138 / 256;

	UCA0ABCTL = 0;

	UCA0MCTL = (7 << 1);



	UCA0CTL1 &= ~UCSWRST;
	// UCA0IE |= UCRXIE;						//enable receive interrupts
	// __bis_SR_register(GIE);                   // Enable Interrupts
}

/**********************************************************************//**
 * @brief  Disables the serial communications peripheral and clears the GPIO
 *         settings used to communicate with the TUSB3410.
 * 
 * @param  none
 * 
 * @return none
 **************************************************************************/
void halUsbShutDown(void)
{
  UCA0IE &= ~UCRXIE;
  UCA0CTL1 = UCSWRST;                          //Reset State
  USB_PORT_SEL &= ~( USB_PIN_RXD + USB_PIN_TXD );
  USB_PORT_DIR |= USB_PIN_TXD;
  USB_PORT_DIR |= USB_PIN_RXD;
  USB_PORT_OUT &= ~(USB_PIN_TXD + USB_PIN_RXD);
}

/**********************************************************************//**
 * @brief  Sends a character over UART to the TUSB3410.
 * 
 * @param  character The character to be sent. 
 * 
 * @return none
 **************************************************************************/
void halUsbSendChar(char character)
{
  while (!(UCA0IFG & UCTXIFG));
  UCA0TXBUF = character;
}

char halUsbRecvChar(){
    while (!(UCA0IFG & UCRXIFG));
    return UCA0RXBUF;
}

/**********************************************************************//**
 * @brief  Sends a string of characters to the TUSB3410
 * 
 * @param  string[] The array of characters to be transmit to the TUSB3410.
 * 
 * @param  length   The length of the string.
 * 
 * @return none
 **************************************************************************/
void halUsbSendString(char string[], unsigned char length)
{
  volatile unsigned char i;
  for (i=0; i < length; i++)
    halUsbSendChar(string[i]);  
}

#ifdef USE_IRQ_RX
/************************************************************************/
interrupt(USCI_A1_VECTOR) USCI_A1_ISR (void)
{
  halUsbReceiveBuffer[bufferSize++] = UCA0RXBUF;
   __bic_SR_register_on_exit(LPM3_bits);   
}
#endif

// provide putchar used by printf
/*
// ATRABER: DO NOT USE as it collides with standard library of MSP430 compiler
int putchar(int c){
    halUsbSendChar(c);
    return 1;
}*/

#include <stdio.h>
#include <string.h>

int fputc(int _c, register FILE *_fp)
{
	if(_c == '\n')
		halUsbSendChar('\r');
	halUsbSendChar(_c);

  return((unsigned char)_c);
}

int fputs(const char *_ptr, register FILE *_fp)
{
  unsigned int i, len;

  len = strlen(_ptr);

  halUsbSendString(_ptr, len);

  return len;
}
