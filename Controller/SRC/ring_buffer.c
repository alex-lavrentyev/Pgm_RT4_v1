/*
    Упрапвление кольцевым буфером
*/

#include "ring_buffer.h"


void        ring_buffer_init( RING_BUFFER_DATA *pRing, void *pBuffer, BSIZE size );
BSIZE		ring_buffer_get_free(RING_BUFFER_DATA *pRing );
BSIZE		ring_buffer_get_used(RING_BUFFER_DATA *pRing );
BSIZE		ring_buffer_write( RING_BUFFER_DATA *pRing, UNIT *pSrc, BSIZE items );
BSIZE		ring_buffer_read( RING_BUFFER_DATA *pRing, UNIT *pDst, BSIZE items );
void        ring_buffer_flush(RING_BUFFER_DATA *pRing );

#ifdef      RING_BUFFER_USE_LOCK
UNIT		ring_buffer_lock(RING_BUFFER_DATA *pRing );
void        ring_buffer_unlock(RING_BUFFER_DATA *pRing );
#endif


//////////////////////////////////////////////////////////
// Критические секции
#ifdef	USE_CRITICAL_SECTIONS

#include "intrinsics.h"

__istate_t		i_state;

void	BeginCriticalSection();
void	EndCriticalSection();

void	BeginCriticalSection()
{	
	i_state = __get_interrupt_state();
	__disable_interrupt();
}

void	EndCriticalSection()
{
	__set_interrupt_state( i_state );
}
#else


#define		BeginCriticalSection()
#define		EndCriticalSection()

#endif
//------------------------------------------
//
void		ring_buffer_init( RING_BUFFER_DATA *pRing, void *pBuffer, BSIZE size )
{
    pRing->pBuffer = (UNIT *)pBuffer;
    pRing->pTop    = pRing->pBuffer + size;
    pRing->pHead   = pRing->pBuffer;
    pRing->pTail   = pRing->pBuffer;
    pRing->size    = size;
    pRing->items   = 0;
}

//--------------------------------------------
// Очистка буфера
void		ring_buffer_flush(RING_BUFFER_DATA *pRing )
{
	BeginCriticalSection();
    pRing->pHead   = pRing->pBuffer;
    pRing->pTail   = pRing->pBuffer;
    pRing->items   = 0;
	EndCriticalSection();
}

//------------------------------------------------
// свободно байт в буфере
BSIZE		ring_buffer_get_free( RING_BUFFER_DATA *pRing )
{
	BSIZE ret;
	
	BeginCriticalSection();
	
	ret = (pRing->size - pRing->items);
	
	EndCriticalSection();

    return ret;
}


//-----------------------------------------------
// занято байт в буфере
BSIZE		ring_buffer_get_used( RING_BUFFER_DATA *pRing )
{
    return pRing->items;
}

//----------------------------------------------------
//
BSIZE		ring_buffer_write( RING_BUFFER_DATA *pRing, UNIT *pSrc, BSIZE items )
{
    BSIZE free;
    BSIZE ret;
    
    free = ring_buffer_get_free( pRing );
    
    // записываем столько, сколько свободного места
    if( items > free ) items = free;
    ret = items;
    
	BeginCriticalSection();
	
    if( (pRing->pHead + items) > pRing->pTop )
    { // запись в 2 приёма, тк переход через начало
        free = pRing->pTop - pRing->pHead; // байт в первом этапе
        items -= free;  // остаётся для второго этапа
        
        // первый этап
        while( free )
        {
            *pRing->pHead = *pSrc;
            
            pRing->pHead++;
            pSrc++;
            free--;
        }
        
        // второй этап
        pRing->pHead = pRing->pBuffer;
        free = items;
        
        while( free )
        {
            *pRing->pHead = *pSrc;
            
            pRing->pHead++;
            pSrc++;
            free--;
        }
    }
    else
    { // запись в 1 приём
        free = items;
        while( free )
        {
            *pRing->pHead = *pSrc;
            
            pRing->pHead++;
            pSrc++;
            free--;
        }
    }
    
    // увеличили счетчик байт в буфере
    pRing->items += ret;
    
	EndCriticalSection();
	
    return ret;
}

//----------------------------------------------------
//
BSIZE		ring_buffer_read( RING_BUFFER_DATA *pRing, UNIT *pDst, BSIZE items )
{
    BSIZE used;
    BSIZE ret;
    
    used = ring_buffer_get_used(pRing);
    
    // читаем не больше, чем есть в буфере
    if(items > used) items = used;
    ret = items;
    
	BeginCriticalSection();
	
    if( (pRing->pTail + items) > pRing->pTop )
    { // читаем в 2 приема, переход через начало
        used = ( pRing->pTop - pRing->pTail );
		items -= used; //(!!! BUG BUG BUG) Здесь должен вычисляться остаток!!!
		
        while( used )
        {
            *pDst = *pRing->pTail;
            pDst++;
            pRing->pTail++;
            used--;
        }
        
        pRing->pTail = pRing->pBuffer;
        used = items;		// Остаток
		
        while( used )
        {
            *pDst = *pRing->pTail;
            pDst++;
            pRing->pTail++;
            used--;
        }
    }
    else
    {
        used = items;
        while( used )
        {
            *pDst = *pRing->pTail;
            pDst++;
            pRing->pTail++;
            used--;
        }
    }
    
    // Уменьшили количество байт в буфере на прочитанное
    pRing->items -= ret;
	
	EndCriticalSection();
	
    return ret;
}




//-----------------------------------------
// Механизм семафора для кольцевого буфера

#ifdef      RING_BUFFER_USE_LOCK
//---------------------------------------------------
// Возвращает 0 если удачно
// иначе не 0
UNIT   ring_buffer_lock(RING_BUFFER_DATA *pRing )
{
    if( pRing->mutex != RING_BUFFER_FREE ) return 1;
    
    __disable_interrupt();
    
    pRing->mutex = RING_BUFFER_LOCKED;
    return 0;
}

void            ring_buffer_unlock(RING_BUFFER_DATA *pRing )
{
    pRing->mutex   = RING_BUFFER_FREE;
    
    __enable_interrupt();
}
#endif

//------------------------------------------------------------------------------