#include "myDefs.h"
#if MILESTONE_2==1
#ifndef TYPES_H
#define TYPES_H

#include "i2c_ARM.h"

// Our data structures for our tasks:
typedef struct __myADCStruct{
    myI2CStruct *i2cData;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} myADCStruct;

typedef struct __navigationStruct{
    motorControlStruct *motorControl;
    mappingStruct *mapData;
    speedLimitControlStruct *speedData;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} navigationStruct;

typedef struct __motorControlStruct {
    myI2CStruct *i2cData;
    navigationStruct *navData;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} motorControlStruct;

typedef struct __mappingStruct{
    navigationStruct *navData;
    speedLimitControlStruct * speedData;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} mappingStruct;

typedef struct __speedLimitControlStruct{
    myI2CStruct *i2cData;
    navigationStruct *navData;
    mappingStruct *mapData;
    xQueueHandle inQ;
} speedLimitControlStruct;

//ADC thread incoming message types
#define ADCMsgTypeTimer 1
#define vtI2CMsgTypeADCSend 2

//I2C thread incoming and outgoing message types
#define vtI2CMsgTypeMotor 1
#define vtI2CMsgTypeRead 2
#define i2cMsgTypeTimer 3
#define notifyRqstRecvdType 4
//#define vtI2CMsgTypeADC 5

//Motor Control thread incoming message types
#define moveForwardMsg 1
#define moveBackwardMsg 2
#define rotateClockwiseMsg 3
#define rotateCounterclockwiseMsg 4
#define abortMsg 5
#define encoderDataMsg 6
#define motorTimerMsg 7

//Navigation thread incoming message types
#define updateMoveForwardMsgType 1
#define updateMoveBackwardMsgType 2
#define updateRotateClockwiseMsgType 3
#define updateRotateCounterclockwiseMsgType 4
#define updateSpeedLimitDataMsgType 5

//Mapping thread incoming message types
#define angleMsg 1

//Speed limit thread incoming message types
#define colorSensorDataMsg 1

//LCD message types
// a timer message -- not to be printed
#define LCDMsgTypeTimer 1
// a message to be printed
#define LCDMsgTypePrint 2
// Added by Matthew Ibarra for ADC LCD Task Message 2/4/2013
#define LCDMsgTypeADC 3

//I2C message types
//Empty Messages
#define COLOR_SENSOR_EMPTY_MESSAGE 0x50
#define ENCODERS_EMPTY_MESSAGE 0x51
#define IR_EMPTY_MESSAGE 0x52
#define ADC_EMPTY_MESSAGE 0x53
#define GENERIC_EMPTY_MESSAGE 0x54
//Non-empty messages
#define COLOR_SENSOR_MESSAGE 0x10
#define ENCODERS_MESSAGE 0x11
#define IR_MESSAGE 0x12
#define ADC_MESSAGE 0x13

//I2C error message types to be sent to Web Server
#define COLOR_SENSOR_RQST_DROPPED 0xF0
#define ENCODERS_RQST_DROPPED 0xF1
#define IR_RQST_DROPPED 0xF2
#define MOTOR_RQST_DROPPED 0xF3
#define ADC_RQST_DROPPED 0xF4

//Error codes
#define VT_I2C_Q_FULL 1
#define UNKNOWN_I2C_MSG_TYPE 2
#define UNKNOWN_CONDUCTOR_MSG_TYPE 3
#define INCORRECT_I2C_MSG_FORMAT 4
#define Q_RECV_ERROR 5
#define TIMER_ERROR 6
#define I2C_Q_FULL 7
#define TASK_CREATION_ERROR 8
#define INCORRECT_MOTOR_CONTROL_MSG_FORMAT 9
#define UNKNOWN_MOTOR_CONTROL_MSG_TYPE 10
#define INCORRECT_NAVIGATION_MSG_FORMAT 11
#define UNKNOWN_NAVIGATION_MSG_TYPE 12
#define INCORRECT_MAPPING_MSG_FORMAT 13
#define UNKNOWN_MAPPING_MSG_TYPE 14
#define INCORRECT_SPEED_LIMIT_MSG_FORMAT 15
#define UNKNOWN_SPEED_LIMIT_MSG_TYPE 16

#endif
#endif