#include "myDefs.h"
#if MILESTONE_2==1
#ifndef TYPES_H
#define TYPES_H

#include "i2c_ARM.h"
#include "motorControl.h"
#include "mapping.h"
#include "speedLimit.h"
#include "lcdTask.h"

// Our data structures for our tasks:
typedef struct __navigationStruct{
    motorControlStruct *motorControl;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} navigationStruct;

typedef struct __motorControlStruct {
    myI2CStruct *i2cData;
    webServerStruct *webData;
    vtLCDStruct *lcdData;
    xQueueHandle inQ;
} motorControlStruct;

typedef struct __speedLimitControlStruct{
    motorControlStruct *motorControl;
    navigationStruct *navData;
    webServerStruct *webData;
    xQueueHandle inQ;
} speedLimitControlStruct;

typedef struct __irControlStruct{
    navigationStruct *navData;
    xQueueHandle inQ;
} irControlStruct;

typedef struct __powerStruct{
    xQueueHandle inQ;
} powerStruct;

typedef struct __webServerStruct{
    xQueueHandle inQ;
} webServerStruct;

//I2C thread incoming and outgoing message types
#define vtI2CMsgTypeMotor 1
#define vtI2CMsgTypeRead 2
#define i2cMsgTypeTimer 3
#define notifyRqstRecvdType 4

//IR Control thread incoming message types
#define irDataMsg 1

//Power thread incoming message types
#define powerDataMsg 1

//Motor Control thread incoming message types
#define setDirForwardMsg 1
#define setDirReverseMsg 2
#define setMotorSpeedMsg 3
#define turnLeftMsg 4
#define turnRightMsg 5
#define motorStopMsg 6
#define encoderDataMsg 7

//Navigation thread incoming message types
#define AIUpdateDistancesMsgType 1
#define AIUpdateWallAnglesMsgType 2
#define AIUpdateIsWallsMsgType 3
#define AIUpdateFinishLineMsgType 4

//Web Server thread incoming message types
#define webNotifyCurrentSpeedMsgType 1
#define webNotifySpeedLimitZoneMsgType 2
#define webNotifyFinishLineMsgType 3
#define webNotifyPowerMsgType 4
#define webNotifyFastestTimeMsgType 5
#define webNotifySpeedViolationMsgType 6

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
#define PIC2680_EMPTY_MESSAGE 0x55
#define PIC26J50_EMPTY_MESSAGE 0x56
//Non-empty messages
#define COLOR_SENSOR_MESSAGE 0x10
#define ENCODERS_MESSAGE 0x11
#define IR_MESSAGE 0x12
#define PIC2680_ADC_MESSAGE 0x13
#define PIC26J50_ADC_MESSAGE 0x14

//I2C error message types to be sent to Web Server
#define COLOR_SENSOR_RQST_DROPPED 0xF0
#define ENCODERS_RQST_DROPPED 0xF1
#define IR_RQST_DROPPED 0xF2
#define MOTOR_RQST_DROPPED 0xF3
#define PIC2680_ERROR 0xF4
#define PIC26J50_ERROR 0xF5

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
#define INCORRECT_IR_MSG_FORMAT 17
#define UNKNOWN_IR_MSG_TYPE 18
#define INCORRECT_POWER_MSG_FORMAT 19
#define UNKNOWN_POWER_MSG_TYPE 20
#define INCORRECT_WEB_SERVER_MSG_FORMAT 21
#define UNKNOWN_WEB_SERVER_MSG_TYPE 22

#endif
#endif