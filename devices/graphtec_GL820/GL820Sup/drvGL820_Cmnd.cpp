/*
 * drvGL820_Cmnd.cpp
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "drvGL820.h"
#include "drvGL820_Cmnd.h"
#include <epicsExport.h>

//
//
//
asynStatus
drvGL820::WriteRead(char *sendBuffer, char *recvBuffer, int &recvBufSize, int timeout)
{
  size_t nActual = 0;
  size_t nRead;
  int    eomReason;
  char   *pValue      = sendBuffer;
  size_t nChars       = strlen(sendBuffer);
  char   *pReadBuffer = recvBuffer;

  asynStatus status = pasynOctetSyncIO->writeRead(this->pasynUserDriver, pValue, nChars,
                                                  pReadBuffer, MAX_BUF_SIZE, timeout,
                                                  &nActual, &nRead, &eomReason);
  // printf("[DEBUG(drvGL820::WriteRead)]: status=%d pReadBuffer=%s\n", status, pReadBuffer);
  // status = (asynStatus) callParamCallbacks();
  callParamCallbacks();
  recvBufSize = nRead;
  
  return status;
}

//
//
//
int
drvGL820::StrCmndParse(char *readBuf, char *valueBuf)
{
  int rtn = -1;

  if(readBuf != NULL) {
    if(strlen(readBuf) > 0) {
      char *ptr = strchr(readBuf, '\r');
      if(ptr != NULL) {
	      memset(ptr, 0x00, 1);
      }
      ptr = strchr(readBuf, '\n');
      if(ptr != NULL) {
	      memset(ptr, 0x00, 1);
      }
      ptr = strchr(readBuf, ' ');
      if(ptr != NULL) {
        ptr++;
        strcpy(valueBuf, ptr);
        rtn = strlen(valueBuf);
      }
    }
  }
  // printf("[DEBUG]: StrCmndParse valueBuf=%s rtn=%d\n", valueBuf, rtn);
  return rtn;
}

//
// 
//
int
drvGL820::DCRangeStrCmndParse(char *readBuf, CH_COEF &coef)
{
  int rtn = 0;
  if(this->StrCmndParse(readBuf, coef.rangeStr) > 0) {
    // printf("[DEBUG(drvGL820::DCRangeStrCmndParse)]; readBuf=%s coef.rangeStr=%s\n", readBuf, coef.rangeStr);

    if(coef.rangeStr[0] == '1') {
      coef.intCoef = 2;
    } else if(coef.rangeStr[0] == '2') {
      coef.intCoef = 1;
    } else if(coef.rangeStr[0] == '5') {
      coef.intCoef = 4;
    }

    char *dataDef[] =  {"20MV",				// 
			"50MV", "100MV", "200MV",	//
			"500MV", "1V",   "2V",		//
			"5V",    "10V",  "20V", "1-5V",	//
			"50V",   "100V"};		// 
    // V換算
    double coefDef[] = {1000000,
			100000, 100000, 100000,
			10000,  10000,  10000,
			1000,   1000,   1000,   1000,
			100,	100};

    for(int i=0 ; i<13 ; i++) {
      if(strcasecmp(coef.rangeStr, dataDef[i]) == 0) {
	coef.decCoef = 1.0 / coefDef[i];
	break;
      }
    }
    // printf("[DEBUG(drvGL820::DCRangeStrCmndParse)]; coef.rangeStr=%s coef.intCoef=%f coef.decCoef=%f\n", coef.rangeStr,coef.intCoef,coef.decCoef);
    rtn = 1;
  }
  return rtn;
}

//
// 測定開始/停止設定
// 機器の測定開始/停止ではなく、換算係数取得を行う
//
asynStatus
drvGL820::SetStartStop(asynUser *pasynUser, epicsInt32 value)
{
  asynStatus rtn = asynSuccess;
  char cmndBuf[64];
  char dataBuf[256];
  int  dataBufSize;
  int  i;

  memset(cmndBuf, 0x00, sizeof(cmndBuf));
  memset(dataBuf, 0x00, sizeof(dataBuf));

  pasynOctetSyncIO->setInputEos(this->pasynUserDriver, "\r\n", 2);

  if(value == 1) {
    for(i=0 ; i<PORT_COUNT ; i++) {
      sprintf(cmndBuf, ":AMP:CH%d:INP?\r\n", i+1);
      
      // チャネル入力状態取得
      if(this->WriteRead(cmndBuf, dataBuf, dataBufSize) != asynError) {
	      if(this->StrCmndParse(dataBuf, m_ChCoef[i].modeStr) > 0) {
          if(strcasecmp(m_ChCoef[i].modeStr, "DC") == 0) {
            // DCの場合には、RANGEを取得
            sprintf(cmndBuf, ":AMP:CH%d:RANG?\r\n", i+1);
            memset(dataBuf, 0x00, sizeof(dataBuf));
            if(this->WriteRead(cmndBuf, dataBuf, dataBufSize) != asynError) {
              this->DCRangeStrCmndParse(dataBuf, m_ChCoef[i]);
            }
            // printf("[DEBUG(drvGL820::SetStartStop)]; dataBuf=%s m_ChCoef[%02d].modeStr=%s m_ChCoef[%02d].rangeStr=%s\n", dataBuf, i, m_ChCoef[i].modeStr, i, m_ChCoef[i].rangeStr);
          } else if(strcasecmp(m_ChCoef[i].modeStr, "TEMP") == 0) {
            // 温度の場合には、係数は0.1固定
            m_ChCoef[i].intCoef = 1;
            m_ChCoef[i].decCoef = 0.005;
            //printf("[DEBUG]: temp mode\n");
          } else if(strcasecmp(m_ChCoef[i].modeStr, "RH") == 0) {
            // 湿度の場合には、DC1Vと同じ
            // 整数部: 測定値 / 2
            // 小数部: 整数部 / 10000
            m_ChCoef[i].intCoef = 2;
            m_ChCoef[i].decCoef = 1 / 10000;
          } else {
            m_ChCoef[i].intCoef = 0;
            m_ChCoef[i].decCoef = 0;
          }
        }
      }
    }
    m_StartFlag = 1;
  } else {
    m_StartFlag = 0;
  }
  return rtn;
}

//
// データ取得
// 
asynStatus
drvGL820::GetChData(asynUser *pasynUser, epicsFloat64 &value, int ch)
{
  asynStatus rtn = asynError;
  
  if((ch >= 0) && (ch < PORT_COUNT)) {
    value = this->m_ChData[ch];
    rtn = asynSuccess;
  }
  return rtn;
}

//
// データ受信スレッド
//
void drvGL820::readTask(void)
{
  char cmndBuf[32];
  MEAS_READ_DATA recvBuf;
  int  dataBufSize;

  memset(cmndBuf, 0x00, sizeof(cmndBuf));
  memset(&recvBuf, 0x00, sizeof(recvBuf));

  sprintf(cmndBuf, ":MEAS:OUTP:ONE?\r\n");

  while(1) {
    if(m_StartFlag == 1) {
      pasynOctetSyncIO->setInputEos(this->pasynUserDriver, "", 0);
      dataBufSize = sizeof(MEAS_READ_DATA);
      if(this->WriteRead(cmndBuf, (char *)&recvBuf, dataBufSize, 1) != asynError) {
        for(int i=0 ; i<PORT_COUNT ; i++) {
          // Note: the channel readings come in the opposite order
          /*WORD tmpData = SWAPWORD(recvBuf.analog[PORT_COUNT-i-1]);*/
          /*WORD tmpData = SWAPWORD(recvBuf.analog[PORT_COUNT-i-1]);*/
          WORD tmpData = SWAPWORD(recvBuf.analog[i]);
          //printf("Raw measurement: %d -> %d\n", i, tmpData);
          //printf("[DEBUG(drvGL820::readTask)]; tmpData[%02d]=%x m_ChData[%02d]=%f m_ChCoef[%d].intCoef=%f m_ChCoef[%02d].decCoef=%f m_ChCoef[%d].modeStr=%s\n", i, tmpData, i, m_ChData[i], i, m_ChCoef[i].intCoef, i, m_ChCoef[i].decCoef, i, m_ChCoef[i].modeStr);

          double td = (double)((signed short)tmpData);
          if(tmpData == 0xffff) {
            this->m_ChData[i] = -999;
          } else if((tmpData == 0x7fff) || (tmpData == 0x7ffe) || (tmpData == 0x7ffd) || (tmpData == 0x7ffd)) {
            this->m_ChData[i] = -999;
          } else {
            this->m_ChData[i] = (td / this->m_ChCoef[i].intCoef) * this->m_ChCoef[i].decCoef;
          }          
        }

        //printf("\n");
      }
    } else {
      epicsThreadSleep(1);
    }
  }
}
