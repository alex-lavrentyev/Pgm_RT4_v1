/*
	Board exchange protocol
*/

/*
	Все операции программатор производит только побайтно.
	Это значит, что запись/чтение больших ПЗУ может занять несколько минут
	Для проверки чистоты ПЗУ следует прочитать все байты.
	FLASH ПЗУ чистятся только целиком, 
	перед записью нужно очистить, если использовалась, автоочистки нет.
	(!!!) 2716 и 2716E имеют разные времянки, быть внимательным
*/

#ifndef		__PROTOCOL_H__
#define		__PROTOCOL_H__			1

// To check connection or work
// Board must return RET_OK
#define		PGM_RESTART				0x60	

//////////////////////////////////////////////////////////////////////////
// 0x6A + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_RT4				0x6A

// 0x6B + AddrLo + AddrHi
// Return Data byte from device
#define		READ_RT4				0x6B


/////////////////////////////////////////////////////////////////////////
// 0x6C + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_RT5				0x6C

// 0x6D + AddrLo + AddrHi
// Return Data byte from device
#define		READ_RT5				0x6D


////////////////////////////////////////////////////////////////////////
// Burn 2716 2732 with Vpp = 24V
// 0x71 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_2716				0x71

// 0x72 + AddrLo + AddrHi
// Return Data byte from device
#define		READ_2716				0x72

// Burn 2764 27128 with Vpp = 12V
// 0x75 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_2764				0x75

// 0x76 + AddrLo + AddrHi
// Return Data byte from device
#define		READ_2764				0x76

// Burn 27256  with Vpp = 12V
// 0x77 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_27256				0x77

///////////////////////////////////////////////////////////////////////////
// Burn 2816  FLASH tech, used 12V on OEn pin only for ERASE Op
// 0x31 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_2816				0x31

// 0x32 + AddrLo + AddrHi
// Return Data byte from device
#define		READ_2816				0x32

// 0x33
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		ERASE_2816				0x33

// (!!!) 2816E have different timings to compare 2816
// Burn 2764 27128 with Vpp = 12V
// 0x75 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_2816E				0x35

// 0x76 + AddrLo + AddrHi
// Return Data byte from device
#define		READ_2816E				0x36

// 0x37
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		ERASE_2816E				0x37

// Burn 2764 27128 with Vpp = 12V
// 0x75 + AddrLo + AddrHi + Data
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		WRITE_2864				0x38

// 0x76 + AddrLo + AddrHi
// Return Data byte from device
#define		READ_2864				0x39

// 0x3A
// Return RET_OK if burn byte success or
// Return RET_ERROR if burn failed
#define		ERASE_2864				0x3A


// WE pulse width for FLASH series 28xx
// 1 tik at Fosc = 14745600 ~ 70nS
#define		T_WP_500_NS				8
#define		T_WP_100_NS				2


/////////////////////////////////////////////////////////////////////////
// Коды ответа программатора
#define		RET_OK					0x5A
#define		RET_ERROR				0x50

////////////////////////////////////////////////////////////////////////
// Для тестирования и прогрева РТ4, RT5 и др.
// C максимальной скоростью перебираем адресные линии
// Любой пришедший байт отменяет команду
#define		RUN_ADDRESS				0x61		// 


#endif
