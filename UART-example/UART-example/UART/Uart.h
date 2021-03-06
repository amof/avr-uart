/************************************************************************
Title:    Interrupt UART library with receive/transmit circular buffers
Author:   Julien Delvaux
Software: Atmel Studio 7
Hardware: AVR 8-Bits, tested with ATmega1284P and ATmega88PA-PU
License:  GNU General Public License 3
Usage:    see Doxygen manual

Based on original library by Peter Fleury, Tim Sharpe, Nicholas Zambetti and Andy Gock.



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

LICENSE:
    Copyright (C) 2006 Peter Fleury

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
************************************************************************/

/** 
 *  @defgroup avr-uart UART Library
 *  @code #include <uart.h> @endcode
 * 
 *  @brief Interrupt UART library using the built-in UART with transmit and receive circular buffers. 
 *
 *  This library can be used to transmit and receive data through the built in UART. 
 *
 *  An interrupt is generated when the UART has finished transmitting or
 *  receiving a byte. The interrupt handling routines use circular buffers
 *  for buffering received and transmitted data.
 *
 *  The UART_RXn_BUFFER_SIZE and UART_TXn_BUFFER_SIZE constants define
 *  the size of the circular buffers in bytes. Note that these constants must be a power of 2.
 *
 *  You need to define these buffer sizes in uart.h
 *
 *  !! The maximum baudrate @8Mhz (internal clock) is 57600bps.
 *
 *  @note Based on Atmel Application Note AVR306
 *  @author Julien Delvaux <delvaux.ju@gmail.com>
 *  @note Based on original library by Peter Fleury, Tim Sharpe and Andy Gock.
 */


#ifndef UART_H_
#define UART_H_

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#if (__GNUC__ * 100 + __GNUC_MINOR__) < 304
	#error "This library requires AVR-GCC 3.4 or later, update to newer AVR-GCC compiler !"
#endif

/************************************************************************/
/* Constants and macros                                                 */
/************************************************************************/

/* Enable USART 1 if required */
#define USART0_ENABLED
//#define USART1_ENABLED

/* Set size of receive and transmit buffers */

#ifndef UART_RX0_BUFFER_SIZE
	#define UART_RX0_BUFFER_SIZE 64 /**< Size of the circular receive buffer, must be power of 2 */
#endif
#ifndef UART_RX1_BUFFER_SIZE
	#define UART_RX1_BUFFER_SIZE 64 /**< Size of the circular receive buffer, must be power of 2 */
#endif

#ifndef UART_TX0_BUFFER_SIZE
	#define UART_TX0_BUFFER_SIZE 64 /**< Size of the circular transmit buffer, must be power of 2 */
#endif
#ifndef UART_TX1_BUFFER_SIZE
	#define UART_TX1_BUFFER_SIZE 64 /**< Size of the circular transmit buffer, must be power of 2 */
#endif

/** @brief  UART Baudrate Expression
 *  @param  xtalCpu  system clock in Mhz, e.g. 4000000L for 4Mhz          
 *  @param  baudRate baudrate in bps, e.g. 1200, 2400, 9600     
 */
#define UART_BAUD_SELECT(baudRate,xtalCpu)  (xtalCpu / (8UL * baudRate)) -1UL

/* Error code */
#define UART_FRAME_ERROR      0x0800              /**< Framing Error by UART       */
#define UART_OVERRUN_ERROR    0x0400              /**< Overrun condition by UART   */
#define UART_BUFFER_OVERFLOW  0x0200              /**< receive ringbuffer overflow */
#define UART_NO_DATA          0x0100              /**< no receive data available   */

/* Macros, to allow use of legacy names */

#define uart_init(b)      uart0_init(b)
#define uart_close()      uart0_close()
#define uart_getc()       uart0_getc()
#define uart_putc(d)      uart0_putc(d)
#define uart_puts(s)      uart0_puts(s)
#define uart_puts_p(s)    uart0_puts_p(s)
#define uart_available()  uart0_available()
#define uart_flush()      uart0_flush()

/************************************************************************/
/* Functions prototype                                                  */
/************************************************************************/

/**
   @brief   Initialize UART and set baudrate 
   @param   baudrate Specify baudrate using macro UART_BAUD_SELECT()
   @return  none
*/
extern void uart0_init(uint32_t baudrate);

/**
   @brief   Close UART, flush and clear any received datas
   @param   none
   @return  none
*/
extern void uart0_close(void);


/**
 *  @brief   Get received byte from ringbuffer
 *
 * Returns in the lower byte the received character and in the 
 * higher byte the last receive error.
 * UART_NO_DATA is returned when no data is available.
 *
 *  @return  lower byte:  received byte from ringbuffer
 *  @return  higher byte: last receive status
 *           - \b 0 successfully received data from UART
 *           - \b UART_NO_DATA           
 *             <br>no receive data available
 *           - \b UART_BUFFER_OVERFLOW   
 *             <br>Receive ringbuffer overflow.
 *             We are not reading the receive buffer fast enough, 
 *             one or more received character have been dropped 
 *           - \b UART_OVERRUN_ERROR     
 *             <br>Overrun condition by UART.
 *             A character already present in the UART UDR register was 
 *             not read by the interrupt handler before the next character arrived,
 *             one or more received characters have been dropped.
 *           - \b UART_FRAME_ERROR       
 *             <br>Framing Error by UART
 */
extern uint16_t uart0_getc(void);

/**
 *  @brief   Peek at next byte in ringbuffer
 *
 * Returns the next byte (character) of incoming UART data without removing it from the
 * internal ring buffer. That is, successive calls to uartN_peek() will return the same
 * character, as will the next call to uartN_getc().
 *
 * UART_NO_DATA is returned when no data is available.
 *
 *  @return  lower byte:  next byte in ringbuffer
 *  @return  higher byte: last receive status
 *           - \b 0 successfully received data from UART
 *           - \b UART_NO_DATA           
 *             <br>no receive data available
 *           - \b UART_BUFFER_OVERFLOW   
 *             <br>Receive ringbuffer overflow.
 *             We are not reading the receive buffer fast enough, 
 *             one or more received character have been dropped 
 *           - \b UART_OVERRUN_ERROR     
 *             <br>Overrun condition by UART.
 *             A character already present in the UART UDR register was 
 *             not read by the interrupt handler before the next character arrived,
 *             one or more received characters have been dropped.
 *           - \b UART_FRAME_ERROR       
 *             <br>Framing Error by UART
 */
extern uint16_t uart0_peek(void);

/**
 *  @brief   Put byte to ringbuffer for transmitting via UART
 *  @param   data byte to be transmitted
 *  @return   byte: status of insertion
 *           - \b 0 successfully received data from UART
 *           - \b UART_BUFFER_OVERFLOW
 *             <br>Sending ringbuffer overflow.
 */
extern uint8_t uart0_putc(uint8_t data);


/**
 *  @brief   Put string to ringbuffer for transmitting via UART
 *
 *  The string is buffered by the uart library in a circular buffer
 *  and one character at a time is transmitted to the UART using interrupts.
 *  Blocks if it can not write the whole string into the circular buffer.
 * 
 *  @param   s string to be transmitted
 *  @return  none
 */
extern void uart0_puts(const char *s );


/**
 * @brief    Put string from program memory to ringbuffer for transmitting via UART.
 *
 * The string is buffered by the uart library in a circular buffer
 * and one character at a time is transmitted to the UART using interrupts.
 * Blocks if it can not write the whole string into the circular buffer.
 *
 * @param    s program memory string to be transmitted
 * @return   none
 * @see      uart0_puts_P
 */
extern void uart0_puts_p(const char *s );

/**
 * @brief    Macro to automatically put a string constant into program memory
 * @param    __s string in program memory
 */
#define uart_puts_P(__s)       uart0_puts_p(PSTR(__s))
#define uart0_puts_P(__s)      uart0_puts_p(PSTR(__s))

/**
 *  @brief   Return number of bytes waiting in the receive buffer
 *  @return  bytes waiting in the receive buffer
 */
extern uint16_t uart0_available(void);

/**
 *  @brief   Flush bytes waiting in receive buffer
 */
extern void uart0_flush(void);


/** @brief  Initialize USART1 (only available on selected ATmegas) @see uart_init */
extern void uart1_init(uint32_t baudrate);
/** @brief  Close USART1 (only available on selected ATmegas) @see uart_close */
extern void uart1_close(void);
/** @brief  Get received byte of USART1 from ringbuffer. (only available on selected ATmega) @see uart_getc */
extern uint16_t uart1_getc(void);
/** @brief  Peek at next byte in USART1 ringbuffer */
extern uint16_t uart1_peek(void);
/** @brief  Put byte to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_putc */
extern void uart1_putc(uint8_t data);
/** @brief  Put string to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts */
extern void uart1_puts(const char *s );
/** @brief  Put string from program memory to ringbuffer for transmitting via USART1 (only available on selected ATmega) @see uart_puts_p */
extern void uart1_puts_p(const char *s );
/** @brief  Macro to automatically put a string constant into program memory */
#define uart1_puts_P(__s)       uart1_puts_p(PSTR(__s))
/** @brief   Return number of bytes waiting in the receive buffer */
extern uint16_t uart1_available(void);
/** @brief   Flush bytes waiting in receive buffer */
extern void uart1_flush(void);

#endif /* UART_H_ */