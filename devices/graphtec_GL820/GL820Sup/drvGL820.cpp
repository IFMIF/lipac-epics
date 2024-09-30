/*
 * drvGL820.cpp
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>

#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <iocsh.h>

#include "drvGL820.h"
#include <epicsExport.h>

static const char *driverName="drvGL820";

//
//
//
void readTask(void *drvPvt)
{
  drvGL820 *pPvt = (drvGL820 *)drvPvt;

  pPvt->readTask();
}

//
//
//
drvGL820::drvGL820(const char *portName, const char *asynIpPortName) : asynPortDriver(portName,
		   0, /* maxAddr */
		   NUM_GL820_PARAMS,
		   asynInt32Mask | asynFloat64Mask | asynEnumMask | asynDrvUserMask | asynOctetMask, /* Inter face mask */
		   asynOctetMask | asynEnumMask | asynDrvUserMask ,  /* Interrupt mask */
		   0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
		   1, /* Autoconnect */
		   0, /* Default priority */
		   0) /* Default stack size*/
{
  char tmpStr[32];
  asynStatus status;
  const char* functionName = "drvGL820";
  
  pasynOctetSyncIO->connect(asynIpPortName, 0, &(this->pasynUserDriver), NULL);

  eventId_ = epicsEventCreate(epicsEventEmpty);

  createParam(P_SetStart_Str,       asynParamInt32,         &P_SetStart);
  createParam(P_SetStop_Str,        asynParamInt32,         &P_SetStop);
  for(int i=0 ; i<PORT_COUNT ; i++) {
    sprintf(tmpStr, "%s_%02d", P_GetChData_Str, i+1);
    createParam(tmpStr,        asynParamFloat64,       &P_GetChData[i]);
  }

  /* Create the thread that computes the waveforms in the background */
  status = (asynStatus)(epicsThreadCreate("drvGL820AsynPortDriverTask",
					  epicsThreadPriorityMedium,
					  epicsThreadGetStackSize(epicsThreadStackMedium),
					  (EPICSTHREADFUNC)::readTask,
					  this) == NULL);
  if (status) {
    printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
  }
}

//
//
//
asynStatus
drvGL820::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *paramName;
  const char* functionName = "writeInt32";

  /* Set the parameter in the parameter library. */
  status = (asynStatus) setIntegerParam(function, value);

  /* Fetch the parameter string name for possible use in debugging */
  getParamName(function, &paramName);

  // 通信するコマンド
  if(function == P_SetStart) {
    status = this->SetStartStop(pasynUser, 1);
  } else if(function == P_SetStop) {
    status = this->SetStartStop(pasynUser, 0);
  }

  /* Do callbacks so higher layers see any changes */
  status = (asynStatus) callParamCallbacks();

  if (status)
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, name=%s, value=%d",
                  driverName, functionName, status, function, paramName, value);
  else
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, name=%s, value=%d\n",
              driverName, functionName, function, paramName, value);
  return status;
}

//
//
//
asynStatus
drvGL820::readFloat64(asynUser *pasynUser, epicsFloat64 *value)
{
  int function = pasynUser->reason;
  asynStatus status = asynSuccess;
  const char *paramName;
  const char* functionName = "readFloat64";
  int ch = 0;

  /* Fetch the parameter string name for possible use in debugging */
  getParamName(function, &paramName);

  //  printf("[DEBUG]: readFloat64 paramName=%s function=%d\n", paramName, function);

  char tmpParamName[256];
  memset(tmpParamName, 0x00, sizeof(tmpParamName));
  strcpy(tmpParamName, paramName);
  if((function >= P_GetChData[0]) && (function <= P_GetChData[PORT_COUNT-1])) {
    // コマンドからチャネル番号を分離
    if(this->GetChNoFromParam(tmpParamName, &ch) == 1) {
      status = this->GetChData(pasynUser, *value, ch-1);
    }
  } else {
  }

  /* Do callbacks so higher layers see any changes */
  status = (asynStatus) callParamCallbacks();

  if (status)
    epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                  "%s:%s: status=%d, function=%d, name=%s, value=%f",
                  driverName, functionName, status, function, paramName, *value);
  else
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
              "%s:%s: function=%d, name=%s, value=%f\n",
              driverName, functionName, function, paramName, *value);
  return status;
}


//
// paramName の後ろ2文字をチャネル番号として分割する
// 後ろ2文字が数字でなければ、そのまま
//
int
drvGL820::GetChNoFromParam(char *param, int *chNo)
{
  int  rtn = 0;
  char chNoStr[3];
  int  paramLen = strlen(param);

  memset(chNoStr, 0x00, sizeof(chNoStr));
  memcpy(chNoStr, &param[paramLen - 2], 2);

  bool digit = true;
  for(int i=0 ; i<2 ; i++) {
    if(isdigit(chNoStr[i]) == 0) {
      digit = false;
      break;
    }
  }

  //  printf("[DEBUG]: GetChNoFromParam param=%s chNoStr=%s digit=%d\n", param, chNoStr, digit);

  if(digit == true) {
    *chNo = atoi(chNoStr);
    memset(&param[paramLen - 3], 0x00, 1);
    rtn = 1;
  }
  return rtn;
}


/* Configuration routine.  Called directly, or from the iocsh function below */

extern "C" {

  /** EPICS iocsh callable function to call constructor for the drvGL820 class.
   * \param[in] portName The name of the asyn port driver to be created.
   * \param[in] */
  int drvGL820Configure(const char *portName, const char *asynIpPortName)
  {
    new drvGL820(portName, asynIpPortName);
    return(asynSuccess);
  }


  /* EPICS iocsh shell commands */

  static const iocshArg initArg0 = { "portName",iocshArgString};
  static const iocshArg initArg1 = { "asynIpPortName",iocshArgString};
  static const iocshArg * const initArgs[] = {&initArg0, &initArg1};
  static const iocshFuncDef initFuncDef = {"drvGL820Configure",2,initArgs};
  static void initCallFunc(const iocshArgBuf *args)
  {
    drvGL820Configure(args[0].sval, args[1].sval);
  }

  void drvGL820Register(void)
  {
    iocshRegister(&initFuncDef,initCallFunc);
  }

  epicsExportRegistrar(drvGL820Register);

}
