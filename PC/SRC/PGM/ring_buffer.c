/*
    ����������� ��������� �������
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
// ������� ������
void		ring_buffer_flush()
{
    sRbData.pHead   = sRbData.pBuffer;
    sRbData.pTail   = sRbData.pBuffer;
    sRbData.items   = 0;
}

//------------------------------------------------
// �������� ���� � ������
UINT		ring_buffer_get_free()
{
    return (sRbData.size - sRbData.items);
}


//-----------------------------------------------
// ������ ���� � ������
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
    
    // ���������� �������, ������� ���������� �����
    if( items > free ) items = free;
    ret = items;
    
    if( (sRbData.pHead + items) > sRbData.pTop )
    { // ������ � 2 �����, �� ������� ������ ������
        free = sRbData.pTop - sRbData.pHead; // ���� � ������ �����
        items -= free;  // ������� ��� ������� �����
        
        // ������ ����
        while( free )
        {
            *sRbData.pHead = *pSrc;
            
            sRbData.pHead++;
            pSrc++;
            free--;
        }
        
        // ������ ����
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
    { // ������ � 1 ����
        free = items;
        while( free )
        {
            *sRbData.pHead = *pSrc;
            
            sRbData.pHead++;
            pSrc++;
            free--;
        }
    }
    
    // ��������� ������� ���� � ������
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
    
    // ������ �� ������, ��� ���� � ������
    if(items > used) items = used;
    ret = items;
    
    if( (sRbData.pTail + items) > sRbData.pTop )
    { // ������ � 2 ������, ������� ����� ������
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
    
    // ��������� ���������� ���� � ������ �� �����������
    sRbData.items -= ret;
    return ret;
}




//-----------------------------------------
// �������� �������� ��� ���������� ������

#ifdef      RING_BUFFER_USE_LOCK
//---------------------------------------------------
// ���������� 0 ���� ������
// ����� �� 0
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