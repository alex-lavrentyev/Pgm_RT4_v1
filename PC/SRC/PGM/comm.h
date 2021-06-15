/*
*/

#ifndef	__COMM_H__
#define	__COMM_H__


#define			COMM_TMP_BUFFER_SIZE		256
#define			COMM_RING_BUFFER_SIZE		2048

extern int		OpenCommPort( CString name );
extern void		CloseCommPort();
extern int		SendComm( BYTE *pBuf, DWORD size );


#endif
