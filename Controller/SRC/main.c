/*
		Программатор 
	2716 	- 27256		UV EPROM
	28С16 	- 28С256	AT FLASH
	KR556RT4
	KR556RT5

	UART на скорости 115200
		
		556RT4 до программирования содержит все "0", т.е. прожигаем только "1"
*/

#include "bsp.h"
#include "pgm.h"
#include "uart.h"
#include "burn.h"
#include "tools.h"

////////////////////////////////////////////////////////////////////////////////
// Prototypes
void	PowerOnInit();
void	Process_income_command();

U8		Get_bit_status( U8 b );	// return 0 or 1

// system intervals
void	T0_start_10ms();		

// Delay utils
void	T1_delay_us( U16 us);
void	T1_delay_ms( U16 ms );
U8		Is_delay_complete();
//void	BlockedDelay() { while( Is_delay_complete() == 0 ); }


// Операции
void	ProcessPgmRestart();

void	ProcessRead_RT4();
void	ProcessWrite_RT4();

void	ProcessRead_RT5();
void	ProcessWrite_RT5();


void	ProcessRead_UV_FLASH();
void	ProcessEraseFlashAll();
void	ProcessWrite_UV_24V();		// 24-вольтовые
void	ProcessWrite_UV_12V();		// 12-вольтовые
void	ProcessWrite_UV_12V_256();
void	ProcessWrite_FLASH(); // адрес, данные, время импульса мкс

void	ProcessEraseFlashSoft2864();



/////////////////////////////////////////////////////////////////////
// Variables
volatile U8		flag_10ms;
volatile U8		addr_hi_status; // Hi 8 bits of address
//volatile bool	t0_event;

static	volatile U8		t1_busy = 0; // 1 - timer count, 0 - timer done

volatile U8 IncomeCmd;	// флаг приёма команды

U8		LedCount;
U8		LedStatus;


static U8		Command;	// Команды
static U8		fWaitArgs;	// ждём аргументы

UNIT				UartRxBuffer[UART_RXBUFFER_SIZE];
UNIT				UartTxBuffer[UART_TXBUFFER_SIZE];
RING_BUFFER_DATA	RxRing, TxRing;


/**********************************************************
	Program start there
************************************************************/
void 	main( void)
{
	//U16 counter;
	
	__disable_interrupt();
	
	addr_hi_status = 0;
	flag_10ms = 0;
	
	LedCount = 100;
	LedStatus = FALSE;
	
	PowerOnInit();
	
	fWaitArgs = 0;
	
	LED_Off();
	
	///////////////////////////////////
	// Test block
	//Burn_byte_RT5( 0x0001, 0xF0);
	//
	///////////////////////////////
		
	__enable_interrupt();
	
	/////////////////////////////////////
	// Test block
	/*
	U16 count = 0;
	while(1)
	{
		T1_delay_ms(3);
		Burn_byte_UV_12V_256( count, 0xF0);
		count++;
	}
	*/
	//
	//////////////////////////////////////
	
	while(1)
	{
		
		////////////////////////////////////////
		// 10 ms Timer Events
		if( flag_10ms )
		{
			__disable_interrupt();
			flag_10ms = 0;
			__enable_interrupt();
			// To Do
			
			if( LedCount )
			{
				LedCount--;
			}
			else
			{
				LedCount = 100;
				if(LedStatus == FALSE) { LED_On(); LedStatus = TRUE; }
				else { LED_Off(); LedStatus = FALSE; }				\
			}
			
		}
		//
		////////////////////////////////////////
		
		////////////////////////////////////////
		// Income command complete
		if( Income_cmd_flag_status() )
		{			
			__disable_interrupt();
			Clr_income_cmd_flag();
			__enable_interrupt();
		}
		
		Process_income_command();
		//
		////////////////////////////////////////
	}
}

/************************************************
	Начальная инициализация
************************************************/
void	PowerOnInit()
{
	Setup_io_ports();
	
	// !!!
	Burn_fuse_dis();
	
	Disable_high_voltage();

	Set_WE();	
	Set_OE();
	Set_CE();
	
	UART_init();
	T0_start_10ms();
	
	t1_busy = 0;
}

/*
 system intervals
	7372800 / 100 = 73728
	73728 / 1024 = 42
	Count = 72
*/
#define	T0_LOAD_VALUE	(0xFF-72)
void	T0_start_10ms()
{
	TCNT0	= T0_LOAD_VALUE;
	TIMSK	|=  1 << TOIE0;
	TCCR0	= (1<<CS02)|(1<<CS00);	// Start at Fosc/1024
}

/*
	Обработка команд
*/
void	Process_income_command()
{		
	
	while( ring_buffer_get_used( &RxRing ) != 0 )
	{	
		if( fWaitArgs == 0 )
		{ // Ждём код команды
			ring_buffer_read( &RxRing, &Command, 1 ); // Один байт команды
		
			switch( Command )
			{
				case PGM_RESTART: { ProcessPgmRestart(); break; }
				case ERASE_2864: { ProcessEraseFlashSoft2864(); break; }
				case ERASE_2816: { ProcessEraseFlashAll(); break; }
				case RUN_ADDRESS: { ProcessRunAddress(); break; }
				//case ERASE_2864E: { ProcessEraseFlashAll(); break; }
				default: { fWaitArgs = 1; break;}
			}		
		}
		else
		{ // Ждём количество аргументов
			switch( Command )
			{
				case READ_RT4: 	{ ProcessRead_RT4(); break; }
				case WRITE_RT4: { ProcessWrite_RT4(); break; }
			
				case READ_RT5:	{ ProcessRead_RT5(); break; }
				case WRITE_RT5:	{ ProcessWrite_RT5(); break; }
			
				case READ_2716:	{ ProcessRead_UV_FLASH(); break; }
				case READ_2764:	{ ProcessRead_UV_FLASH(); break; }
				case READ_2816:	{ ProcessRead_UV_FLASH(); break; }
				case READ_2864:	{ ProcessRead_UV_FLASH(); break; }
			
				case WRITE_2716: { ProcessWrite_UV_24V(); break; }
				case WRITE_2764: { ProcessWrite_UV_12V(); break; }
				case WRITE_27256: { ProcessWrite_UV_12V_256(); break; }
			
				case WRITE_2816: { ProcessWrite_FLASH(); break; }
				case WRITE_2816E: { ProcessWrite_FLASH(); break; }
				case WRITE_2864: { ProcessWrite_FLASH(); break; }
							
				default: { fWaitArgs = 0; break; } // Команда не определена ??????
			} // switch( Command )
		} // else (fArgs==1)
	} // while( есть данные в буфере )
}


/*******************************************************************************
	Обработка запрошенных операций
*******************************************************************************/

//
// Рестарт
void	ProcessPgmRestart()
{
	U8 data = RET_OK;
	
	Burn_fuse_dis();
	Disable_high_voltage();
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}


//
// 2 bytes argument
void	ProcessRead_RT4()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 2 ) return;
	
	ring_buffer_read( &RxRing, (U8*)&a, 2);

	data = Read_RT4( a );
	
	ring_buffer_write( &TxRing, &data, 1 );
	
	/*
	// Найден баг в функции ring_buffer_read(...), см исходник;
	ring_buffer_write( &TxRing, &Command, 1 );
	ring_buffer_read( &RxRing, (UNIT*)&a, 2);
	ring_buffer_write( &TxRing, (UNIT*)&a, 2 );
	*/
	
	UART_send_buffer();
	
	fWaitArgs = 0;
}

//
// 3 bytes argument
void	ProcessWrite_RT4()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	
	fWaitArgs = 0;

	data = data & 0x0F;	// только 4 младших бита
	
	if( Burn_byte_RT4( a, data ) == OK ) 
	{
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}


//
//
void	ProcessRead_RT5()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 2 ) return;
	
	ring_buffer_read( &RxRing, (U8*)&a, 2);

	fWaitArgs = 0;
	
	data = Read_RT5( a );
	
	//UART_send_byte( data );
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
	
}

//
//
void	ProcessWrite_RT5()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	fWaitArgs = 0;
	
	if( Burn_byte_RT5( a, data) == OK )
	{
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();

}

//
//
void	ProcessRead_UV_FLASH()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 2 ) return;
	
	ring_buffer_read( &RxRing, (U8*)&a, 2);

	data = Read_byte_UV_FLASH( a );
	
	ring_buffer_write( &TxRing, &data, 1 );
	
	UART_send_buffer();
	
	fWaitArgs = 0;
}

//
//
void	ProcessEraseFlashAll()
{
	U8 data;
	
	if( Erase_all_FLASH() == OK ) data = RET_OK;
	else data = RET_ERROR;
	
	ring_buffer_write( &TxRing, &data, 1 );
	
	UART_send_buffer();
}

void	ProcessEraseFlashSoft2864()
{
	U8 data;
	
	if( Erase_soft_28C64() == OK ) data = RET_OK;
	else data = RET_ERROR;
	
	ring_buffer_write( &TxRing, &data, 1 );
	
	UART_send_buffer();
}

//
// 24-вольтовые
void	ProcessWrite_UV_24V()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	fWaitArgs = 0;
	
	if( Burn_byte_UV_24V( a, data) == OK )
	{
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}

//
// 12-вольтовые
void	ProcessWrite_UV_12V()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	fWaitArgs = 0;
	
	if( Burn_byte_UV_12V( a, data) == OK )
	{
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}

void	ProcessWrite_UV_12V_256()
{
	U16 a;
	U8	data;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	fWaitArgs = 0;
	
	if( Burn_byte_UV_12V_256( a, data) == OK )
	{
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}


//
//
void	ProcessWrite_FLASH()
{
	U16 a;
	U8	data, result;
	
	if( ring_buffer_get_used( &RxRing ) < 3 ) return;

	ring_buffer_read( &RxRing, (U8*)&a, 2);	// читаем адрес
	ring_buffer_read( &RxRing, &data, 1 );   // читаем данные
	
	fWaitArgs = 0;
	
	// ТК разные флэш имеют разные методы определение готовности, 
	// для непонятых пользуем функцию с простым таймаутом
	switch( Command )
	{
		case WRITE_2864: { result = Burn_byte_FLASH_WE( a, data, 0); break; }
		default: {         result = Burn_byte_FLASH_10mS( a, data, 0); break; }
	}
		
	if( result == OK ) 
	{ 
		data = RET_OK;
	}
	else
	{
		data = RET_ERROR;
	}
	
	ring_buffer_write( &TxRing, &data, 1 );
	UART_send_buffer();
}
/******************************************************************************

*******************************************************************************/


/*
	Состояние бита шины данных
*/
//#pragma inline = forced
U8	Get_bit_status( U8 b  )
{
	U8 q;
	
 	q = Get_data_bits();
	if( (q & (1 << b)) == 0 ) return 0;
	return 1;
}


/*
	7372800 / 1024 = 7200
	7 тактов на мс
	9362 мс макс
*/
#define			TIKS_PER_MS		7 // 14
void	T1_delay_ms( U16 ms )
{
	while( ms )
	{
		Delay_uS(1000);
		ms--;
	}
	/*
	t1_busy	= 1;
	
	TCNT1	= 	0x0000;
	OCR1A	= 	ms * TIKS_PER_MS;
	TCCR1A	=	0x00;
	TIMSK	= 	(1<<OCIE1A);
	TCCR1B	= (1<<CS12)|(1<<CS10)|(1<<WGM12); // Start at Fosc/1024, CTC
	
	while( t1_busy ) __no_operation();
	*/
}

/*
	Настраивает таймер T1 отсчитать максимум 4369 мкс
*/
void	T1_delay_us( U16 us)
{
	while( us )
	{
		Delay_uS(1);
		us--;
	}
	/*
	t1_busy	= 1;
	
	TCNT1	= 0x0000;
	OCR1A	= us * TIKS_PER_US;
	TCCR1A	= 0x00;
	TIMSK	= (1<<OCIE1A);		// Interrupt on OC1A
	TCCR1B	= (1<<CS10) | (1<<WGM12); // Start at Fosc, CTC
	
	while( t1_busy ) __no_operation();
	*/
}


/*
	Return 1 if delay done
*/
#pragma inline = forced
U8		Is_delay_complete()
{ 
	return ((t1_busy == 0)?1:0); 
}
/*
			I   S   R
*/
#pragma vector = TIMER1_COMPA_vect
__interrupt void T1_handler()
{
	TCCR1B	= 0x00;		// Stop timer
	t1_busy = 0;
}

#pragma vector = TIMER0_OVF_vect
__interrupt void T0_handler()
{
	TCNT0	= T0_LOAD_VALUE; // reLoad
	flag_10ms = 1;	
}

//-------------------------------------------------------------------------
