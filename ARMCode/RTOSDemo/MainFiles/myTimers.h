#include "myDefs.h"
#if MILESTONE_1
#ifndef _MY_TIMERS_H
#define _MY_TIMERS_H
#include "lcdTask.h"
#include "i2cTemp.h"
//#include "i2cADC.h"
void startTimerForLCD(vtLCDStruct *vtLCDdata);
void startTimerForTemperature(vtTempStruct *vtTempdata);
//void startTimerForADC(vtADCStruct *vtADCdata);
#endif
#endif