/*
*/

#include "tools.h"

void	ProcessRunAddress();


extern U8 LedStatus;
extern U8 LedCount;
extern RING_BUFFER_DATA	RxRing, TxRing;
extern volatile U8 flag_10ms;


/*
	Пока по UART что-нибудь не придёт
*/
void	ProcessRunAddress()
{
	U16 address;
	
	address = 0;
	
	Clr_SEL_1();	// Активируем
	Set_SEL_2();
	
	do
	{
		Set_address( address & 0x7FFF);
		address++;	// покрутили ...

		
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
				LedCount = 30;
				if(LedStatus == FALSE) { LED_On(); LedStatus = TRUE; }
				else { LED_Off(); LedStatus = FALSE; }				\
			}
			
		}
		//
		////////////////////////////////////////

	}while( ring_buffer_get_used( &RxRing ) == 0 );
	
	Set_SEL_1();	// nOE для UV/FLASH
}


//------------------------------------------------------------------------------
