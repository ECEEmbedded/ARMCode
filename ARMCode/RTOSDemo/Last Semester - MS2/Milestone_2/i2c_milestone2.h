#include "myDefs.h"
#if MILESTONE_2==1
#ifndef I2C_TASK_H
#define I2C_TASK_H
#include "vtI2C.h"
#include "lcdTask_milestone2.h"
// Structure used to pass parameters to the task
// Do not touch...
typedef struct __i2cStruct {
	vtI2CStruct *dev;
	vtLCDStruct *lcdData;
	xQueueHandle inQ;
} myI2CStruct;
// Maximum length of a message that can be received by this task
#define vti2cMaxLen   5

#define ADC_MSG_SIZE 4

#define PIC2680 0
#define PIC26J50 1

// Public API
//
// Start the task
// Args:
//   i2cData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   i2c: pointer to the data structure for an i2c task
//   lcd: pointer to the data structure for an LCD task (may be NULL)
void starti2cTask(myI2CStruct *i2cData,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,vtLCDStruct *lcd);
//
// Send a timer message to the i2c task
// Args:
//   i2cData -- a pointer to a variable of type vtLCDStruct
//   ticksElapsed -- number of ticks since the last message (this will be sent in the message)
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendi2cTimerMsg(myI2CStruct *i2cData,portTickType ticksElapsed,portTickType ticksToBlock);
//
// Send a value message to the Temperature task
// Args:
//   i2cData -- a pointer to a variable of type vtLCDStruct
//   msgType -- the type of the message to send
//   value -- The value to send
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendi2cValueMsg(myI2CStruct *i2cData,uint8_t msgType,uint8_t* value, uint8_t length, portTickType ticksToBlock);
#endif
#endif