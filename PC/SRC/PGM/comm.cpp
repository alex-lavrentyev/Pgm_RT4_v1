/*
	Communication function

				! ! ! ! ! ! ! !
	!!!!! Не OVERLAPPED обмен не идёт !!!!!
				! ! ! ! ! ! ! ! 
*/
/*
	Communication function
*/

#include "stdafx.h"
#include "comm.h"
#include "ring_buffer.h"

// Для имён портов больше COM9 использовать префикс
CString FilePrefix = _T("\\\\.\\");

static HANDLE hSerial = INVALID_HANDLE_VALUE;
DCB dcb;	// Device control block

HANDLE	hThread = NULL;
DWORD	RxThreadID;
static COMMTIMEOUTS to;

static OVERLAPPED rxo, txo;

BYTE RxBuffer[COMM_RING_BUFFER_SIZE];


int		OpenCommPort( CString name );
void	CloseCommPort();
int		SendComm( BYTE *pBuf, DWORD size );

/*
	Return 0, if OK
*/
DWORD WINAPI RxThread( LPVOID lpEvents );

int		OpenCommPort( CString name )
{
	CString NamePrefix;

	// Если есть открытый - закрываем
	if( hSerial != NULL ) CloseCommPort();

	NamePrefix = FilePrefix + name;
	
	// Получаем доступ
	hSerial = CreateFile( NamePrefix.LockBuffer(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL );
	NamePrefix.UnlockBuffer();

	if( hSerial == INVALID_HANDLE_VALUE ) return 1; // Error

	// Если успешно, то кофигурируем
	GetCommState( hSerial, &dcb );
	dcb.BaudRate	= CBR_115200;
	dcb.ByteSize	= 8;
	dcb.fBinary		= TRUE;
	dcb.fParity		= NOPARITY;
	dcb.StopBits	= TWOSTOPBITS;
	SetCommState( hSerial, &dcb );

	// Требуемые размеры буфера приёма-передачи
	SetupComm( hSerial, 2048, 2048 );

	GetCommTimeouts( hSerial, &to );

	to.WriteTotalTimeoutMultiplier = 1;
	to.WriteTotalTimeoutConstant = 50;
	//SetCommTimeouts( hSerial, &to );

	PurgeComm( hSerial, PURGE_RXCLEAR|PURGE_TXCLEAR );

	// Маска событий
	SetCommMask( hSerial, EV_RXCHAR );

	// Создаём отдельный поток для приёма
	hThread = CreateThread( NULL, 0, RxThread, NULL, 0, &RxThreadID );
	if( hThread == NULL )
	{
		CloseCommPort();
		return 1; // Error
	}

	ring_buffer_init( RxBuffer, COMM_RING_BUFFER_SIZE );
	ring_buffer_flush();


	//unsigned char tmp = 0x55;
	//DWORD dwBytesWritten;

	txo.hEvent = 0;
	txo.Offset = 0;
	txo.OffsetHigh = 0;

	rxo.hEvent = 0;
	rxo.Offset = 0;
	rxo.OffsetHigh = 0;

/*
	while( 1 )
	{
		txo.Offset = 0;
		txo.OffsetHigh = 0;

		WriteFile( hSerial, (LPCVOID)&tmp, 1, &dwBytesWritten, &txo );
		GetOverlappedResult( hSerial, &txo, &dwBytesWritten, TRUE );

		Sleep( 10 );
	}
*/
	return 0; // OK
}

void	CloseCommPort()
{
	// Поскольку неясно, как вывести поток из ожидания приёма,
	// прибиваем поток принудительно :((((
	if( hThread != NULL )
	{
		TerminateThread( hThread, 0 );
		CloseHandle( hThread );
		hThread = NULL;
	}

	// Закрываем порт
	if( hSerial != INVALID_HANDLE_VALUE )
	{
		CloseHandle( hSerial );
		hSerial = INVALID_HANDLE_VALUE;

	}

	ring_buffer_flush();
}


/*
	Send size bytes from pBuf over communication port hSerial
*/
int		SendComm( BYTE *pBuf, DWORD size )
{
	DWORD dwBytesWritten;

	if( hSerial == INVALID_HANDLE_VALUE ) return 1;

	if( WriteFile( hSerial, (LPCVOID)pBuf, size, &dwBytesWritten, &txo ) == 0 ) return 1; // error
	GetOverlappedResult( hSerial, &txo, &dwBytesWritten, TRUE );

	if( dwBytesWritten != size ) return 1; // error

	return 0; // OK
}


/*
	Отдельный поток приёма данных
*/
DWORD WINAPI RxThread( LPVOID lpEvents )
{
	DWORD dwMask, dwError;
	COMSTAT status;
	BYTE tmp[COMM_TMP_BUFFER_SIZE];

	while(1)
	{
		WaitCommEvent( hSerial, &dwMask, NULL );
		if( dwMask & EV_RXCHAR )
		{ // Income bytes
			ClearCommError( hSerial, &dwError, &status );
			if( status.cbInQue != 0 )
			{
				ReadFile( hSerial, tmp, status.cbInQue, &dwMask, &rxo ); // прочитали
				GetOverlappedResult( hSerial, &rxo, &dwMask, FALSE ); // читаем, что есть - не ждём!!!

				ring_buffer_write( tmp, status.cbInQue ); // сохранили
			}
		}
	}
}