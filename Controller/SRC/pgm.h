/*
		�������� ������ � ������� ����������

	������������ ����� ������ ����������� ����� �� 65536 ���� (������� 32768)
	������������ ����� ���������� ����� �� 1 �� 16 ����
	������������ ����� ������� ���� ��������� (��������/��� ������)
*/

#ifndef		__PGM_H__
#define		__PGM_H__		1

// Return values
#define			OK			0
#define			FAILED		1

/*
*/

// KR556RT4 Timing
#define		RT4_NORMAL_N			4000
#define		RT4_NORMAL_T			100
#define		RT4_FORCED_N			100
#define		RT4_FORCED_T			10  //10000 // 10-15 mS
#define		RT4_FIX_N				100
#define		RT4_FIX_T				100
#define		RT4_Q					20		// ����������


// KR556RT5 Timing
#define		RT5_NORMAL_N			400
#define		RT5_NORMAL_T			50		// �� 5 ... 100 ��
#define		RT5_Q					4		// ����������



typedef enum
{
	FLASH_2816 = 1,
	FLASH_28256 = 2,
	EPROM_2716 = 3,
	EPROM_27256 = 4,
	KR556RT4 = 5,
	KR556RT5 = 6
}PGM_MODE;


#endif
//-----------------------------------------------------------------------------
