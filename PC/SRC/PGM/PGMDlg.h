// PGMDlg.h : ���� ���������
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#define		MAX_BIN_FILE_SIZE		65536
#define		BYTES_IN_ROW			8 // ��� ���������� ������ � ����
#define		STATE_TIMEOUT			100

#define		MAX_BYTES_IN_STRING			8

// ���������� ���� CPGMDlg
class CPGMDlg : public CDialog
{
// ��������
public:
	CPGMDlg(CWnd* pParent = NULL);	// ����������� �����������

// ������ ����������� ����
	enum { IDD = IDD_PGM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// ��������� DDX/DDV


// ����������
protected:
	HICON m_hIcon;

	// ��������� ������� ����� ���������
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
	// ������������ ���������������� ������
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
	// ���� ������
	bool isOpen;
	afx_msg void OnBoardRead();
	// ������� ������� ���� � ����������� � ��������������
	bool TryConnectBoard(void);
	afx_msg void OnCbnSelendokConnPort();
	CProgressCtrl m_hProgress;
	// ��������� ������ ���
	int ReadToBuffer(void);
	afx_msg void OnFileComparetobuffer();
	afx_msg void OnDeviceComparetobuffer();
	afx_msg void OnBoardBurnbuffer();
	// ������ ���
	int BurnFromBuffer(void);
	// ��������� ��� � �������
	int Compare_vs_Buffer(void);
	afx_msg void OnDeviceErase();
	afx_msg void OnDeviceBlankcheck();
	afx_msg void OnToolsRescanserialports();
	// �������� ���
	int BlankCheckROM(void);
	afx_msg void OnDeviceRestartprogrammator();
	afx_msg void OnDeviceRunaddresscount();
};
