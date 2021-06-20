/*
    Управление кольцевым буфером
*/

#ifndef     __RING_BUFFER__
#define     __RING_BUFFER__		1

#include "intrinsics.h"

// Comment next string to disable critical sections
#define		USE_CRITICAL_SECTIONS	1

/********************************************************************
	Платформозависимые описания типов
*********************************************************************/

// Single unit in buffer
#ifndef		UNIT
#define		UNIT		unsigned char
#endif

#ifndef		PUNIT
#define		PUNIT		UNIT*
#endif

//  Maximum buffer size
#ifndef		BSIZE
#define		BSIZE		unsigned int
#endif


// Кольцевой буфер включает в себя семафор
// для исключения одновременного доступа
//#define     RING_BUFFER_USE_LOCK    1

#ifdef      RING_BUFFER_USE_LOCK
typedef enum
{
    RING_BUFFER_FREE = 0,
    RING_BUFFER_LOCKED
}RING_BUFFER_LOCK_STATUS;
#endif

typedef struct
{
    UNIT       *pBuffer;   // адрес начала буфера
    UNIT       *pTop;      // адрес вершины области памяти
    UNIT       *pHead;     // запись
    UNIT       *pTail;     // чтение
    BSIZE      size;       // размер буфера в байтах
    BSIZE      items;      // количество байт в буфере
#ifdef      RING_BUFFER_USE_LOCK
    RING_BUFFER_LOCK_STATUS mutex;
#endif    
}RING_BUFFER_DATA;

extern void		ring_buffer_init( RING_BUFFER_DATA *pRing, void *pBuffer, BSIZE size );
extern BSIZE	ring_buffer_get_free(RING_BUFFER_DATA *pRing );
extern BSIZE	ring_buffer_get_used(RING_BUFFER_DATA *pRing );
extern BSIZE	ring_buffer_write( RING_BUFFER_DATA *pRing, UNIT *pSrc, BSIZE items );
extern BSIZE	ring_buffer_read( RING_BUFFER_DATA *pRing, UNIT *pDst, BSIZE items );
extern void     ring_buffer_flush(RING_BUFFER_DATA *pRing );

#ifdef      RING_BUFFER_USE_LOCK
extern UNIT    	ring_buffer_lock(RING_BUFFER_DATA *pRing );
extern void     ring_buffer_unlock(RING_BUFFER_DATA *pRing );
#endif

#endif
//------------------------------------------------------------------------------
