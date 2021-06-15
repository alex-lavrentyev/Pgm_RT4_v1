// Burn utils

#ifndef		__BURN_H__
#define		__BURN_H__				1

#include	"defines.h"
#include	"bsp.h"
#include	"pgm.h"

extern U8		Burn_fuse_RT4( U8 bit);
extern U8		Burn_byte_RT4( U16 a, U8 b);
extern U8		Read_RT4( U16 a );

extern U8		Burn_fuse_RT5( U16 a, U8 bit);
extern U8		Burn_byte_RT5( U16 a, U8 b);
extern U8		Read_RT5( U16 a );

extern U8		Read_byte_UV_FLASH( U16 a );

extern	U8		Burn_byte_UV_24V( U16 a, U8 b);
extern	U8		Burn_byte_UV_12V( U16 a, U8 b);
extern	U8		Burn_byte_UV_12V_256( U16 a, U8 b);

//extern U8		Burn_byte_FLASH( U16 a, U8 b, U16 delay_us );
U8		Burn_byte_FLASH_WE( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс
U8		Burn_byte_FLASH_CE( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс
U8		Burn_byte_FLASH_10mS( U16 a, U8 b, U16 delay_us ); // адрес, данные, время импульса мкс

extern U8		Erase_all_FLASH();
extern U8		Erase_soft_28C64();

#endif