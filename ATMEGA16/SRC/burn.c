// Burn utils


#include "burn.h"


U8		Burn_fuse_RT4(U8 bit);
U8		Burn_byte_RT4( U16 a, U8 b);
U8		Read_RT4( U16 a );

U8		Burn_fuse_RT5( U16 a, U8 bit);
U8		Burn_byte_RT5( U16 a, U8 b);
U8		Read_RT5( U16 a );

U8		Read_byte_UV_FLASH( U16 a );
U8		Burn_byte_UV_24V( U16 a, U8 b);		// 24-вольтовые
U8		Burn_byte_UV_12V( U16 a, U8 b);		// 12-вольтовые
U8		Burn_byte_UV_12V_256( U16 a, U8 b);	// 27256

U8		Burn_byte_FLASH_WE( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс
U8		Burn_byte_FLASH_CE( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс
U8		Burn_byte_FLASH_10mS( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс
U8		Erase_all_FLASH();	// 12 V on OE in all cases 10ms min
U8		Erase_soft_28C64();


/**************************************************************
	Burn utils
***************************************************************/

U8		Read_RT4( U16 a )
{
	U8 data;
	
	Bus_dir_in();
	
	Clr_SEL_1();
	
	Set_address( a );
	
	__delay_cycles( TIKS_PER_US );
	
	data = Get_data_bits();
	
	Set_SEL_1();
	
	return data;
}

// KR556RT4
// Изначально содержит все 0
// Прошиваем только 1
// bit - номер бита, считая с 0
U8		Burn_fuse_RT4(U8 bit)
{
	U8 	q;
	U16 i;
		
	if( Get_bit_status( bit ) == 1 ) return OK; // "1" - бит уже прошит
	
	//////////////////////////////////////////
	// Normal phase
	// 1. Burn ~4-5 S if FAILED
	for( i = 0; i<RT4_NORMAL_N; i++ ) // RT4_NORMAL_N = 4000
	{
		for( q = 0; q < RT4_Q; q++ ) // RT4_Q = 20 * 50uS ~1mS
		{
			if( q == 0 )
			{ // burn
				Enable_high_voltage();
				Burn_fuse_en( bit );
				
				T1_delay_us( RT4_NORMAL_T );
				
				Burn_fuse_dis();
				Disable_high_voltage();
				
			}
			else
			{ // pause Q-1 times
				T1_delay_us( RT4_NORMAL_T );
			}
			
		}
		// Check burn completed after each cycle
		if( Get_bit_status( bit ) == 1 ) break; // "1" is already burned
	}
	// 2. Check
	if( Get_bit_status( bit ) == 0 )
	{
		//////////////////////////////////////////////
		// Forced, if normal failed
		for( i = 0; i<RT4_FORCED_N; i++ )	// RT4_FORCED_N = 100 ~20S
		{
			for( q = 0; q < RT4_Q; q++ ) // RT4_Q = 20 * 10mS ~200mS
			{
				if( q == 0 )
				{ // burn
					Enable_high_voltage();
					Burn_fuse_en( bit );
				
					T1_delay_ms( RT4_FORCED_T ); // RT4_FORCED_T = 10000 10mS
				
					Burn_fuse_dis();
					Disable_high_voltage();
				}
				else
				{ // pause Q-1 times
					T1_delay_ms( RT4_FORCED_T );
				}
			}
			// Check burn completed after each cycle
			if( Get_bit_status( bit ) == 1 ) break; // "1" is already burned			
		}
		// Check result
		if( Get_bit_status( bit ) == 0 ) return FAILED;
	}
	
	///////////////////////////////////////////////
	// Fixup at end
	for( i = 0; i<RT4_FIX_N; i++ )
	{
		for( q = 0; q < RT4_Q; q++ )
		{
			if( q == 0 )
			{ // burn
				Enable_high_voltage();
				Burn_fuse_en( bit );
				
				T1_delay_us( RT4_FIX_T );
				
				Burn_fuse_dis();
				Disable_high_voltage();
				
			}
			else
			{ // pause Q-1 times
				T1_delay_us( RT4_FIX_T );
			}
		}
	}
	return OK;
}

// KR556RT4
// Изначально содержит все 0
// Прошиваем только 1
U8		Burn_byte_RT4( U16 a, U8 b)
{
	U8 i;
	U8 ret;
	
	Set_address( a );		
	Bus_dir_in();		// for check ops

	Clr_SEL_1();
	
	ret = OK;
	
	// IC have only 4 bits per word
	// Burn 4 low bits of byte
	for( i = 0; i < 4; i++ )
	{
		if( (b & (1<<i)) != 0 ) ret = Burn_fuse_RT4( i ); // Burn fuse if "1" required
		if( ret == FAILED ) break; // stop on failed
	}
	
	Set_SEL_1();
	
	return ret;
}

/*
		556РТ5
// Изначально РТ5 содержит все 1
// Прожигаем только 0
//
*/
U8		Burn_fuse_RT5( U16 a, U8 bit)
{
	U16 i;
	
	// 0.Check
	Clr_SEL_1();
	Set_SEL_2();
	
	if( Get_bit_status(bit) == 0 ) return OK; // Already burned
	
	for( i = 0; i<RT5_NORMAL_N; i++ ) // х400 попыток * 250ms = 100сек
	{
		// 1. Burn
		Set_SEL_1();
		Clr_SEL_2();
		
		T1_delay_us( 5 );
		
		Enable_high_voltage();
		
		T1_delay_us( 1 );
		
		Burn_fuse_en( bit );
		
		T1_delay_ms( RT5_NORMAL_T ); // 50 мс
		
		Burn_fuse_dis();
		
		T1_delay_us( 1 );
		
		Disable_high_voltage();
		
		// 2. Check
		Clr_SEL_1();
		Set_SEL_2();
		
		T1_delay_us( 10 );
		
		if( Get_bit_status( bit ) == 0 ) return OK;
		
		// Next try (Q = 5)
		T1_delay_ms( RT5_NORMAL_T * (RT5_Q-1) ); // 200 ms
	}
	return FAILED;
}

// Изначально РТ5 содержит все 1
// Прожигаем только 0
//
U8		Burn_byte_RT5( U16 a, U8 b)
{
	U8 i;
	U8 ret;
	
	Bus_dir_in();
	
	ret = OK;
	// Burn 8 bits of byte
	for( i = 0; i < 8; i++ )
	{
		if( (b & (1<<i)) == 0 ) ret = Burn_fuse_RT5( a, i ); // Burn fuse if "0" required
		if( ret == FAILED ) break; // stop on failed
	}
	return ret;
}

//
U8		Read_RT5( U16 a )
{
	U8 data;
	
	Bus_dir_in();
	
	Clr_SEL_1();
	Set_SEL_2();
	
	Set_address( a );
	
	__delay_cycles( TIKS_PER_US );
	
	data = Get_data_bits();
	
	Set_SEL_1();
	Clr_SEL_2();
	
	return data;	
}

/*
	FLASH & UV READ читаются одинакково
*/
U8		Read_byte_UV_FLASH( U16 a )
{
	U8 data; 
	
	Set_WE();
	Bus_dir_in();
	Set_address( a );
	Clr_CE();
	Clr_OE();
	
	T1_delay_us( 1 );
	
	data = Get_data_bits();
	
	Set_OE(); // nOE
	Set_CE();
	
	return data;
}

/*
	Запись ПЗУ с ультафиолетовым стиранием
	2716 2732 with Vpp = 24v
	управляется импульсами CE 50 ms, когда подано Vpp = 24V
	return OK or FAILED
	June 2015 add WE control, need for i2764
*/
U8		Burn_byte_UV_24V( U16 a, U8 b)
{
	U8 data;
	
	// Setup lines
	Set_OE();
	Clr_CE();
	Clr_WE();
	
	Set_address( a );
	Bus_dir_out();
	Set_data_bits( b );
	
	Enable_high_voltage();
	
	// Wait pp
	Delay_uS( 3 );
	
	// Pulse 50 ms
	Set_CE();
	T1_delay_ms( 50 );
	Clr_CE();
	
	Disable_high_voltage();
	
	// Check
	Bus_dir_in();
	Clr_OE();
	Delay_uS( 3 );

	data = Get_data_bits();
	
	Set_OE();	// Hi-Z
	
	Set_CE();
	Set_WE();
	
	if( data != b ) return FAILED;
	
	return OK;
}

/*
	Запись 2764
	Vpp = 12V
	June 2015 add WE control, need for i2764	
*/
U8		Burn_byte_UV_12V( U16 a, U8 b)
{
	U8 data;
	U8 count;
	
	// Setup lines
	Set_OE();
	Set_WE(); // nPGM
	Set_address( a );
	Clr_CE();
	Bus_dir_out();
	Set_data_bits( b );
	
	Delay_uS(2);
	
	//count = 25;	// retry count
	
	Enable_high_voltage();
	
	for( count=0; count < 25; count++ )
	{
		// Programm 1 ms pulse
		Clr_WE();
		T1_delay_us( 1000 );
		Set_WE();
		
		Bus_dir_in();
		Clr_OE();
		Delay_uS(3);
		
		data = Get_data_bits();
		
		Set_OE();	// !!!
		Bus_dir_out();
		Delay_uS(3);
		
		if( data == b ) break; // Complete !
		
	}
	
	
	if( data != b )
	{
		Disable_high_voltage(); // June 2015
		
		Set_CE();
		return FAILED;
	}
	
	// Fix pulse 3 ms
	Clr_WE();
	T1_delay_us( 3000 );
	Set_WE();

	Disable_high_voltage(); // June 2015
	
	// Final check
	Bus_dir_in();
	Clr_OE();
	Delay_uS(3);
		
	data = Get_data_bits();
		
	Set_OE();	// !!!
	Bus_dir_out();
	Delay_uS(3);
		
	Set_CE();
	
	if( data != b ) return FAILED; // Complete !
	
	return OK;
}

/*
	Burn 27256
	have different programming sequence
*/
U8		Burn_byte_UV_12V_256( U16 a, U8 b)
{
	U8 data;
	U8 count;
	
	// Setup lines
	Set_OE();
	Set_WE(); // nPGM
	Set_address( a );
	Set_CE();
	Bus_dir_out();
	Set_data_bits( b );
	
	Enable_high_voltage();
	
	Delay_uS(2);
	
	// Try 25 times
	for( count=0; count < 25; count++ )
	{
		// Programm 1 ms pulse
		// OE = 1, CE = 0, Vpp = Vh
		Clr_WE();
		Clr_CE();
		T1_delay_us( 1000 );
		Set_CE();
		Set_WE();
		
		// Verify
		// OE = 0, CE = 1, Vpp = Vh
		Delay_uS(2);	// 2uS Tdh
		Bus_dir_in();
		Delay_uS(2);		// 2uS Toes
			
		Clr_OE();
		Delay_uS(2);
		
		data = Get_data_bits();
		
		// Next cycle or completed
		// OE = 1, CE = 1, Vpp = Vh
		Set_OE();	// !!!
		Bus_dir_out();
		Delay_uS(2);
		
		if( data == b ) break; // Complete !
		
	}
	
	
	if( data != b )
	{
		// Program FAILED
		// Vpp = 0 or Vdd, CE = 1
		Disable_high_voltage(); // June 2015	
		Set_CE();
		return FAILED;
	}
	
	// Fix pulse 3 ms
	// OE = 1, CE = 0, Vpp = Vh
	
	Clr_WE();
	Clr_CE();
	T1_delay_us( 3000 );
	Set_CE();
	Set_WE();

	Delay_uS(2);	// Tdh
	
	Disable_high_voltage(); // June 2015
	
	// Final check
	Bus_dir_in();
	
	Delay_uS(2);	// Toes ???
		
	Clr_OE();
	Clr_CE();
	Delay_uS(2);
		
	data = Get_data_bits();
		
	Set_OE();	// !!!
	Bus_dir_out();
	Delay_uS(3);
		
	Set_CE();
	
	if( data != b ) return FAILED; // Complete !
	
	return OK;
}



/*
	Запись ПЗУ FLASH-технологии

	28C16 	Twp = 100 - 1000nS, Tcycle = 0,5 - 1,0 mS (!!!)
	28C16E	Twp = 100 - 1000nS, Tcycle = 100 - 200 uS (!!!)
	28C64 	Twp = 100 min, Tcycle = 250nS min

	WE-controlled algoritm
	Задержка в системных тиках
*/

#define		TOGGLE_MASK			0x40
#define		POLLING_MASK		0x80

U8		Burn_byte_FLASH_WE( U16 a, U8 b, U16 tiks )
{ 
	U16 i;
	U8 status;
	U8 check;
	
	
	Set_WE();	// All passive
	Set_OE();
	Set_CE(); 
		
	Delay(8);
	
	Set_address( a );
	Bus_dir_out();
	Set_data_bits( b );
	
	Clr_CE();
			
	Delay(8);
	
	// Burn pulse
	__disable_interrupt();
	Clr_WE();
	__delay_cycles( 8 ); // 
	Set_WE();
	__enable_interrupt();
	
	Delay_uS(3);
	
	// Prepare to check
	Set_CE();
	Bus_dir_in();
	
	Delay_uS(2);
	
	// Check done use DATA POLLING method
	// IO7 have complement (inverted) value, until data was written
	// Use IO7 pin to control see datasheet for detail
	
	i = 0;
	check = (b & POLLING_MASK);
	
	do
	{
		// Timeout
		//if( i == 50000) break;
		if( i == 0xFFFF) break;
		
		Clr_CE();
		Clr_OE();
		//Delay_uS(1);
		Delay( 8 );
		
		status = Get_data_bits() & POLLING_MASK;
		
		Set_OE();
		Set_CE();
		Delay( 8 ); // ~1uS
		
		i++;
	}while( status != check );
	
	
	// Check full data bits
	Clr_CE();
	Clr_OE();
	Delay(8);
		
	status = Get_data_bits();
		
	Set_OE(); 
	Set_CE();
		
	if( status == b )return OK; // Success !!!
	return FAILED; // Error !!!
}


U8		Burn_byte_FLASH_CE( U16 a, U8 b, U16 tiks )
{ 
	U16 i;
	U8 status;
	U8 check;
	
	
	Set_WE();
	Set_OE();
	Set_CE(); 
		
	Set_address( a );
	Bus_dir_out();
	Set_data_bits( b );
	
	Clr_WE();

	Delay_uS(3);
	
	// Burn pulse
	__disable_interrupt();
	Clr_CE();
	__delay_cycles( 8 ); // 
	Set_CE();
	__enable_interrupt();
	
	Delay_uS(3);
	
	// Prepare to check
	Set_WE();
	Bus_dir_in();
	
	Delay_uS(2);
	
	// Check done use DATA POLLING method
	// IO7 have complement (inverted) value, until data was written
	// Use IO7 pin to control see datasheet for detail
	
	i = 0;
	check = (b & POLLING_MASK);
	
	do
	{
		// Timeout
		//if( i == 50000) break;
		if( i == 0xFFFF) break;
		
		Clr_CE();
		Clr_OE();
		//Delay_uS(1);
		Delay( 8 );
		
		status = Get_data_bits() & POLLING_MASK;
		
		Set_OE();
		Set_CE();
		Delay( 8 ); // ~10uS
		
		i++;
	}while( status != check );
	
	
	// Check full data bits
	Clr_CE();
	Clr_OE();
	Delay(16);
		
	status = Get_data_bits();
		
	Set_OE(); 
	Set_CE();
		
	if( status == b )return OK; // Success !!!
	return FAILED; // Error !!!
}


U8		Burn_byte_FLASH_10mS( U16 a, U8 b, U16 tiks )
{ 
	U16 i;
	U8 status;
	
	
	Set_WE();
	Set_OE();
	Set_CE(); 
		
	Set_address( a );
	Bus_dir_out();
	Set_data_bits( b );
	
	Clr_WE();

	Delay_uS(3);
	
	// Burn pulse
	__disable_interrupt();
	Clr_CE();
	__delay_cycles( 8 ); // 
	Set_CE();
	__enable_interrupt();
	
	Delay_uS(3);
	
	// Prepare to check
	Set_WE();
	Set_CE();
	Bus_dir_in();
	
	// Simple 10mS wait	
	for( i=0; i<11; i++)
	{
		Delay( 1000 * TIKS_PER_US ); // mS
	}
	
	
	// Check full data bits
	Clr_CE();
	Clr_OE();
	Delay(16);
		
	status = Get_data_bits();
		
	Set_OE(); 
	Set_CE();
		
	if( status == b )return OK; // Success !!!
	return FAILED; // Error !!!
}


/*
 12 V on OE in all cases 10ms min
 Для очистки всей микросхемы подаётся комбинация 
	CE = L, OE = +12,5V, WE = L min 10 ms
*/
U8		Erase_all_FLASH()
{

	Enable_high_voltage(); // 12 v on OE pin
	
	Clr_CE();
		
	Delay_uS(2);
	
	Clr_WE();
	
	T1_delay_ms( 10 ); // !!!
	
	Set_WE();
	
	Delay_uS(2);
		
	Set_CE();
	
	Disable_high_voltage();	 	
	
	return OK;
}

/*
	Write sequence 6 bytes
	Address		Data
1	0x5555		0xAA
2	0x2AAA		0x55
3	0x5555		0x80
4	0x5555		0xAA
5	0x2AAA		0x55
6	0x5555		0x10
*/
U8		Erase_soft_28C64()
{
	Set_CE();
	Set_OE();
	Set_WE();

	Bus_dir_out();
	
	// 1
	Set_address( 0x5555 );
	Set_data_bits( 0xAA );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);

	// 2
	Set_address( 0x2AAA );
	Set_data_bits( 0x55 );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);

	// 3
	Set_address( 0x5555 );
	Set_data_bits( 0x80 );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);
	
	// 4
	Set_address( 0x5555 );
	Set_data_bits( 0xAA );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);
	
	// 5
	Set_address( 0x2AAA );
	Set_data_bits( 0x55 );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);
	
	// 6
	Set_address( 0x5555 );
	Set_data_bits( 0x10 );
	
	Clr_CE();
	Clr_WE();
	
	Delay(8);
	
	Set_WE();
	Set_CE();
	
	Delay(8);
	
	//===================
	T1_delay_ms( 200 ); // (!!!) Time to ERASE
	//===================
	Set_CE();
	Set_OE();
	Set_WE();
	
	return OK;
}
