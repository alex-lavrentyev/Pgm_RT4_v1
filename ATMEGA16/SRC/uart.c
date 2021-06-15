/*
*/

#include "bsp.h"
#include "defines.h"
#include "pgm.h"
#include "uart.h"
#include "ring_buffer.h"


// Prototypes
void	UART_init();
//void	UART_send_byte( U8 byte );
void	UART_send_buffer();
U8		UART_check_busy();

// Variables
static U8	status;
//static U16*	tx_ptr;
//static U16	tx_size;


/*
	115200 8N2
*/
void	UART_init()
{
	ring_buffer_init( &RxRing, UartRxBuffer, UART_RXBUFFER_SIZE );
	ring_buffer_flush( &RxRing );
	
	ring_buffer_init( &TxRing, UartTxBuffer, UART_TXBUFFER_SIZE );
	ring_buffer_flush( &TxRing );
	
	UCSRC = (1<<URSEL)|(1<<USBS)|(1<<UCSZ0)|(1<<UCSZ1);	// 2 stop-bits
	UBRRL = 3;	// 115200
	UCSRA = 0;
	UCSRB |= (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	status 	= READY;
	//index	= 0;
}


// Simple send byte
/*
void	UART_send_byte( U8 byte )
{
	while( (UCSRA & (1<<UDRE)) == 0 ); // wait until prev byte send
	UDR = byte;
}
*/

/*
	Предатчик теперь работает тоже с кольцевым буфером
*/
void	UART_send_buffer()
{
	//tx_ptr = addr;
	//tx_size = size;
	
	UCSRB |= (1<<UDRIE);
}


U8		UART_check_busy()
{
	return status;
}

/*
		I   S   R
*/
#pragma vector = USART_UDRE_vect
__interrupt void UDRE_handler()
{
	if( ring_buffer_get_used( &TxRing ) == 0 )
	{ // Больше нет данных для передачи
		UCSRB &= ~(1<<UDRIE);
	}
	else
	{
		U8 data;
		ring_buffer_read( &TxRing, &data, 1 );
		
		UDR = data;
	}
	/*
	UDR = *tx_ptr;
	
	tx_size--;
	
	if( tx_size != 0 ) tx_ptr++;  // next byte address
	else UCSRB &= ~(1 << UDRIE);  // IRQ disable
	*/
}

#pragma vector = USART_RXC_vect
__interrupt void	RXC_handler()
{
	U8 data;
	
	data = UDR;
	
	ring_buffer_write( &RxRing, &data, 1 );
	
	Set_income_cmd_flag();
	
	/*
	if( Income_cmd_flag_status() ) return; // Busy !!!
	
	data = UDR;
	
	if( index == 0 )
	{
		switch( data )
		{
			case READ_RT4:	
			case READ_RT5:	
			case WRITE_RT4:
			case WRITE_RT5:
			{ 
				rx_buffer[index] = data; 
				index++; 
				break;
			}
			case PGM_RESTART:
			{
				rx_buffer[0] = data; 	
				Set_income_cmd_flag();
			}
			default: { break; }
		}
	}
	else
	{
		rx_buffer[index] = data;
		index++;
		
		// не должно быть неопределённых значений
		switch( rx_buffer[0] )
		{
			case READ_RT4: 
			case READ_RT5: 
			{
				if( index == 3 ) { index=0; Set_income_cmd_flag(); }
				break;
			}
			case WRITE_RT4:
			case WRITE_RT5:
			{
				if( index == 4) { index=0; Set_income_cmd_flag(); }
			}
		}
	}
	*/
}
//------------------------------------------------------------------------------

