/*
    ���������� ��������� �������
*/

#ifndef     __RING_BUFFER__
#define     __RING_BUFFER__		1

#ifndef		U8
#define		U8		BYTE
#endif

#ifndef		U32
#define		U32	DWORD
#endif

// ��������� ����� �������� � ���� �������
// ��� ���������� �������������� �������
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
    U8       *pBuffer;   // ����� ������ ������
    U8       *pTop;      // ����� ������� ������� ������
    U8       *pHead;     // ������
    U8       *pTail;     // ������
    U32      size;       // ������ ������ � ������
    U32      items;      // ���������� ���� � ������
#ifdef      RING_BUFFER_USE_LOCK
    RING_BUFFER_LOCK_STATUS mutex;
#endif    
}RING_BUFFER_DATA;

extern void		ring_buffer_init( void *pBuffer, U32 size );
extern U32		ring_buffer_get_free();
extern U32		ring_buffer_get_used();
extern U32		ring_buffer_write( U8 *pSrc, U32 items );
extern U32		ring_buffer_read( U8 *pDst, U32 items );
extern void     ring_buffer_flush();

#ifdef      RING_BUFFER_USE_LOCK
extern U8    ring_buffer_lock();
extern void             ring_buffer_unlock();
#endif

#endif
//------------------------------------------------------------------------------
