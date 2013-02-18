#include "myDefs.h"
#if MILESTONE_1==1
#include "LCDtask.h"

#include "i2c_ARM.h"
#include "myTypes.h"

#define maxADCMsgLen   (sizeof(portTickType))

// Public API
//
// Start the task
// Args:
//   adcData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   myi2c: pointer to the data structure for an i2c task
//   lcdData: pointer to the data structure for an LCD task (may be NULL)
void vStartADCTask(vtADCStruct *adcData,unsigned portBASE_TYPE uxPriority, myI2CStruct *myi2c, vtLCDStruct *lcdData);
//
// Send a value message to the ADC task
// Args:
//   adcData -- a pointer to a variable of type vtLCDStruct
//   msgType -- the type of the message to send
//   value -- The value to send
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE SendADCDataMsg(vtADCStruct *adcData,uint8_t msgType,uint8_t value,portTickType ticksToBlock);
//
// Send a timer message to the ADC task
// Args:
//   adcData -- a pointer to a variable of type vtLCDStruct
//   ticksElapsed -- number of ticks since the last message (this will be sent in the message)
//   ticksToBlock -- how long the routine should wait if the queue is full
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE SendADCTimerMsg(vtADCStruct *adcData,portTickType ticksElapsed,portTickType ticksToBlock);

#endif
