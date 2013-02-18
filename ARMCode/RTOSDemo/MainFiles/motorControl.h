#include "myDefs.h"
#if MILESTONE_2==1
#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H
#include "i2c_ARM.h"
#include "myTypes.h"

#define maxMotorMsgLen 5

// Public API
//
// The job of this task is to provide an API for the Navigation task to move the Rover
// Start the task
// Args:
//   motorControlData: Data structure used by the task
//   uxPriority -- the priority you want this task to be run at
//   myi2c: pointer to the data structure for the i2c task
//   navData: pointer to the data structure for the Navigation Task
void vStartMotorControlTask(motorControlStruct *motorControlData,unsigned portBASE_TYPE uxPriority, myI2CStruct *myi2c, navigationStruct *navData, vtLCDStruct *lcdData);

//Motor Commands

// Tell the rover to move forward
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
//   centimeters -- the number of centimeters the rover should move forward
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendMoveForwardMsg(motorControlStruct *motorControlData,uint8_t centimeters);

// Tell the rover to move backward
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
//   centimeters -- the number of centimeters the rover should move backward
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendMoveBackwardMsg(motorControlStruct *motorControlData,uint8_t centimeters);

// Tell the rover rotate clockwise
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
//   degrees -- the number of degrees to rotate (between 0 and 180)
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendRotateClockwiseMsg(motorControlStruct *motorControlData,uint8_t degrees);

// Tell the rover rotate counterclockwise
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
//   degrees -- the number of degrees to rotate (between 0 and 180)
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendRotateCounterclockwiseMsg(motorControlStruct *motorControlData,uint8_t degrees);

// Tell the rover to abort its current motor operation (AKA Stop the rover)
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendMotorAbortMsg(motorControlStruct *motorControlData);

// Send the Motor Control thread encoder data from the I2C bus
// Args:
//   motorControlData -- a pointer to the data structure for the Motor Control Task
//   data -- buffer containing encoder data
//   length -- length of the data buffer
// Return:
//   Result of the call to xQueueSend()
portBASE_TYPE sendEncoderDataMsg(motorControlStruct *motorControlData,uint8_t *data, uint8_t length);

portBASE_TYPE sendmotorTimerMsg(motorControlStruct *motorData, portTickType ticksElapsed, portTickType ticksToBlock);
#endif
#endif