/************************************************************************
Title:    Example UART
Brief:	  This example shows how to send back datas received from UART RX port
Author:   Julien Delvaux
Software: Atmel Studio 7
Hardware: AVR 8-Bits, tested with ATmega1284P and ATmega88PA-PU
License:  GNU General Public License 3


LICENSE:
	Copyright (C) 2015 Julien Delvaux

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
    
************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "UART/Uart.h"

int main(void)
{
    uart_init(UART_BAUD_SELECT(57600, 8000000L));
    sei();

    while(1){;
		if(uart_available()>0){
			for(uint8_t i=0;i<uart_available();i++){
				uart_putc(uart_getc());
			}
		}
	}
}
