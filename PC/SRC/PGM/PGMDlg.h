// PGMDlg.h : файл заголовка
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#define		MAX_BIN_FILE_SIZE		65536
#define		BYTES_IN_ROW			8 // для текстового вывода в файл
#define		STATE_TIMEOUT			100

#define		MAX_BYTES_IN_STRING			8

// диалоговое окно CPGMDlg
class CPGMDlg : public CDialog
{
// Создание
public:
	CPGMDlg(CWnd* pParent = NULL);	// стандартный конструктор

// Данные диалогового окна
	enum { IDD = IDD_PGM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// поддержка DDX/DDV


// Реализация
protected:
	HICON m_hIcon;

	// Созданные функции схемы сообщений
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//HANDLE hStateThread;
	// ROM content display
	CListCtrl DataView;
	// COM port selector
	CComboBox ConnPort;
	// Selector ROM device
	CComboBox ROMSelector;
	// Сканирование последовательных портов
	void ScanCommPorts(void);
	afx_msg void FileRead();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
	afx_msg void OnFileReadtobuffer();
	afx_msg void OnToolsBintotext();
	afx_msg void OnToolsTexttobin();
	bool IsDigit(char data);
	char Char2Byte(char data);

	afx_msg void OnFileWritefrombuffer();
	CStatic OpMode;
	// Порт открыт
	bool isOpen;
	afx_msg void OnBoardRead();
	// Попытка открыть порт и соединиться с программатором
	bool TryConnectBoard(void);
	afx_msg void OnCbnSelendokConnPort();
	CProgressCtrl m_hProgress;
	// Процедура чтения ПЗУ
	int ReadToBuffer(void);
	afx_msg void OnFileComparetobuffer();
	afx_msg void OnDeviceComparetobuffer();
	afx_msg void OnBoardBurnbuffer();
	// Запись ПЗУ
	int BurnFromBuffer(void);
	// Сравнение ПЗУ с буфером
	int Compare_vs_Buffer(void);
	afx_msg void OnDeviceErase();
	afx_msg void OnDeviceBlankcheck();
	afx_msg void OnToolsRescanserialports();
	// Проверка ПЗУ
	int BlankCheckROM(void);
	afx_msg void OnDeviceRestartprogrammator();
	afx_msg void OnDeviceRunaddresscount();
};
