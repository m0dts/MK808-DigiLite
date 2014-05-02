#include <htc.h>
#include "sci.h"

/* Routines for initialisation and use of the SCI
 * for the PIC processor.
 */

/* other options:
 * frame errors
 */


unsigned char
sci_Init(unsigned long int baud, unsigned char ninebits)
{
	int X;
	unsigned long tmp;
	
	/* calculate and set baud rate register */
	/* for asynchronous mode */
	tmp = 16UL * baud;
	X = (int)(FOSC/tmp) - 1;
	if((X>255) || (X<0))
	{
		tmp = 64UL * baud;
		X = (int)(FOSC/tmp) - 1;
		if((X>255) || (X<0))
		{
			return 1;	/* panic - baud rate unobtainable */
		}
		else
			BRGH = 0;	/* low baud rate */
	}
	else
		BRGH = 1;	/* high baud rate */
	SPBRG = X;	/* set the baud rate */

	SYNC = 0;	/* asynchronous */
	SPEN = 1;	/* enable serial port pins */
	CREN = 1;	/* enable reception */
	SREN = 0;	/* no effect */
	TXIE = 0;	/* disable tx interrupts */
	RCIE = 0;	/* disable rx interrupts */
	TX9  = ninebits?1:0;	/* 8- or 9-bit transmission */
	RX9  = ninebits?1:0;	/* 8- or 9-bit reception */
	TXEN = 1;	/* enable the transmitter */

	return 0;
}

void
sci_PutByte(unsigned char byte)
{
	while(!TXIF)	/* set when register is empty */
		continue;
	TXREG = byte;

	return;
}

unsigned char sci_GetByte(void)
{
	char timer = 0;
	while(timer<76){	//while register is empty try to read char for 1 second, if nothing then return 1
		if(T0IF){
			timer++;
			T0IF = 0;
		}
		if(RCIF){
			return RCREG;	
		}
	}
return 1;

}


unsigned char
sci_CheckOERR(void)
{
	char a;
	if(OERR)	/* re-enable after overrun error */
	{
		
		CREN = 0;
		a=RCREG;
		a=RCREG;
		a=RCREG;	
		CREN = 1;
		return 1;
	}
	
	return 0;
}




unsigned char
sci_GetFERR(void)
{
	char timer = 0;
	while(timer<76){	/* while register is empty */
		if(T0IF){
			timer++;
			T0IF = 0;
		}
			if(RCIF){
		return FERR;	/* RXD9 and FERR are gone now */
		}
	}
return 0;	/* RCIF is not cleared until RCREG is read */
}

