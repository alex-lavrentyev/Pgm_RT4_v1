/*
		BOARD & Custom types
*/

#ifndef		__BSP_H__
#define		__BSP_H__			1

#include 	"defines.h"
#include 	"iom16.h"
#include	"intrinsics.h"
#include	"protocol.h"
#include	"ring_buffer.h"

#define		FOSC			7372800 	//14745600
#define		TIKS_PER_US		7 			// 15
#define		TOP_US_COUNT	(65535 / TIKS_PER_US)


#define		Delay(c)		__delay_cycles(c)
#define		Delay_uS(c)	    __delay_cycles(TIKS_PER_US * c)

#define		UART_RXBUFFER_SIZE		8
#define		UART_TXBUFFER_SIZE		8


// Bit definition for control port

/*
	PORTA	- двунаправленная шина данных 
	PORTB	- управление прожигом РТ4, РТ5	1 - прожигает бит
	PORTC	- вывод адреса
	PORTD	- управление
*/

#define		BIT_RXD		0
#define		BIT_TXD		1
#define		BIT_SEL_2	2
#define		BIT_SEL_1	3
#define		BIT_WE		4
#define		BIT_RESERV	5
#define		BIT_HVOLT	6		// 1 -разрешает высокое напряжение
#define		BIT_ALE		7

//==============================================================
// Low level macros
// Lines control
#define		Setup_io_ports() { DDRA = 0x00; \
								PORTB = 0x00; \
								DDRB = 0xFF; \
								DDRC = 0xFF; \
								PORTD = 0x1F; \
								DDRD = 0xFF; }

// Simple ops

// Установить данные на шине
#define 	Set_data_bits( d )			PORTA = d

// Прочитать состояние шины данных
#define 	Get_data_bits()				PINA

// Направление шины данных
#define		Bus_dir_out()				DDRA = 0xFF //Program
#define		Bus_dir_in()				DDRA = 0x00 // Read/Verify

// Активировать прожиг перемычки только РТ4 РТ5
// Активировать можно только 1 бит, аргумент = номер бита (0-7)
#define		Burn_fuse_en( b )			PORTB	= (1<<b)

// Отключить прожиг перемычки
#define		Burn_fuse_dis()				PORTB	= 0x00		// отключить сразу ВСЕ!!!

// Подать высокое напряжение
#define		Enable_high_voltage()		PORTD	|= (1<<BIT_HVOLT)

// Снять высокое напряжение
#define		Disable_high_voltage()		PORTD	&= ~(1<<BIT_HVOLT)

// Управление WE для FLASH
//#define		Set_WE()					PORTD	|= (1<<BIT_WE)
//#define		Clr_WE()					PORTD	&= ~(1<<BIT_WE)

// Управление ALE
#define		Set_ALE()					PORTD 	|= (1<<BIT_ALE)
#define		Clr_ALE()					PORTD	&= ~(1<<BIT_ALE)

// Управление SEL_1 & SEL_2
#define		Set_SEL_1()					PORTD	|= (1<<BIT_SEL_1)
#define		Clr_SEL_1()					PORTD	&= ~(1<<BIT_SEL_1)
#define		Set_SEL_2()					PORTD	|= (1<<BIT_SEL_2)
#define		Clr_SEL_2()					PORTD	&= ~(1<<BIT_SEL_2)


// Установить адрес на шине и защёлкнуть
// a.7 always = 0;
extern volatile U8 addr_hi_status;
extern U8 LedStatus;
#define		Set_address( a ) { addr_hi_status = HIBYTE(a) | (LedStatus << 7); PORTC = addr_hi_status; Set_ALE(); Delay(8); Clr_ALE(); PORTC = LOWBYTE(a); }

//===============================================================
// ROM control
#define		Set_OE()			Set_SEL_1()
#define		Clr_OE()			Clr_SEL_1()

#define		Set_CE()			Set_SEL_2()
#define		Clr_CE()			Clr_SEL_2()

#define		Set_WE()			PORTD |= (1<<BIT_WE)
#define		Clr_WE()			PORTD &= ~(1<<BIT_WE)

//===================================================================

//===================================================================
// LED control 1 = On
#define		LED_On()		{ PORTC = PORTC | 0x80; Set_ALE(); Delay(8); Clr_ALE();}
#define		LED_Off()		{ PORTC = PORTC & 0x7F; Set_ALE(); Delay(8); Clr_ALE();}

extern volatile U8 IncomeCmd;
#define	Set_income_cmd_flag()	{ IncomeCmd = 1; }
#define	Clr_income_cmd_flag()	{ IncomeCmd = 0; }
#define	Income_cmd_flag_status()	IncomeCmd

extern void	T1_delay_us( U16 us);
extern void	T1_delay_ms( U16 ms );


extern U8	Get_bit_status( U8 b  );

extern UNIT				UartRxBuffer[UART_RXBUFFER_SIZE];
extern UNIT				UartTxBuffer[UART_TXBUFFER_SIZE];
extern RING_BUFFER_DATA	RxRing, TxRing;


#endif