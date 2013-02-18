#include "myDefs.h"
#if MILESTONE_2==1
#ifndef SPEEDLIMIT_H
#define SPEEDLIMIT_H
#include "i2c_ARM.h"
#include "myTypes.h"

#define maxSpeedLimitMsgLen 5

// Public API
//
// The job of this task is to keep track of the state of the speed limit
// Start the task
// Args:
//   speedData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   myi2c: pointer to the data structure for the i2c task
//   navData: pointer to the data structure for the Navigation Task
//   mapData: pointer to the data structure for the Mapping Task
void vStartSpeedLimitTask(speedLimitControlStruct *speedData,unsigned portBASE_TYPE uxPriority, myI2CStruct *myi2c, navigationStruct *navData, mappingStruct *mapData);

// Send the Speed Limit Control thread color sensor data from the I2C bus
// Args:
//   speedData -- a pointer to the data structure for the Speed Limit Control task
//   data -- buffer containing encoder data
//   length -- length of the data buffer
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendColorSensorDataMsg(speedLimitControlStruct *speedData,uint8_t *data, uint8_t length);

#endif
#endif