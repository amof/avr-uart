/*************************************************************************
	
	Updated UART library by Julien Delvaux
	
	Based on updated UART library (this one) by Andy Gock
	https://github.com/andygock/avr-uart

	Based on updated UART library (this one) by Tim Sharpe
	http://beaststwo.org/avr-uart/index.shtml

	Based on original library by Peter Fluery
	http://homepage.hispeed.ch/peterfleury/avr-software.html

*************************************************************************/

/************************************************************************
Changelog for modifications made by Julien Delvaux, starting with the current
  library version by Andy Gock as of 17/11/2014.

Date        Description
=========================================================================
2015-12-29
	- Reduced compatibility to Atmega88PA family and Atmega1284P family
	

************************************************************************/

#include "Uart.h"


/************************************************************************/
/* Constants and macros                                                 */
/************************************************************************/

/* size of RX/TX buffers */
#define UART_RX0_BUFFER_MASK ( UART_RX0_BUFFER_SIZE - 1)
#define UART_RX1_BUFFER_MASK ( UART_RX1_BUFFER_SIZE - 1)

#define UART_TX0_BUFFER_MASK ( UART_TX0_BUFFER_SIZE - 1)
#define UART_TX1_BUFFER_MASK ( UART_TX1_BUFFER_SIZE - 1)

#if ( UART_RX0_BUFFER_SIZE & UART_RX0_BUFFER_MASK )
	#error RX0 buffer size is not a power of 2
#endif
#if ( UART_TX0_BUFFER_SIZE & UART_TX0_BUFFER_MASK )
	#error TX0 buffer size is not a power of 2
#endif

#if ( UART_RX1_BUFFER_SIZE & UART_RX1_BUFFER_MASK )
	#error RX1 buffer size is not a power of 2
#endif
#if ( UART_TX1_BUFFER_SIZE & UART_TX1_BUFFER_MASK )
	#error TX1 buffer size is not a power of 2
#endif

#if	defined(__AVR_ATmega48A__) ||defined(__AVR_ATmega48PA__) || defined(__AVR_ATmega88A__) || \
	defined(__AVR_ATmega88PA__) ||defined(__AVR_ATmega168A__) || defined(__AVR_ATmega168PA__) || \
	defined(__AVR_ATmega328P__)
	/* ATmega with one USART */
	#define ATMEGA_USART0
	#define UART0_RECEIVE_INTERRUPT		USART_RX_vect
	#define UART0_TRANSMIT_INTERRUPT	USART_UDRE_vect
	#define UART0_STATUS				UCSR0A
	#define UART0_CONTROL				UCSR0B
	#define UART0_DATA					UDR0
	#define UART0_UDRIE					UDRIE0
#elif defined(__AVR_ATmega164P__) || defined(__AVR_ATmega324P__) || defined(__AVR_ATmega644P__) || \
	  defined(__AVR_ATmega1284P__)
	/* ATmega with two USART */
	#define ATMEGA_USART0
	#define ATMEGA_USART1
	#define UART0_RECEIVE_INTERRUPT		USART0_RX_vect
	#define UART1_RECEIVE_INTERRUPT		USART1_RX_vect
	#define UART0_TRANSMIT_INTERRUPT	USART0_UDRE_vect
	#define UART1_TRANSMIT_INTERRUPT	USART1_UDRE_vect
	#define UART0_STATUS				UCSR0A
	#define UART0_CONTROL				UCSR0B
	#define UART0_DATA					UDR0
	#define UART0_UDRIE					UDRIE0
	#define UART1_STATUS				UCSR1A
	#define UART1_CONTROL				UCSR1B
	#define UART1_DATA					UDR1
	#define UART1_UDRIE					UDRIE1
#else
	#error "no UART definition for MCU available"
#endif

/************************************************************************/
/* Global variable                                                      */
/************************************************************************/

#if defined( USART0_ENABLED )
	#if defined( ATMEGA_USART0 )
		static volatile uint8_t UART_TxBuf[UART_TX0_BUFFER_SIZE];
		static volatile uint8_t UART_RxBuf[UART_RX0_BUFFER_SIZE];
		static volatile uint8_t UART_TxHead;
		static volatile uint8_t UART_TxTail;
		static volatile uint8_t UART_RxHead;
		static volatile uint8_t UART_RxTail;
		static volatile uint8_t UART_LastRxError;
	#endif
#endif

#if defined( USART1_ENABLED )
	#if defined( ATMEGA_USART1 )
		static volatile uint8_t UART1_TxBuf[UART_TX1_BUFFER_SIZE];
		static volatile uint8_t UART1_RxBuf[UART_RX1_BUFFER_SIZE];
		static volatile uint8_t UART1_TxHead;
		static volatile uint8_t UART1_TxTail;
		static volatile uint8_t UART1_RxHead;
		static volatile uint8_t UART1_RxTail;
		static volatile uint8_t UART1_LastRxError;
	#endif
#endif

ISR(UART0_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART Receive Complete interrupt
Purpose:  called when the UART has received a character
**************************************************************************/
{
	uint16_t tmphead;
	uint8_t data;
	uint8_t usr;
	uint8_t lastRxError;
	
	// read UART data register
	usr  = UART0_STATUS;
    data = UART0_DATA;
	
	/* error */
	#if defined( ATMEGA_USART0 )
		lastRxError = (usr & ((1<<(FE0))|(1<<(DOR0))));
	#endif
	/* calculate buffer index */
	tmphead = ( UART_RxHead + 1) & UART_RX0_BUFFER_MASK;
	
	if ( tmphead == UART_RxTail ) {
		/* error: receive buffer overflow */
		lastRxError = UART_BUFFER_OVERFLOW >> 8;
		} else {
		/* store new index */
		UART_RxHead = tmphead;
		/* store received data in buffer */
		UART_RxBuf[tmphead] = data;
	}
	UART_LastRxError = lastRxError;  
}

#if defined(ATMEGA_USART0)

ISR(UART0_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART Data Register Empty interrupt
Purpose:  called when the UART is ready to transmit the next byte
**************************************************************************/
{
	uint16_t tmptail;

	if ( UART_TxHead != UART_TxTail) {
		/* calculate and store new buffer index */
		tmptail = (UART_TxTail + 1) & UART_TX0_BUFFER_MASK;
		UART_TxTail = tmptail;
		/* get one byte from buffer and write it to UART */
		UART0_DATA = UART_TxBuf[tmptail];  /* start transmission */
		} else {
		/* tx buffer empty, disable UDRE interrupt */
		UART0_CONTROL &= ~(1 << UART0_UDRIE);
	}
}


/*************************************************************************
Function: uart0_init()
Purpose:  initialize UART and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart0_init(uint32_t baudrate){
	#if defined( ATMEGA_USART0 )
	
	UART_TxHead = 0;
	UART_TxTail = 0;
	UART_RxHead = 0;
	UART_RxTail = 0;

	UART0_STATUS = 1<<U2X0 ;  // Asynchronous Transmission -> Must be set to 0 if used synchronous
	
	//Set baud rate
	UBRR0 = baudrate ;
	
	//Configuration
	UART0_CONTROL = 1<<RXEN0 | 1<<TXEN0 | 1<<RXCIE0 ;
	UCSR0C = 3<<UCSZ00 ;
	
	#endif
	
}

/*************************************************************************
Function: uart0_getc()
Purpose:  return byte from ringbuffer
Returns:  byte:  received byte from ringbuffer
**************************************************************************/
uint16_t uart0_getc(void)
{
	uint16_t tmptail;
	uint8_t data;

	if ( UART_RxHead == UART_RxTail ) {
		return UART_NO_DATA;   /* no data available */
	}

	/* calculate /store buffer index */
	tmptail = (UART_RxTail + 1) & UART_RX0_BUFFER_MASK;
	UART_RxTail = tmptail;

	/* get data from receive buffer */
	data = UART_RxBuf[tmptail];

	return (UART_LastRxError << 8) + data;
}

/*************************************************************************
Function: uart0_peek()
Purpose:  Returns the next byte (character) of incoming UART data without
          removing it from the ring buffer. That is, successive calls to
		  uartN_peek() will return the same character, as will the next
		  call to uartN_getc()
Returns:  byte:  next byte in ring buffer
**************************************************************************/
uint16_t uart0_peek(void)
{
	uint16_t tmptail;
	uint8_t data;

	if ( UART_RxHead == UART_RxTail ) {
		return UART_NO_DATA;   /* no data available */
	}

	tmptail = (UART_RxTail + 1) & UART_RX0_BUFFER_MASK;

	/* get data from receive buffer */
	data = UART_RxBuf[tmptail];

	return (UART_LastRxError << 8) + data;

}

/*************************************************************************
Function: uart0_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  byte:  status of the insertion
**************************************************************************/
uint8_t uart0_putc(uint8_t data)
{
	uint8_t error = 0;
	uint16_t tmphead;

	tmphead  = (UART_TxHead + 1) & UART_TX0_BUFFER_MASK;

	if (tmphead != UART_TxTail){
		//there is room in buffer
		UART_TxBuf[tmphead] = data;
		UART_TxHead = tmphead;
	}
	else{
		//no room in buffer
		error = UART_BUFFER_OVERFLOW;
	}

	/* enable UDRE interrupt */
	UART0_CONTROL |= (1<<UART0_UDRIE);
	
	return error;
}

/*************************************************************************
Function: uart0_puts()
Purpose:  transmit string to UART
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void uart0_puts(const char *s )
{
	while (*s) {
		uart0_putc(*s++);
	}

}

/*************************************************************************
Function: uart0_puts_p()
Purpose:  transmit string from program memory to UART
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart0_puts_p(const char *progmem_s )
{
	register char c;

	while ( (c = pgm_read_byte(progmem_s++)) ) {
		uart0_putc(c);
	}

}

/*************************************************************************
Function: uart0_available()
Purpose:  Determine the number of bytes waiting in the receive buffer
Input:    None
Returns:  Integer number of bytes in the receive buffer
**************************************************************************/
uint16_t uart0_available(void)
{
	return (UART_RX0_BUFFER_SIZE + UART_RxHead - UART_RxTail) & UART_RX0_BUFFER_MASK;
}

/*************************************************************************
Function: uart0_flush()
Purpose:  Flush bytes waiting the receive buffer.  Acutally ignores them.
Input:    None
Returns:  None
**************************************************************************/
void uart0_flush(void)
{
	UART_RxHead = UART_RxTail;
}

#endif

#if defined( USART1_ENABLED )

/*
 * these functions are only for ATmegas with two USART
 */
#if defined( ATMEGA_USART1 )

ISR(UART1_RECEIVE_INTERRUPT)
/*************************************************************************
Function: UART1 Receive Complete interrupt
Purpose:  called when the UART1 has received a character
**************************************************************************/
{
	uint16_t tmphead;
	uint8_t data;
	uint8_t usr;
	uint8_t lastRxError;

	/* read UART status register and UART data register */
	usr  = UART1_STATUS;
	data = UART1_DATA;
	
	/* error */
	lastRxError = (usr & ((1<<FE1)|(1<<DOR1)) );
	
	/* calculate buffer index */
	tmphead = ( UART1_RxHead + 1) & UART_RX1_BUFFER_MASK;

	if ( tmphead == UART1_RxTail ) {
		/* error: receive buffer overflow */
		lastRxError = UART_BUFFER_OVERFLOW >> 8;
	} else {
		/* store new index */
		UART1_RxHead = tmphead;
		/* store received data in buffer */
		UART1_RxBuf[tmphead] = data;
	}
	UART1_LastRxError = lastRxError;

}


ISR(UART1_TRANSMIT_INTERRUPT)
/*************************************************************************
Function: UART1 Data Register Empty interrupt
Purpose:  called when the UART1 is ready to transmit the next byte
**************************************************************************/
{
	uint16_t tmptail;

	if ( UART1_TxHead != UART1_TxTail) {
		/* calculate and store new buffer index */
		tmptail = (UART1_TxTail + 1) & UART_TX1_BUFFER_MASK;
		UART1_TxTail = tmptail;
		/* get one byte from buffer and write it to UART */
		UART1_DATA = UART1_TxBuf[tmptail];  /* start transmission */
	} else {
		/* tx buffer empty, disable UDRE interrupt */
		UART1_CONTROL &= ~(1 << UART1_UDRIE);
	}
}


/*************************************************************************
Function: uart1_init()
Purpose:  initialize UART1 and set baudrate
Input:    baudrate using macro UART_BAUD_SELECT()
Returns:  none
**************************************************************************/
void uart1_init(uint32_t baudrate)
{
	UART1_TxHead = 0;
	UART1_TxTail = 0;
	UART1_RxHead = 0;
	UART1_RxTail = 0;

	UART1_STATUS = 1<<U2X1 ;  // Asynchronous Transmission -> Must be set to 0 if used synchronous
		
	//Set baud rate
	UBRR1 = baudrate ;
		
	//Configuration
	UART1_CONTROL = 1<<RXEN1 | 1<<TXEN1 | 1<<RXCIE1 ;
	UCSR1C = 3<<UCSZ10 ;
		
} /* uart_init */


/*************************************************************************
Function: uart1_getc()
Purpose:  return byte from ringbuffer
Returns:  lower byte:  received byte from ringbuffer
          higher byte: last receive error
**************************************************************************/
uint16_t uart1_getc(void)
{
	uint16_t tmptail;
	uint8_t data;

	if ( UART1_RxHead == UART1_RxTail ) {
		return UART_NO_DATA;   /* no data available */
	}

	/* calculate /store buffer index */
	tmptail = (UART1_RxTail + 1) & UART_RX1_BUFFER_MASK;
	UART1_RxTail = tmptail;

	/* get data from receive buffer */
	data = UART1_RxBuf[tmptail];

	return (UART1_LastRxError << 8) + data;

} /* uart1_getc */

/*************************************************************************
Function: uart1_peek()
Purpose:  Returns the next byte (character) of incoming UART data without
          removing it from the ring buffer. That is, successive calls to
		  uartN_peek() will return the same character, as will the next
		  call to uartN_getc()
Returns:  lower byte:  next byte in ring buffer
          higher byte: last receive error
**************************************************************************/
uint16_t uart1_peek(void)
{
	uint16_t tmptail;
	uint8_t data;

	if ( UART1_RxHead == UART1_RxTail ) {
		return UART_NO_DATA;   /* no data available */
	}

	tmptail = (UART1_RxTail + 1) & UART_RX1_BUFFER_MASK;

	/* get data from receive buffer */
	data = UART1_RxBuf[tmptail];

	return (UART1_LastRxError << 8) + data;

} /* uart1_peek */

/*************************************************************************
Function: uart1_putc()
Purpose:  write byte to ringbuffer for transmitting via UART
Input:    byte to be transmitted
Returns:  none
**************************************************************************/
void uart1_putc(uint8_t data)
{
	uint16_t tmphead;

	tmphead  = (UART1_TxHead + 1) & UART_TX1_BUFFER_MASK;

	while ( tmphead == UART1_TxTail ) {
		;/* wait for free space in buffer */
	}

	UART1_TxBuf[tmphead] = data;
	UART1_TxHead = tmphead;

	/* enable UDRE interrupt */
	UART1_CONTROL    |= (1<<UART1_UDRIE);

} /* uart1_putc */


/*************************************************************************
Function: uart1_puts()
Purpose:  transmit string to UART1
Input:    string to be transmitted
Returns:  none
**************************************************************************/
void uart1_puts(const char *s )
{
	while (*s) {
		uart1_putc(*s++);
	}

} /* uart1_puts */


/*************************************************************************
Function: uart1_puts_p()
Purpose:  transmit string from program memory to UART1
Input:    program memory string to be transmitted
Returns:  none
**************************************************************************/
void uart1_puts_p(const char *progmem_s )
{
	register char c;

	while ( (c = pgm_read_byte(progmem_s++)) ) {
		uart1_putc(c);
	}

} /* uart1_puts_p */



/*************************************************************************
Function: uart1_available()
Purpose:  Determine the number of bytes waiting in the receive buffer
Input:    None
Returns:  Integer number of bytes in the receive buffer
**************************************************************************/
uint16_t uart1_available(void)
{
	return (UART_RX1_BUFFER_SIZE + UART1_RxHead - UART1_RxTail) & UART_RX1_BUFFER_MASK;
} /* uart1_available */



/*************************************************************************
Function: uart1_flush()
Purpose:  Flush bytes waiting the receive buffer.  Acutally ignores them.
Input:    None
Returns:  None
**************************************************************************/
void uart1_flush(void)
{
	UART1_RxHead = UART1_RxTail;
} /* uart1_flush */

#endif

#endif /* defined( USART1_ENABLED ) */