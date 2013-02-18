#include "myDefs.h"
#if MILESTONE_2==1
#ifndef CONDUCTOR_H
#define CONDUCTOR_H

#include "vtI2C.h"
#include "i2c_ARM.h"
#include "motorControl.h"
#include "navigation.h"

// Structure used to pass parameters to the task
// Do not touch...
typedef struct __ConductorStruct {
	vtI2CStruct *dev;
	myI2CStruct *i2cData;
    motorControlStruct *motorControl;
    navigationStruct *navData;
    speedLimitControlStruct *speedData;
    myADCStruct *adcData;
} vtConductorStruct;

// Public API
//
// The job of this task is to read from the message queue that is output by the I2C thread and to distribute the messages to the right
//   threads.
// Start the task.
// Args:
//   _conductorData: Data structure used by the task
//   _uxPriority -- the priority you want this task to be run at
//   _i2c: pointer to the data structure for an i2c interrupt handler
//   _myi2c: pointer to the data structure for the i2c task
//   _mc: pointer to the data structure for the motor control task
//   _nav: pointer to the data structure for the navigation task
//   _speed: pointer to the data structure for the speed limit control task
void vStartConductorTask(vtConductorStruct *_conductorData,unsigned portBASE_TYPE _uxPriority, vtI2CStruct *_i2c,myI2CStruct *_myi2c, motorControlStruct *_mc, navigationStruct *_nav, speedLimitControlStruct *_speed, myADCStruct *_adc);
#endif
#endif