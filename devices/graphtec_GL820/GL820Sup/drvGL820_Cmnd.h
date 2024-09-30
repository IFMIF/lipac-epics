#ifndef _drvGL820_Cmnd_Header_File_Included
#define _drvGL820_Cmnd_Header_File_Included

#include "asynPortDriver.h"
#include "asynOctetSyncIO.h"
#include "drvGL820.h"

#define MAX_BUF_SIZE            65536
#define PORT_COUNT		20
#define TIMEOUT			5

#define WORD			unsigned short
#define SWAPWORD(x)		(WORD)((((WORD)x & 0x00ff) << 8) | (((WORD)x & 0xff00) >> 8))

typedef struct _ChCoef {
  float   intCoef;		// 整数部係数
  float   decCoef;		// 小数部係数
  char    modeStr[8];		// 入力設定文字列
  char    rangeStr[16];		// レンジ設定文字列
} CH_COEF;

typedef struct _MeasReadData {
  char	  header[8];		// header
  WORD	  analog[PORT_COUNT];	// analog
  WORD    pulse[2][4];
  WORD    logic;
  WORD    alarm[2];
  WORD    alarmLP;
  WORD    alarmOut;
  WORD    status;
} MEAS_READ_DATA;

#endif // _drvGL820_Cmnd_Header_File_Included
