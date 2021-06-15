/*
    ����������� ��������� �������
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
// ����������� ������
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
// ������� ������
void		ring_buffer_flush(RING_BUFFER_DATA *pRing )
{
	BeginCriticalSection();
    pRing->pHead   = pRing->pBuffer;
    pRing->pTail   = pRing->pBuffer;
    pRing->items   = 0;
	EndCriticalSection();
}

//------------------------------------------------
// �������� ���� � ������
BSIZE		ring_buffer_get_free( RING_BUFFER_DATA *pRing )
{
	BSIZE ret;
	
	BeginCriticalSection();
	
	ret = (pRing->size - pRing->items);
	
	EndCriticalSection();

    return ret;
}


//-----------------------------------------------
// ������ ���� � ������
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
    
    // ���������� �������, ������� ���������� �����
    if( items > free ) items = free;
    ret = items;
    
	BeginCriticalSection();
	
    if( (pRing->pHead + items) > pRing->pTop )
    { // ������ � 2 �����, �� ������� ����� ������
        free = pRing->pTop - pRing->pHead; // ���� � ������ �����
        items -= free;  // ������� ��� ������� �����
        
        // ������ ����
        while( free )
        {
            *pRing->pHead = *pSrc;
            
            pRing->pHead++;
            pSrc++;
            free--;
        }
        
        // ������ ����
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
    { // ������ � 1 ����
        free = items;
        while( free )
        {
            *pRing->pHead = *pSrc;
            
            pRing->pHead++;
            pSrc++;
            free--;
        }
    }
    
    // ��������� ������� ���� � ������
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
    
    // ������ �� ������, ��� ���� � ������
    if(items > used) items = used;
    ret = items;
    
	BeginCriticalSection();
	
    if( (pRing->pTail + items) > pRing->pTop )
    { // ������ � 2 ������, ������� ����� ������
        used = ( pRing->pTop - pRing->pTail );
		items -= used; //(!!! BUG BUG BUG) ����� ������ ����������� �������!!!
		
        while( used )
        {
            *pDst = *pRing->pTail;
            pDst++;
            pRing->pTail++;
            used--;
        }
        
        pRing->pTail = pRing->pBuffer;
        used = items;		// �������
		
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
    
    // ��������� ���������� ���� � ������ �� �����������
    pRing->items -= ret;
	
	EndCriticalSection();
	
    return ret;
}




//-----------------------------------------
// �������� �������� ��� ���������� ������

#ifdef      RING_BUFFER_USE_LOCK
//---------------------------------------------------
// ���������� 0 ���� ������
// ����� �� 0
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