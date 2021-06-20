/*
*/

#ifndef		__UART_H__
#define		__UART_H__				1


extern void	UART_init();
//extern void	UART_send_byte( U8 byte );
extern void	UART_send_buffer();
extern U8	UART_check_busy();

//extern U8			rx_buffer[UART_RXBUFFER_SIZE];
#endif
