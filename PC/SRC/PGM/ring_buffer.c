/*
    Упрапвление кольцевым буфером
*/

#include "stdafx.h"
#include "ring_buffer.h"


RING_BUFFER_DATA    sRbData;

void        ring_buffer_init( void *pBuffer, UINT size );
UINT		ring_buffer_get_free();
UINT		ring_buffer_get_used();
UINT		ring_buffer_write( U8 *pSrc, UINT items );
UINT		ring_buffer_read( U8 *pDst, UINT items );
void        ring_buffer_flush();

#ifdef      RING_BUFFER_USE_LOCK
U8			ring_buffer_lock();
void        ring_buffer_unlock();
#endif


//------------------------------------------
//
void		ring_buffer_init( void *pBuffer, UINT size )
{
    sRbData.pBuffer = (U8 *)pBuffer;
    sRbData.pTop    = sRbData.pBuffer + size;
    sRbData.pHead   = sRbData.pBuffer;
    sRbData.pTail   = sRbData.pBuffer;
    sRbData.size    = size;
    sRbData.items   = 0;
}

//--------------------------------------------
// Очистка буфера
void		ring_buffer_flush()
{
    sRbData.pHead   = sRbData.pBuffer;
    sRbData.pTail   = sRbData.pBuffer;
    sRbData.items   = 0;
}

//------------------------------------------------
// свободно байт в буфере
UINT		ring_buffer_get_free()
{
    return (sRbData.size - sRbData.items);
}


//-----------------------------------------------
// занято байт в буфере
UINT		ring_buffer_get_used()
{
    return sRbData.items;
}

//----------------------------------------------------
//
UINT		ring_buffer_write( U8 *pSrc, UINT items )
{
    UINT free;
    UINT ret;
    
    free = ring_buffer_get_free();
    
    // записываем столько, сколько свободного места
    if( items > free ) items = free;
    ret = items;
    
    if( (sRbData.pHead + items) > sRbData.pTop )
    { // запись в 2 приёма, тк переход черепз начало
        free = sRbData.pTop - sRbData.pHead; // байт в первом этапе
        items -= free;  // остаётся для второго этапа
        
        // первый этап
        while( free )
        {
            *sRbData.pHead = *pSrc;
            
            sRbData.pHead++;
            pSrc++;
            free--;
        }
        
        // второй этап
        sRbData.pHead = sRbData.pBuffer;
        free = items;
        
        while( free )
        {
            *sRbData.pHead = *pSrc;
            
            sRbData.pHead++;
            pSrc++;
            free--;
        }
    }
    else
    { // запись в 1 приём
        free = items;
        while( free )
        {
            *sRbData.pHead = *pSrc;
            
            sRbData.pHead++;
            pSrc++;
            free--;
        }
    }
    
    // увеличили счетчик байт в буфере
    sRbData.items += ret;
    
    return ret;
}

//----------------------------------------------------
//
UINT		ring_buffer_read( U8 *pDst, UINT items )
{
    UINT used;
    UINT ret;
    
    used = ring_buffer_get_used();
    
    // читаем не больше, чем есть в буфере
    if(items > used) items = used;
    ret = items;
    
    if( (sRbData.pTail + items) > sRbData.pTop )
    { // читаем в 2 приема, переход через начало
        used = ( sRbData.pTop - sRbData.pTail );
        while( used )
        {
            *pDst = *sRbData.pTail;
            pDst++;
            sRbData.pTail++;
            used--;
        }
        
        sRbData.pTail = sRbData.pBuffer;
        used = items - used;
        while( used )
        {
            *pDst = *sRbData.pTail;
            pDst++;
            sRbData.pTail++;
            used--;
        }
    }
    else
    {
        used = items;
        while( used )
        {
            *pDst = *sRbData.pTail;
            pDst++;
            sRbData.pTail++;
            used--;
        }
    }
    
    // Уменьшили количество байт в буфере на прочитанное
    sRbData.items -= ret;
    return ret;
}




//-----------------------------------------
// Механизм семафора для кольцевого буфера

#ifdef      RING_BUFFER_USE_LOCK
//---------------------------------------------------
// Возвращает 0 если удачно
// иначе не 0
U8   ring_buffer_lock()
{
    if( sRbData.mutex != RING_BUFFER_FREE ) return 1;
    
    __disable_interrupt();
    
    sRbData.mutex = RING_BUFFER_LOCKED;
    return 0;
}

void            ring_buffer_unlock()
{
    sRbData.mutex   = RING_BUFFER_FREE;
    
    __enable_interrupt();
}
#endif

//------------------------------------------------------------------------------