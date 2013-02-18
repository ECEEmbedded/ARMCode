#include "myDefs.h"
#if MILESTONE_2==1
#ifndef NAVIGATION_H
#define NAVIGATION_H
#include "myTypes.h"

#define maxNavigationMsgLen 5

// Public API
//
// The job of this task is to get the rover from Point A to Point B
// Start the task
// Args:
//   navData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   mc: pointer to the data structure for the motor control task
//   mapData: pointer to the data structure for the mapping task
//   speedData: pointer to the data structure for the speed limit control task
//   lcdData: pointer to the data structure for the LCD task
// Return:
//   void
void vStartNavigationTask(navigationStruct *navData,unsigned portBASE_TYPE uxPriority, motorControlStruct *mc, mappingStruct *mapData, speedLimitControlStruct *speedData, vtLCDStruct *lcdData);

//Motor Control communication API

// Get the number of centimeters the rover has moved forward
// Args:
//   navData -- a pointer to the data structure for the Navigation Task
//   centimeters -- the number of centimeters the rover has moved forward
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE updateMoveForwardMsg(navigationStruct *navData,uint8_t centimeters);

// Get the number of centimeters the rover has moved backward
// Args:
//   navData -- a pointer to the data structure for the Navigation Task
//   centimeters -- the number of centimeters the rover has moved backward
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE updateMoveBackwardMsg(navigationStruct *navData,uint8_t centimeters);

// Get the number of degrees the rover has rotated clockwise
// Args:
//   navData -- a pointer to the data structure for the Navigation Task
//   degrees -- the number of degress the rover has rotated clockwise
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE updateRotateClockwiseMsg(navigationStruct *navData,uint8_t degrees);

// Get the number of degrees the rover has rotated counterclockwise
// Args:
//   navData -- a pointer to the data structure for the Navigation Task
//   degrees -- the number of degress the rover has rotated counterclockwise
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE updateRotateCounterclockwiseMsg(navigationStruct *navData,uint8_t degrees);

// I2C communication API

// Get color sensor readings from the I2C bus
// Send the navigation thread (me) the readings via the I2C bus
// Args:
//   navData -- a pointer to the data structure for the Navigation Task
//   data -- a buffer containing the color sensor reading(s)
//   length -- the length of the data buffer
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendSpeedLimitDataMsg(navigationStruct *navData,uint8_t *data, uint8_t length);

#endif
#endif