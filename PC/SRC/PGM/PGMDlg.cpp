// PGMDlg.cpp : файл реализации
//

#include "stdafx.h"
#include "PGM.h"
#include "PGMDlg.h"

#include "comm.h"
#include "protocol.h"
#include "ring_buffer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define		TIMER_1					1
#define		TOTAL_DEV_NUM			9

#define		OP_TIMEOUT				(5 * 100)	 // 5Sec
#define		LONG_TIMEOUT			(101 * 100)   // 101 cек, согласно времянок РТ5

typedef struct
{
	int			num;		// позиция в списке
	CString		name;		// название
	int			size;		// замер в байтах
	unsigned char empty;	// содержимое байта после очистки
}DEVICE_ITEM;

//#define			K155RE3			0
#define			K556RT4			0
#define			K556RT5			1
#define			I2716			2
#define			I2732			3
#define			I2764			4
#define			I27128			5
#define			I27256			6
#define			AT28C16			7
#define			AT28C64			8

// June 2015



DEVICE_ITEM dev[TOTAL_DEV_NUM] = { \
	/*{K155RE3, _T("K155RE3"),					32	0xFF}, \*/
	{K556RT4,	_T("K556RT4"),					256, 0x00}, \
	{K556RT5,	_T("K556RT5"),					512, 0xFF}, \
	{I2716,		_T("i2716 (UV 25V 2Kx8)"),		2048, 0xFF}, \
	{I2732,		_T("i2732 (UV 21V or 25V 4Kx8)"),	4096, 0xFF}, \
	{I2764,		_T("i2764 (UV 12V 8Kx8)"),		8192, 0xFF}, \
	{I27128,	_T("i27128 (UV 12V 16Kx8)"),	16384, 0xFF}, \
	{I27256,	_T("i27256 (UV 12V 32Kx8)"),	32768, 0xFF}, \
	{AT28C16,	_T("AT28C16 (FL 2Kx8)"),		2048, 0xFF}, \
	{AT28C64,	_T("AT28C64 (FL 8Kx8)"),		8192, 0xFF} \
};

CString subkey = _T("HARDWARE\\DEVICEMAP\\SERIALCOMM");
HKEY hlm = HKEY_LOCAL_MACHINE;

#define		MAX_PORT_ITEMS		16
#define		MAX_PORT_NAME_LEN	128
int		PortItems = 0;	// количество портов
WCHAR	PortNames[MAX_PORT_ITEMS][MAX_PORT_NAME_LEN];	// Имена портов
BYTE	Uart_Tx_Buffer[16];		// для передачи команд, они короткие


// Для хранения данных для программирования мс
unsigned char FileBuffer[MAX_BIN_FILE_SIZE];
unsigned int  FileSize = 0;

/*
	Данные  для потока работы с ПЗУ
*/

typedef enum
{
	IDLE = 0,				// простой
	CHECK_RESPONSE,		// ожидание ответа наличия платы
	CHECK_CONNECT,		// Ожидание соединения
	READ_TO_BUFFER,		// Read data to buffer
	WRITE_TO_ROM,		// Write from buffer to ROM
	COMPARE_ROM_BUFFER, // Compare ROM device vs Buffer
	BLANK_CHECK,		// Проверка ПЗУ
	WAIT_COMPLETE		// ожидание кода завершения операции
}STATE_MACHINE;

STATE_MACHINE GlobalState = IDLE;
DWORD WINAPI ThreadFSM( LPVOID lpEvents );


typedef struct
{
	unsigned short int	address;	// текущий адрес ПЗУ
	unsigned short int  len;		// сколько байт читать/писать
	unsigned char op_dev;			// Тип ПЗУ
	unsigned char op_code;			// Код операции для передачи программатору
	unsigned int timeout;			// Таймаут операции в тиках задачи 
	bool		rq;					// true - посылка кода операции, false - ждём ответа
	//STATE_MACHINE current;			// Текущая операция
	//STATE_MACHINE next;				// Следующая операция
}JOB_DATA;
JOB_DATA Jdata;
unsigned int	wd_timer;

// Диалоговое окно CAboutDlg используется для описания сведений о приложении

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Данные диалогового окна
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // поддержка DDX/DDV

// Реализация
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// диалоговое окно CPGMDlg




CPGMDlg::CPGMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPGMDlg::IDD, pParent)
	//, isOpen(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPGMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUF_DAT, DataView);
	DDX_Control(pDX, IDC_CONN_PORT, ConnPort);
	DDX_Control(pDX, IDC_DEV_SEL, ROMSelector);
	DDX_Control(pDX, IDC_OP_TYPE, OpMode);
	DDX_Control(pDX, IDC_PROGRESS1, m_hProgress);
}

BEGIN_MESSAGE_MAP(CPGMDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
ON_WM_TIMER()
ON_WM_CLOSE()
ON_BN_CLICKED(IDCANCEL, &CPGMDlg::OnBnClickedCancel)
ON_BN_CLICKED(IDOK, &CPGMDlg::OnBnClickedOk)
ON_COMMAND(ID_FILE_READTOBUFFER, &CPGMDlg::OnFileReadtobuffer)
ON_COMMAND(ID_TOOLS_BINTOTEXT, &CPGMDlg::OnToolsBintotext)
ON_COMMAND(ID_TOOLS_TEXTTOBIN, &CPGMDlg::OnToolsTexttobin)
ON_COMMAND(ID_FILE_WRITEFROMBUFFER, &CPGMDlg::OnFileWritefrombuffer)
ON_COMMAND(ID_BOARD_READ, &CPGMDlg::OnBoardRead)
ON_CBN_SELENDOK(IDC_CONN_PORT, &CPGMDlg::OnCbnSelendokConnPort)
ON_COMMAND(ID_FILE_COMPARETOBUFFER, &CPGMDlg::OnFileComparetobuffer)
ON_COMMAND(ID_DEVICE_COMPARETOBUFFER, &CPGMDlg::OnDeviceComparetobuffer)
ON_COMMAND(ID_BOARD_BURNBUFFER, &CPGMDlg::OnBoardBurnbuffer)
ON_COMMAND(ID_DEVICE_ERASE, &CPGMDlg::OnDeviceErase)
ON_COMMAND(ID_DEVICE_BLANKCHECK, &CPGMDlg::OnDeviceBlankcheck)
ON_COMMAND(ID_TOOLS_RESCANSERIALPORTS, &CPGMDlg::OnToolsRescanserialports)
ON_COMMAND(ID_DEVICE_RESTARTPROGRAMMATOR, &CPGMDlg::OnDeviceRestartprogrammator)
ON_COMMAND(ID_DEVICE_RUNADDRESSCOUNT, &CPGMDlg::OnDeviceRunaddresscount)
END_MESSAGE_MAP()



void CPGMDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}


// обработчики сообщений CPGMDlg

BOOL CPGMDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Добавление пункта ''О программе...'' в системное меню.

	// IDM_ABOUTBOX должен быть в пределах системной команды.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Задает значок для этого диалогового окна. Среда делает это автоматически,
	//  если главное окно приложения не является диалоговым
	SetIcon(m_hIcon, TRUE);			// Крупный значок
	SetIcon(m_hIcon, FALSE);		// Мелкий значок

	// TODO: добавьте дополнительную инициализацию
	int i;
	SetTimer( TIMER_1, USER_TIMER_MINIMUM, NULL ); // 100ms timer start
	//Menu.LoadMenu( IDR_MAIN_MENU );
	//SetMenu( &Menu );

	FileSize = 0;
	isOpen = false;

	/////////////////////////////////////////////////////////////////
	// Разметка буфера
	LV_COLUMN col;
	CString txt;
	CRect r;

	DataView.DeleteAllItems();
	DataView.GetClientRect( &r );

	// Address column
	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;
	col.cchTextMax = 16;
	col.pszText = _T("Address");
	col.cx = r.Width()/5;

	DataView.InsertColumn( 0, &col );

	// 8 bytes column
	col.cx = r.Width()/10;
	for( i = 1; i<9; i++ )
	{
		txt.Format(_T("%02x"), i-1);
		col.pszText = txt.LockBuffer();
		DataView.InsertColumn( i, &col );
		txt.UnlockBuffer();
	}
	/*
	for( i = 0; i<4; i++ )
	{
		DataView.InsertItem( i, _T("Item"));
		DataView.SetItemText( i, 1, _T("SubItem"));
	}
	*/
	/////////////////////////////////////////////////////////////////
	// Доступные для подключения порты
	ConnPort.ResetContent();
	ScanCommPorts();
	if( PortItems )
	{
		for( i=0; i<PortItems; i++ )
		{
			ConnPort.InsertString( i, PortNames[i] );
		}
		ConnPort.SetCurSel( 0 );
	}

	////////////////////////////////////////////////////////////////////
	// ROM devices
	ROMSelector.ResetContent();

	for( i = 0; i<TOTAL_DEV_NUM; i++ )
	{
		ROMSelector.InsertString( dev[i].num, dev[i].name );
	}
	
	ROMSelector.SetCurSel( 0 );

	//DWORD IDStateFSM; // dummy
	//CreateThread( NULL, 0, ThreadFSM, NULL, 0, &IDStateFSM );

	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}


// При добавлении кнопки свертывания в диалоговое окно нужно воспользоваться приведенным ниже кодом,
//  чтобы нарисовать значок. Для приложений MFC, использующих модель документов или представлений,
//  это автоматически выполняется рабочей средой.

void CPGMDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // контекст устройства для рисования

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Выравнивание значка по центру клиентского прямоугольника
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Нарисуйте значок
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// Система вызывает эту функцию для получения отображения курсора при перемещении
//  свернутого окна.
HCURSOR CPGMDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// Сканирование последовательных портов

void CPGMDlg::ScanCommPorts(void)
{
		////////////////////////////////////////////////////////////////////
	// Serial ports
	LONG res;
	HKEY key;
	WCHAR Name[16][128];
	DWORD Size;

	DWORD DSize;
	DWORD Type = REG_SZ;

	int i;

	res = RegOpenKey( hlm, subkey, &key);
	
	// If success read all key values
	if( res == ERROR_SUCCESS )
	{
		i = 0;
		for(;;)
		{
			// Читаем имя переменной реестра
			Size = 128;
			res = RegEnumValue( key, i, Name[i], &Size, NULL, NULL, NULL, NULL );
			if( res != ERROR_SUCCESS ) break; // Last or none

			// Читаем значение переменной реестра
			DSize = 128;
			res = RegQueryValueEx( key, Name[i], NULL, &Type, (LPBYTE)PortNames[i], &DSize );
			if( res != ERROR_SUCCESS ) break;

			i++;
		}
	} // on exit i content number of ports
	
	PortItems = i;
}


void CPGMDlg::FileRead()
{
	return;
}

void CPGMDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: добавьте свой код обработчика сообщений или вызов стандартного

	switch( GlobalState )
	{
		case IDLE: { OpMode.SetWindowTextW(_T("Idle")); OpMode.Invalidate(); break; }
		case CHECK_RESPONSE: { OpMode.SetWindowTextW(_T("Wait responce...")); OpMode.Invalidate(); break; }
		 /*
			Задача чтения ПЗУ
		 */
		case READ_TO_BUFFER:
		{
			ReadToBuffer();
			break;
		}
		case WRITE_TO_ROM:
		{
			BurnFromBuffer();
			break;
		}
		case COMPARE_ROM_BUFFER:
		{
			Compare_vs_Buffer();
			break;
		}
		case BLANK_CHECK:
		{
			BlankCheckROM();
			break;
		}
		default: break;
	}
	CDialog::OnTimer(nIDEvent);
}

void CPGMDlg::OnClose()
{
	// TODO: добавьте свой код обработчика сообщений или вызов стандартного

	CloseCommPort();
	KillTimer( TIMER_1 );

	//if( hStateThread != INVALID_HANDLE_VALUE ) TerminateThread( hStateThread, 0 );

	CDialog::OnClose();

	OnOK();	// Иначе приложение не закроется
}

void CPGMDlg::OnBnClickedCancel()
{
	// TODO: добавьте свой код обработчика уведомлений
	//OnCancel();
}

void CPGMDlg::OnBnClickedOk()
{
	// TODO: добавьте свой код обработчика уведомлений
	//OnOK();
}


/*
	Read binary file to buffer
*/
#define		MAX_BYTES_IN_STRING			8
void CPGMDlg::OnFileReadtobuffer()
{
	// TODO: добавьте свой код обработчика команд
	TCHAR szFilter[] = _T("Binary Files (*.bin)|*.bin|ROM Images (*.rom)|*.rom|All Files (*.*)|*.*||");
	TCHAR Dir[MAX_PATH+1];
	GetCurrentDirectory( MAX_PATH, Dir );

	CString Name;
	CFile fileIn;
	UINT len;
	int colIndex, strIndex;
	int address;
	//int i;
	unsigned char data;

	CFileDialog dlg( TRUE, _T("*.bin"), 0, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, szFilter );
	dlg.GetOFN().lpstrInitialDir = Dir;

	if( dlg.DoModal() == IDOK )
	{
		Name = dlg.GetPathName();
		if( fileIn.Open( Name, CFile::modeRead, NULL ) == 0 )
		{
			MessageBox( _T("Can't open source file!"), _T("Read File"), MB_ICONEXCLAMATION|MB_OK );
			return;
		}

		len = (UINT)fileIn.GetLength();

		if( len > MAX_BIN_FILE_SIZE )
		{
			MessageBox( _T("Source file is too big!"), _T("Read File"), MB_ICONINFORMATION|MB_OK );
			return;
		}
		
		// Fill buffer with BLANK symbols
		memset( FileBuffer, dev[Jdata.op_dev].empty, MAX_BIN_FILE_SIZE );

		fileIn.SeekToBegin();
		fileIn.Read( FileBuffer, len );
		FileSize = len;

		fileIn.SeekToBegin();
		colIndex = 0;
		strIndex = 0;
		address = 0;

		// Fill header
		CString txt;
		CRect r;

		DataView.DeleteAllItems(); // Колонки остаются при очистке


		// Fill cell
		//LV_ITEM item;
		//item.mask = LVIF_TEXT;

		while( len )
		{
			// Add string
			if( colIndex % (MAX_BYTES_IN_STRING+1) == 0 )
			{
				txt.Format(_T("%04X"), address);
				DataView.InsertItem( strIndex, txt.LockBuffer() );
				txt.UnlockBuffer();
				strIndex++;
				colIndex = 1;
			}

			// Add columns
			fileIn.Read( &data, 1);

			txt.Format(_T("%02X"), data);
			DataView.SetItemText( (strIndex-1), colIndex, txt.LockBuffer() );
			txt.UnlockBuffer();

			colIndex++;
			address++;

			//
			len--;
		}

		fileIn.Close();
		DataView.Invalidate(TRUE);
	} //dlg = IDOK
}

/*
	Convert binary to text file
*/
void CPGMDlg::OnToolsBintotext()
{
	// TODO: добавьте свой код обработчика команд
	TCHAR szFilter[] = _T("BIN Files (*.bin)|*.bin|ROM Files (*.rom)|*.rom|All Files (*.*)|*.*||"); // TXT extension only !!!
	TCHAR Dir[MAX_PATH+1];
	CString Name;
	CString OutName;
	CFile In, Out;
	ULONGLONG SrcLen;
	UCHAR inData;
	UINT i;
	unsigned char CR_LF[2] = {0x0D, 0x0A};

	GetCurrentDirectory( MAX_PATH, Dir );

	CFileDialog dlg( TRUE, _T("*.bin"), 0, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, szFilter );
	dlg.GetOFN().lpstrInitialDir = Dir;

	if( dlg.DoModal() == IDOK )
	{
		char buffer[10];


		Name = dlg.GetPathName(); //dlg.GetFileName();
		OutName = dlg.GetPathName();
		OutName.Truncate( OutName.GetLength() - 3 );
		OutName += _T("txt");
		
		if( In.Open( Name, CFile::modeRead, NULL) == 0 )
		{
			MessageBox( _T("Can't open source file!"), _T("TEXT to BIN"), MB_ICONEXCLAMATION|MB_OK);
			return;
		}

		if( Out.Open( OutName, (CFile::modeWrite | CFile::modeCreate), NULL ) == 0 )
		{
			MessageBox( _T("Cant't open destination file!"), _T("TEXT to BIN"), MB_ICONEXCLAMATION|MB_OK);
			In.Close();
			return;
		}

		SrcLen = In.GetLength();

		// For any case
		In.SeekToBegin();
		Out.SeekToBegin();
		i = 0;

		while( SrcLen )
		{
			In.Read( &inData, 1 );
			
			sprintf_s( buffer, "%02X", inData );

			Out.Write( buffer, 2 );

			SrcLen--;
			i++;

			if( (i % BYTES_IN_ROW) == 0 && (i!=0) ) Out.Write( CR_LF, 2 );
		}
		In.Close();
		Out.Close();

		MessageBox(_T("Done!"), _T("BIN to TEXT"), MB_ICONINFORMATION|MB_OK );
	} // CFileDlg == OK
}

/*
	Convert raw text to binary
	Open *.txt file then convert & save *.bin file, name the same as .txt file
	During convert, ignored control CR LF etc symbols
	If exist not 0 - F symbols, then ERROR
*/
void CPGMDlg::OnToolsTexttobin()
{
	// TODO: добавьте свой код обработчика команд
//	TCHAR szFilter[] = _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||");
	TCHAR szFilter[] = _T("Text Files (*.txt)|*.txt||"); // TXT extension only !!!
	TCHAR Dir[MAX_PATH+1];
	CString Name;
	CString OutName;
	CFile In, Out;
	ULONGLONG SrcLen;
	CHAR inData, outData;
	bool Order = false;

	GetCurrentDirectory( MAX_PATH, Dir );

	CFileDialog dlg( TRUE, _T("*.txt"), 0, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, szFilter );
	dlg.GetOFN().lpstrInitialDir = Dir;

	if( dlg.DoModal() == IDOK )
	{
		Name = dlg.GetPathName(); //dlg.GetFileName();
		OutName = dlg.GetPathName();
		OutName.Truncate( OutName.GetLength() - 3 );
		OutName += _T("bin");
		
		if( In.Open( Name, CFile::modeRead, NULL) == 0 )
		{
			MessageBox( _T("Can't open source file!"), _T("TEXT to BIN"), MB_ICONEXCLAMATION|MB_OK);
			return;
		}

		if( Out.Open( OutName, (CFile::modeWrite | CFile::modeCreate), NULL ) == 0 )
		{
			MessageBox( _T("Cant't open destination file!"), _T("TEXT to BIN"), MB_ICONEXCLAMATION|MB_OK);
			In.Close();
			return;
		}

		SrcLen = In.GetLength();

		// For any case
		In.SeekToBegin();
		Out.SeekToBegin();
		Order = false;

		while( SrcLen )
		{
			In.Read( &inData, 1 );
			
			if( IsDigit( inData ) == true )
			{
				if( Order == false )
				{
					outData = Char2Byte( inData ) << 4;
					Order = true;
				}
				else
				{
					outData |= Char2Byte( inData );
					Out.Write( &outData, 1 );
					Order = false;
				}
			}

			SrcLen--;
		}
		In.Close();
		Out.Close();
	} // CFileDlg == OK
}

char CPGMDlg::Char2Byte(char data)
{
	switch( data )
	{
		case '0': return 0;
		case '1' : return 1;
		case '2' : return 2;
		case '3' : return 3;
		case '4' : return 4;
		case '5' : return 5;
		case '6' : return 6;
		case '7' : return 7;
		case '8' : return 8;
		case '9' : return 9;
		case 'A' : return 0x0A;
		case 'a' : return 0x0A;
		case 'B' : return 0x0B;
		case 'b' : return 0x0B;
		case 'C' : return 0x0C;
		case 'c' : return 0x0C;
		case 'D' : return 0x0D;
		case 'd' : return 0x0D;
		case 'E' : return 0x0E;
		case 'e' : return 0x0E;
		case 'F' : return 0x0F;
		case 'f' : return 0x0F;
		default: return 0;
	}
}

bool CPGMDlg::IsDigit(char data)
{
	if( data >= '0' && data <= '9' ) return true;
	if( data >= 'A' && data <= 'F' ) return true;
	if( data >= 'a' && data <= 'f' ) return true;

	return false;
}

/*
	Запись буфера в файл
	Размер файла будет в соответствии с ёмкостью выбранного устройства
*/
void CPGMDlg::OnFileWritefrombuffer()
{
	// TODO: добавьте свой код обработчика команд
	TCHAR szFilter[] = _T("BIN Files (*.bin)|*.bin||"); // TXT extension only !!!
	TCHAR Dir[MAX_PATH+1];
	CString Name;
	CFile fileOut;
	UINT Len;
	int sel;

	GetCurrentDirectory( MAX_PATH, Dir );

	CFileDialog dlg( FALSE, _T("*.bin"), 0, 0, szFilter );
	dlg.GetOFN().lpstrInitialDir = Dir;

	if( dlg.DoModal() == IDOK )
	{
		Name = dlg.GetPathName();

		// Check file exist
		if( fileOut.Open( Name, CFile::modeRead, NULL ) != 0 )
		{ // Open success -> Exist ...
			if( MessageBox( _T("Overwrite selected file?"), _T("File already exist!"), MB_ICONQUESTION|MB_YESNO ) == IDNO )
			{
				fileOut.Close();
				return; // user break
			}
		}

		//Open file for write
		if( fileOut.Open( Name, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary, NULL ) == 0 )
		{
			MessageBox( _T("Can't create file to write!"), _T("File error!"), MB_ICONEXCLAMATION|MB_OK );
			return;
		}

		sel = ROMSelector.GetCurSel();
		Len = dev[sel].size;
		
		fileOut.Write( FileBuffer, Len );


		MessageBox( _T("File write done!"), dlg.GetFileName(), MB_ICONEXCLAMATION|MB_OK );

		fileOut.Close();
	}
}



/*
	Читаем ПЗУ
*/
void CPGMDlg::OnBoardRead()
{
	// TODO: добавьте свой код обработчика команд
	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}


	// Заполняем структуру для задачи работы с ПЗУ
	Jdata.address = 0;
	Jdata.op_dev = ROMSelector.GetCurSel();
	Jdata.len = dev[Jdata.op_dev].size;
	Jdata.timeout = 100;	// 10 сек
	Jdata.rq = true;

	switch( Jdata.op_dev )
	{
		case K556RT4: Jdata.op_code = READ_RT4; break;
		case K556RT5: Jdata.op_code = READ_RT5; break;

		case I2716: Jdata.op_code = READ_2716; break;
		case I2732: Jdata.op_code = READ_2764; break;
		case I2764: Jdata.op_code = READ_2764; break;
		case I27128: Jdata.op_code = READ_2764; break;
		case I27256: Jdata.op_code = READ_2764; break;


		case AT28C16: Jdata.op_code = READ_2816; break;
		case AT28C64: Jdata.op_code = READ_2864; break;
		default: MessageBox(_T("Unsupported Device!"), NULL, MB_ICONEXCLAMATION|MB_OK ); return;
	}
	
	m_hProgress.SetRange32( 0, Jdata.len);
	m_hProgress.SetPos( 0 );
	m_hProgress.Invalidate();

	GlobalState = READ_TO_BUFFER;	// Инициация задачи чтения ПЗУ

	DataView.DeleteAllItems();
	DataView.Invalidate();

	OpMode.SetWindowTextW(_T("ROM read...")); 
	OpMode.Invalidate();

	
	//FileBuffer[MAX_BIN_FILE_SIZE];
	memset( FileBuffer, dev[Jdata.op_dev].empty, MAX_BIN_FILE_SIZE );
	FileSize = Jdata.len; // To store

}




// Попытка открыть порт и соединиться с программатором
bool CPGMDlg::TryConnectBoard(void)
{
	CString PortName;
	BYTE c;
	//int i;

	if( isOpen == false )
	{ // Not open, try open
		if( PortItems == 0 )
		{
			MessageBox(_T("No ports find to connect board!"), _T("Operation cancelled"), MB_ICONEXCLAMATION|MB_OK );
			return false;
		}
	
		ConnPort.GetLBText( ConnPort.GetCurSel(), PortName );

		if( OpenCommPort( PortName ) != 0 )
		{
			MessageBox(_T("Can't open port!"), _T("I/O port"), MB_ICONEXCLAMATION|MB_OK );	
			return false;
		}

		// Sure, to connect board
		ring_buffer_flush();

		c = PGM_RESTART;
		SendComm( &c, 1 );
		
		m_hProgress.SetRange32( 0, 199 );
		c = 0;
		do
		{
			if( c < 200 )
			{
				c++;
				m_hProgress.SetPos( c );
				m_hProgress.Invalidate();
			}
			else
			{
				MessageBox(_T("Board not found!"), PortName.LockBuffer(), MB_ICONEXCLAMATION|MB_OK );
				PortName.UnlockBuffer();
				CloseCommPort();
				isOpen = false;
				return false;
			}

			Sleep(10);
		}while( ring_buffer_get_used() == 0 );
		

		ring_buffer_read( &c, 1 );

		if( c == RET_OK )
		{
			ring_buffer_flush();
			isOpen = true; // Порт открыт
			return true;
		}


		MessageBox(_T("Incorrect board responce!"), PortName.LockBuffer(), MB_ICONEXCLAMATION|MB_OK );
		CloseCommPort();
		isOpen = false;

		return false;
	} // isOpen == FASLE
	else
	{ // Already was open, check responce only
		// Sure, to connect board
		ring_buffer_flush();

		c = PGM_RESTART;
		SendComm( &c, 1 );
		
		c = 0;
		m_hProgress.SetRange32( 0, 199 );
		do
		{
			if( c < 200 )
			{
				c++;
				m_hProgress.SetPos( c );
				m_hProgress.Invalidate();
			}
			else
			{
				MessageBox(_T("Board not found!"), PortName.LockBuffer(), MB_ICONEXCLAMATION|MB_OK );
				PortName.UnlockBuffer();
				CloseCommPort();
				isOpen = false;
				return false;
			}

			Sleep(10);
		}while( ring_buffer_get_used() == 0 );
		
		ring_buffer_read( &c, 1 );
		if( c == RET_OK )
		{
			ring_buffer_flush();
			isOpen = true;
			return true;
		}


		MessageBox(_T("Incorrect board responce!"), PortName.LockBuffer(), MB_ICONEXCLAMATION|MB_OK );
		CloseCommPort();
		isOpen = false;

		return false;
	}
	return false;
}

/*
	После смены порта соединение недей
*/
void CPGMDlg::OnCbnSelendokConnPort()
{
	// TODO: добавьте свой код обработчика уведомлений
	if( isOpen == true )
	{
		CloseCommPort();
		isOpen = false;
	}
}



// Процедура чтения ПЗУ
int CPGMDlg::ReadToBuffer(void)
{
			if( Jdata.rq == true )
			{
				if( (Jdata.address % 16 ) == 0 )
				{
					CString str;

					str.Format(_T("ROM read at: 0x%04X-0x%04X" ), Jdata.address, Jdata.address + 0x0F );

					OpMode.SetWindowTextW( str.LockBuffer() );
					OpMode.Invalidate();
					str.UnlockBuffer();
				}

				Jdata.rq = false; // ждём ответа
				ring_buffer_flush();	// Clear income buffer

				Uart_Tx_Buffer[0] = Jdata.op_code;		// 3 bytes total
				Uart_Tx_Buffer[1] = (BYTE)Jdata.address; // low byte
				Uart_Tx_Buffer[2] = (BYTE)(Jdata.address >> 8); // high byte
				SendComm( Uart_Tx_Buffer, 3 );

				wd_timer = OP_TIMEOUT;

			}
			else
			{ // ждём ответа,если не конец и не ошибка, то продолжаем
				if( ring_buffer_get_used() != 0 )
				{
					wd_timer = OP_TIMEOUT;

					BYTE idata;
					ring_buffer_read( &idata, 1 );
					*(FileBuffer + Jdata.address) = idata;

					Jdata.len--;

					if( Jdata.len != 0 )
					{ // Продолжаем чтение
						Jdata.address++;

						m_hProgress.SetPos( Jdata.address );
						m_hProgress.Invalidate();

						Jdata.rq = true;
					}
					else
					{ // Операция полностью завершена
						GlobalState = IDLE;
						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

						// Заполнить табличку просмотра кода
						CString txt;
						int address = 0;
						int strIndex = 0;
						int colIndex = 0;
						int len = dev[Jdata.op_dev].size;

						while( len )
						{
							// Add string
							if( colIndex % (MAX_BYTES_IN_STRING+1) == 0 )
							{
								txt.Format(_T("%04X"), address);
								DataView.InsertItem( strIndex, txt.LockBuffer() );
								txt.UnlockBuffer();
								strIndex++;
								colIndex = 1;
							}


							txt.Format(_T("%02X"), FileBuffer[address]);
							DataView.SetItemText( (strIndex-1), colIndex, txt.LockBuffer() );
							txt.UnlockBuffer();

							colIndex++;
							address++;

							//
							len--;
						} // while ( len )
					} // ring_buffer_get_used() == 0
				} // else Jdata.rq == FALSE
				else
				{
					wd_timer--;
					if( wd_timer == 0 )
					{
						MessageBox(_T("Operation terminated!!!"), _T("Timeout"), MB_ICONEXCLAMATION|MB_OK);
						GlobalState = IDLE;
						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

						return 0;
					}
				}
			} // Case READ_TO_BUFFER

	return 0;
}

/*
	Сравнение файла с содержимым буфера
*/
void CPGMDlg::OnFileComparetobuffer()
{
	// TODO: добавьте свой код обработчика команд
	TCHAR szFilter[] = _T("Binary Files (*.bin)|*.bin|ROM Images (*.rom)|*.rom|All Files (*.*)|*.*||");
	TCHAR Dir[MAX_PATH+1];
	GetCurrentDirectory( MAX_PATH, Dir );

	CString Name;
	CFile fileIn;
	UINT len;
	int address;
	unsigned char data;
	int index;

	index = ROMSelector.GetCurSel();

	CFileDialog dlg( TRUE, _T("*.bin"), 0, OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST, szFilter );
	dlg.GetOFN().lpstrInitialDir = Dir;

	if( dlg.DoModal() == IDOK )
	{
		Name = dlg.GetPathName();
		if( fileIn.Open( Name, CFile::modeRead, NULL ) == 0 )
		{
			MessageBox( _T("Can't open source file!"), _T("Read File"), MB_ICONEXCLAMATION|MB_OK );
			return;
		}

		len = (UINT)fileIn.GetLength();

		if( len != dev[index].size )
		{
			MessageBox( _T("Different size ROM vs FILE!"), _T("Compare with File"), MB_ICONINFORMATION|MB_OK );
			fileIn.Close();
			return;
		}

		// Размер совпал, сравниваем побайтно
		address = 0;
		fileIn.SeekToBegin();

		while( len )
		{
			fileIn.Read( &data, 1 );

			if( FileBuffer[address] != data )
			{
				Name.Format( _T("at address =%04X: Buffer =%02X vs File=%02X"), address, FileBuffer[address], data );
				MessageBox( Name.LockBuffer(), _T("Compare with File"), MB_ICONEXCLAMATION|MB_OK );
				Name.UnlockBuffer();
				fileIn.Close();
				return;
			}

			address++;
			len--;
		}

		MessageBox( _T("Buffer content is equial File!"), _T("Compare with File"), MB_ICONINFORMATION|MB_OK );
		fileIn.Close();
	} // если файл выбран
}

/*
	Сравнение содержимого устройства с содержимым буфера
*/
void CPGMDlg::OnDeviceComparetobuffer()
{
	// TODO: добавьте свой код обработчика команд

	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}
	
	// Заполняем структуру для задачи работы с ПЗУ
	Jdata.address = 0;
	Jdata.op_dev = ROMSelector.GetCurSel();
	Jdata.len = dev[Jdata.op_dev].size;
	Jdata.timeout = 100;	// 10 сек
	Jdata.rq = true;

	switch( Jdata.op_dev )
	{
		case K556RT4: Jdata.op_code = READ_RT4; break;
		case K556RT5: Jdata.op_code = READ_RT5; break;

		case I2716: Jdata.op_code = READ_2716; break;
		case I2732: Jdata.op_code = READ_2764; break;
		case I2764: Jdata.op_code = READ_2764; break;
		case I27128: Jdata.op_code = READ_2764; break;
		case I27256: Jdata.op_code = READ_2764; break;


		case AT28C16: Jdata.op_code = READ_2816; break;
		case AT28C64: Jdata.op_code = READ_2864; break;
		default: MessageBox(_T("Unsupported Device!"), NULL, MB_ICONEXCLAMATION|MB_OK ); return;
	}
	
	m_hProgress.SetRange32( 0, Jdata.len);
	m_hProgress.SetPos( 0 );
	m_hProgress.Invalidate();

	GlobalState = COMPARE_ROM_BUFFER;	// Инициация задачи чтения ПЗУ

	OpMode.SetWindowTextW(_T("Compare...")); 
	OpMode.Invalidate();	
}

void CPGMDlg::OnBoardBurnbuffer()
{
	// TODO: добавьте свой код обработчика команд


	if( MessageBox( _T("Do you want burn ROM device with current buffer content?"), _T("Sure?"), MB_ICONQUESTION|MB_YESNO ) != IDYES ) return;

	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}


	// Заполняем структуру для задачи работы с ПЗУ
	Jdata.address = 0;
	Jdata.op_dev = ROMSelector.GetCurSel();
	Jdata.len = dev[Jdata.op_dev].size;
	Jdata.timeout = 100;	// 10 сек
	Jdata.rq = true;

	switch( Jdata.op_dev )
	{
		case K556RT4: Jdata.op_code = WRITE_RT4; break;
		case K556RT5: Jdata.op_code = WRITE_RT5; break;

		case I2716: Jdata.op_code = WRITE_2716; break;
		case I2732: Jdata.op_code = WRITE_2716; break;
		case I2764: Jdata.op_code = WRITE_2764; break;
		case I27128: Jdata.op_code = WRITE_2764; break;
		case I27256: Jdata.op_code = WRITE_27256; break;		// LAB 15.05.2015 Due to change programming sequence

		case AT28C16: Jdata.op_code = WRITE_2816; break;
		case AT28C64: Jdata.op_code = WRITE_2864; break;
		default: MessageBox(_T("Unsupported Device!"), NULL, MB_ICONEXCLAMATION|MB_OK ); return;
	}
	
	m_hProgress.SetRange32( 0, Jdata.len);
	m_hProgress.SetPos( 0 );
	m_hProgress.Invalidate();

	GlobalState = WRITE_TO_ROM;	// Инициация задачи чтения ПЗУ

	OpMode.SetWindowTextW(_T("ROM write, DO NOT POWER OFF!")); 
	OpMode.Invalidate();	
}

// Запись ПЗУ
// Файл должен быть прочитан в буфер
int CPGMDlg::BurnFromBuffer(void)
{
			if( Jdata.rq == true )
			{

				if( (Jdata.address % 16 ) == 0 )
				{
					CString str;

					str.Format(_T("ROM burn at: 0x%04X-0x%04X" ), Jdata.address, Jdata.address + 0x0F );

					OpMode.SetWindowTextW( str.LockBuffer() );
					OpMode.Invalidate();
					str.UnlockBuffer();
				}

				Jdata.rq = false; // ждём ответа

				ring_buffer_flush();	// Clear income buffer

				Uart_Tx_Buffer[0] = Jdata.op_code;		// 4 bytes total
				Uart_Tx_Buffer[1] = (BYTE)Jdata.address; // low byte
				Uart_Tx_Buffer[2] = (BYTE)(Jdata.address >> 8); // high byte
				Uart_Tx_Buffer[3] = FileBuffer[Jdata.address];
				SendComm( Uart_Tx_Buffer, 4 );

				if( (dev[Jdata.op_dev].num == K556RT4) || (dev[Jdata.op_dev].num == K556RT5) ) wd_timer = LONG_TIMEOUT;
				else wd_timer = OP_TIMEOUT;

			}
			else
			{ // ждём ответа,если не конец и не ошибка, то продолжаем
				if( ring_buffer_get_used() != 0 )
				{
					wd_timer = OP_TIMEOUT;

					BYTE idata;
					ring_buffer_read( &idata, 1 );

					if( idata != RET_OK )
					{
						CString msg;

						GlobalState = IDLE;

						msg.Format( _T("Burn failed at address = %04X"), Jdata.address );
						MessageBox( msg.LockBuffer(), _T("BURN"), MB_ICONSTOP|MB_OK );
						msg.UnlockBuffer();

						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();
						return 0;
					}

					Jdata.len--;

					if( Jdata.len != 0 )
					{ // Продолжаем чтение
						Jdata.address++;

						m_hProgress.SetPos( Jdata.address );
						m_hProgress.Invalidate();

						Jdata.rq = true;
					}
					else
					{ // Операция полностью завершена
						GlobalState = IDLE;
						//CString msg;
						//msg.Format( _T("Burn failed at address = %04X"), Jdata.address );
						MessageBox( _T("Burn process complete!"), _T("BURN"), MB_ICONINFORMATION|MB_OK );
						//msg.UnlockBuffer();


						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

					} // ring_buffer_get_used() == 0
				} // else Jdata.rq == FALSE
				else
				{
					wd_timer--;
					if( wd_timer == 0 )
					{
						MessageBox(_T("Operation terminated!!!"), _T("Timeout"), MB_ICONEXCLAMATION|MB_OK);
						GlobalState = IDLE;
						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

						return 0;
					}
				}
			} // Case READ_TO_BUFFER
	return 0;
}

// Сравнение ПЗУ с буфером
int CPGMDlg::Compare_vs_Buffer(void)
{
			if( Jdata.rq == true )
			{
				if( (Jdata.address % 16 ) == 0 )
				{
					CString str;

					str.Format(_T("ROM compare at: 0x%04X-0x%04X" ), Jdata.address, Jdata.address + 0x0F );

					OpMode.SetWindowTextW( str.LockBuffer() );
					OpMode.Invalidate();
					str.UnlockBuffer();
				}

				Jdata.rq = false; // ждём ответа

				ring_buffer_flush();	// Clear income buffer

				Uart_Tx_Buffer[0] = Jdata.op_code;		// 3 bytes total
				Uart_Tx_Buffer[1] = (BYTE)Jdata.address; // low byte
				Uart_Tx_Buffer[2] = (BYTE)(Jdata.address >> 8); // high byte
				SendComm( Uart_Tx_Buffer, 3 );

				wd_timer = OP_TIMEOUT;

			}
			else
			{ // ждём ответа,если не конец и не ошибка, то продолжаем
				if( ring_buffer_get_used() != 0 )
				{
					wd_timer = OP_TIMEOUT;

					BYTE idata;
					ring_buffer_read( &idata, 1 );

					// Для РТ4 только младшие 4 бита
					if( dev[Jdata.op_dev].num == K556RT4 ) idata = idata & 0x0F;

					if( idata != FileBuffer[Jdata.address] )
					{
						CString msg;

						GlobalState = IDLE;

						msg.Format( _T("Compare failed at address %04X: Buffer = %02X ROM = %02X"), \
							Jdata.address, FileBuffer[Jdata.address], idata );

						MessageBox( msg.LockBuffer(), _T("Compare ROM vs Buffer"), MB_ICONSTOP|MB_OK );
						msg.UnlockBuffer();

						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();
						return 0;
					}

					Jdata.len--;

					if( Jdata.len != 0 )
					{ // Продолжаем чтение
						Jdata.address++;

						m_hProgress.SetPos( Jdata.address );
						m_hProgress.Invalidate();

						Jdata.rq = true;
					}
					else
					{ // Операция полностью завершена
						GlobalState = IDLE;
						//CString msg;
						//msg.Format( _T("Burn failed at address = %04X"), Jdata.address );
						MessageBox( _T("Compare process complete! \n ROM content match Buffer."), _T("Compapre ROM vs Buffer"), MB_ICONINFORMATION|MB_OK );
						//msg.UnlockBuffer();

						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

					} // ring_buffer_get_used() == 0
				} // else Jdata.rq == FALSE
				else
				{
					wd_timer--;
					if( wd_timer == 0 )
					{
						MessageBox(_T("Operation terminated!!!"), _T("Timeout"), MB_ICONEXCLAMATION|MB_OK);
						GlobalState = IDLE;
						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

						return 0;
					}
				}
			} // Case READ_TO_BUFFER
	return 0;
}

void CPGMDlg::OnDeviceErase()
{
	int timeout;

	// TODO: добавьте свой код обработчика команд
	if( MessageBox( _T("Do you really want ERASE ROM device?"), _T("Sure?"), MB_ICONQUESTION|MB_YESNO ) != IDYES ) return;

	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}


	// Заполняем структуру для задачи работы с ПЗУ
	Jdata.address = 0;
	Jdata.op_dev = ROMSelector.GetCurSel();
	Jdata.len = dev[Jdata.op_dev].size;
	Jdata.timeout = 100;	// 10 сек
	Jdata.rq = true;

	// Только EEPROM
	switch( Jdata.op_dev )
	{
		//case K556RT4: Jdata.op_code = WRITE_RT4; break;
		//case K556RT5: Jdata.op_code = WRITE_RT5; break;
		//case I2716: Jdata.op_code = WRITE_2716; break;
		//case I2764: Jdata.op_code = WRITE_2764; break;
		case AT28C16: Jdata.op_code = ERASE_2816; break;
		case AT28C64: Jdata.op_code = ERASE_2864; break;
		default: MessageBox(_T("Unsupported Device!"), NULL, MB_ICONEXCLAMATION|MB_OK ); return;
	}

	ring_buffer_flush();

	Uart_Tx_Buffer[0] = ERASE_2864;		// 3 bytes total
	SendComm( Uart_Tx_Buffer, 1 );

	timeout = 200;
	while( timeout )
	{
		if( ring_buffer_get_used() != 0 )
		{
			BYTE idata;
			ring_buffer_read( &idata, 1 );

			if( idata == RET_OK )
			{
					MessageBox( _T("ERASE process completed!"), _T("ERASE"), MB_ICONINFORMATION|MB_OK );
					return;
			}
			else
			{
					MessageBox( _T("ERASE process failed!"), _T("ERASE"), MB_ICONSTOP|MB_OK );
					return;
			}
		}

		timeout--;
		if( timeout == 0 )
		{
			MessageBox(_T("ERASE process terminated due to timeout!"), _T("ERASE"), MB_ICONEXCLAMATION|MB_OK );
			return;
		}
		Sleep(10);
	}
}

/*
	Проверка ПЗУ 
*/
void CPGMDlg::OnDeviceBlankcheck()
{
	// TODO: добавьте свой код обработчика команд
	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}


	// Заполняем структуру для задачи работы с ПЗУ
	Jdata.address = 0;
	Jdata.op_dev = ROMSelector.GetCurSel();
	Jdata.len = dev[Jdata.op_dev].size;
	Jdata.timeout = 100;	// 10 сек
	Jdata.rq = true;

	switch( Jdata.op_dev )
	{
		case K556RT4: Jdata.op_code = READ_RT4; break;
		case K556RT5: Jdata.op_code = READ_RT5; break;

		case I2716: Jdata.op_code = READ_2716; break;
		case I2732: Jdata.op_code = READ_2764; break;
		case I2764: Jdata.op_code = READ_2764; break;
		case I27128: Jdata.op_code = READ_2764; break;
		case I27256: Jdata.op_code = READ_2764; break;

		case AT28C16: Jdata.op_code = READ_2816; break;
		case AT28C64: Jdata.op_code = READ_2864; break;
		default: MessageBox(_T("Unsupported Device!"), NULL, MB_ICONEXCLAMATION|MB_OK ); return;
	}
	
	m_hProgress.SetRange32( 0, Jdata.len);
	m_hProgress.SetPos( 0 );
	m_hProgress.Invalidate();

	GlobalState = BLANK_CHECK;	// Инициация задачи чтения ПЗУ

	DataView.DeleteAllItems();
	DataView.Invalidate();

	OpMode.SetWindowTextW(_T("Blank check...")); 
	OpMode.Invalidate();

	
	//FileBuffer[MAX_BIN_FILE_SIZE];
	memset( FileBuffer, dev[Jdata.op_dev].empty, MAX_BIN_FILE_SIZE );
	FileSize = Jdata.len; // To store

}


/*
	Проверить подключенные порты
*/
void CPGMDlg::OnToolsRescanserialports()
{
	// TODO: добавьте свой код обработчика команд
	int i;

	// Текущий порт должен быть закрыт
	if( isOpen == true )
	{
		CloseCommPort();
		isOpen = false;
	}

	/////////////////////////////////////////////////////////////////
	// Доступные для подключения порты
	ConnPort.ResetContent();
	ScanCommPorts();
	if( PortItems )
	{
		for( i=0; i<PortItems; i++ )
		{
			ConnPort.InsertString( i, PortNames[i] );
		}
		ConnPort.SetCurSel( 0 );
	}
}

// Проверка ПЗУ
int CPGMDlg::BlankCheckROM(void)
{
			if( Jdata.rq == true )
			{
				if( (Jdata.address % 16 ) == 0 )
				{
					CString str;

					str.Format(_T("ROM blank check at: 0x%04X-0x%04X" ), Jdata.address, Jdata.address + 0x0F );

					OpMode.SetWindowTextW( str.LockBuffer() );
					OpMode.Invalidate();
					str.UnlockBuffer();
				}

				Jdata.rq = false; // ждём ответа

				ring_buffer_flush();	// Clear income buffer

				Uart_Tx_Buffer[0] = Jdata.op_code;		// 3 bytes total
				Uart_Tx_Buffer[1] = (BYTE)Jdata.address; // low byte
				Uart_Tx_Buffer[2] = (BYTE)(Jdata.address >> 8); // high byte
				SendComm( Uart_Tx_Buffer, 3 );

				wd_timer = OP_TIMEOUT;

			}
			else
			{ // ждём ответа,если не конец и не ошибка, то продолжаем
				if( ring_buffer_get_used() != 0 )
				{
					wd_timer = OP_TIMEOUT;

					BYTE idata;
					ring_buffer_read( &idata, 1 );

					// Для РТ4 только младшие 4 бита
					if( dev[Jdata.op_dev].num == K556RT4 ) idata = idata & 0x0F;

					if( idata != dev[Jdata.op_dev].empty )
					{
						CString msg;

						GlobalState = IDLE;

						msg.Format( _T("Blank check failed at address 0x%04X: Blank = 0x%02X ROM = 0x%02X"), Jdata.address, dev[Jdata.op_dev].empty, idata );

						MessageBox( msg.LockBuffer(), _T("Blank check ROM"), MB_ICONSTOP|MB_OK );
						msg.UnlockBuffer();

						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();
						return 0;
					}

					Jdata.len--;

					if( Jdata.len != 0 )
					{ // Продолжаем чтение
						Jdata.address++;

						m_hProgress.SetPos( Jdata.address );
						m_hProgress.Invalidate();

						Jdata.rq = true;
					}
					else
					{ // Операция полностью завершена
						GlobalState = IDLE;
						MessageBox( _T("Blank check process complete! \n ROM is empty."), _T("Blank check ROM"), MB_ICONINFORMATION|MB_OK );

						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

					} // ring_buffer_get_used() == 0
				} // else Jdata.rq == FALSE
				else
				{
					wd_timer--;
					if( wd_timer == 0 )
					{
						MessageBox(_T("Operation terminated!!!"), _T("Timeout"), MB_ICONEXCLAMATION|MB_OK);
						GlobalState = IDLE;
						m_hProgress.SetPos( 0 );
						m_hProgress.Invalidate();

						return 0;
					}
				}
			} // Case READ_TO_BUFFER
	return 0;
}

void CPGMDlg::OnDeviceRestartprogrammator()
{
	// TODO: добавьте свой код обработчика команд
	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}

	ring_buffer_flush();	// Clear income buffer

	Uart_Tx_Buffer[0] = PGM_RESTART;	// 3 bytes total
	SendComm( Uart_Tx_Buffer, 1 );

	OpMode.SetWindowTextW(_T("PGM_RESTART was sent."));
	OpMode.Invalidate();
}

void CPGMDlg::OnDeviceRunaddresscount()
{
	// TODO: добавьте свой код обработчика команд
	if( isOpen == false )
	{
		OpMode.SetWindowTextW(_T("Try connect board..."));
		OpMode.Invalidate();
		if( TryConnectBoard() == false ) return;
		OpMode.SetWindowTextW(_T("Idle."));
		OpMode.Invalidate();
	}

	ring_buffer_flush();	// Clear income buffer

	Uart_Tx_Buffer[0] = RUN_ADDRESS;	// 3 bytes total
	SendComm( Uart_Tx_Buffer, 1 );

	OpMode.SetWindowTextW(_T("RUN_ADDRESS was sent."));
	OpMode.Invalidate();
}
