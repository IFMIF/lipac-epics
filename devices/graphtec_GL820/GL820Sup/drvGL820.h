#ifndef _drvGL820_Header_File_Included
#define _drvGL820_Header_File_Included
/*
 * drvGL820.h
 *
 * Asyn driver that inherits from the asynPortDriver class to demonstrate its use.
 * It simulates a digital scope looking at a 1kHz 1000-point noisy sine wave.  Controls are
 * provided for time/division, volts/division, volt offset, trigger delay, noise amplitude, update time,
 * and run/stop.
 * Readbacks are provides for the waveform data, min, max and mean values.
 *
 * Author: Mark Rivers
 *
 * Created Feb. 5, 2009
 */

#include "asynPortDriver.h"
#include "asynOctetSyncIO.h"
#include "drvGL820_Cmnd.h"

/* These are the drvInfo strings that are used to identify the parameters.
 * They are used by asyn clients, including standard asyn device support */
#define P_SetStart_Str            "SET_START"                     /* asynOctet      w  */
#define P_SetStop_Str             "SET_STOP"                      /* asynOctet      w  */
#define P_GetChData_Str           "GET_DATA"                      /* asynFloat64    r  */

/** Class that demonstrates the use of the asynPortDriver base class to greatly simplify the task
 * of writing an asyn port driver.
 * This class does a simple simulation of a digital oscilloscope.  It computes a waveform, computes
 * statistics on the waveform, and does callbacks with the statistics and the waveform data itself.
 * I have made the methods of this class public in order to generate doxygen documentation for them,
 * but they should really all be private. */
class drvGL820 : public asynPortDriver {
 public:
  drvGL820(const char *portName, const char *asynIpPortName);

  /* These are the methods that we override from asynPortDriver */
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
  /*
  virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);
  virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
				      size_t nElements, size_t *nIn);
  */
  /*
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);

    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);

    virtual asynStatus writeInt8Array(asynUser *pasynUser, epicsInt8 *value, size_t nElements);

    virtual asynStatus readInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                     size_t nElements, size_t *nIn);
    virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[],
                                size_t nElements, size_t *nIn);
  */
  void readTask(void);

 protected:
  /** Values used for pasynUser->reason, and indexes into the parameter library. */
  int  P_SetStart;
#define FIRST_GL820_COMMAND P_SetStart
  int  P_GetChData[PORT_COUNT];
  int  P_SetStop;
#define LAST_GL820_COMMAND P_SetStop

 private:
  int GetChNoFromParam(char *param, int *chNo);

  epicsEventId eventId_;

  asynUser	*pasynUserDriver;
  asynStatus    WriteRead(char *sendBuffer, char *recvBuffer, int &recvBufSize, int timeout=TIMEOUT);

  asynStatus	SetStartStop(asynUser *pasynUser, epicsInt32 value);
  asynStatus	GetChData(asynUser *pasynUser, epicsFloat64 &value, int ch);

  int		StrCmndParse(char *readBuf, char *valueBuf);
  int		DCRangeStrCmndParse(char *readBuf, CH_COEF &coef);

  /*
  asynStatus	SetMeasInterval(asynUser *pasynUser, epicsInt32 value);
  asynStatus	GetMeasInterval(asynUser *pasynUser, epicsInt32 *value);
  */

  epicsFloat64	m_ChData[PORT_COUNT];
  CH_COEF	m_ChCoef[PORT_COUNT];
  
  int           m_StartFlag;
};


#define NUM_GL820_PARAMS (&LAST_GL820_COMMAND - &FIRST_GL820_COMMAND + 1)

#endif // _drvGL820_Header_File_Included
