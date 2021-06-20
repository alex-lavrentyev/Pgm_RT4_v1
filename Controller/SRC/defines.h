/*
*/

#ifndef		__DEFINES_H__
#define		__DEFGNES_H__			1

#ifndef		FALSE
#define		FALSE		0
#endif

#ifndef		TRUE
#define		TRUE		1
#endif

#define		U8			unsigned char
#define		U16			unsigned int
#define		U32			unsigned long


#define		S8			signed char
#define		S16			signed int
#define		S32			signed long

#define		BUSY		1
#define		READY		0

#define		HIBYTE(x)	(U8)(x>>8)
#define		LOWBYTE(x)	(U8)(x)


#endif
//-----------------------------------------------------------------------------
