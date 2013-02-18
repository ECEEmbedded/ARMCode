#include "myDefs.h"
#if MILESTONE_2==1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "semphr.h"

/* include files. */
#include "vtUtilities.h"
#include "i2c_ARM.h"
#include "myTypes.h"
#include "motorControl.h"
#include "navigation.h"
#include "LCDtask.h"
/* *********************************************** */

#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// Length of the queue to this task
#define motorControlQLen 10
// actual data structure that is sent in a message
typedef struct __motorControlMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxMotorMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} motorControlMsg;

/* The Motor Control task. */
static portTASK_FUNCTION_PROTO( vMotorControlTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartMotorControlTask(motorControlStruct *params,unsigned portBASE_TYPE uxPriority, myI2CStruct *myi2c, navigationStruct *navData, vtLCDStruct *lcdData)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(motorControlQLen,sizeof(motorControlMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->i2cData = myi2c;
    params->navData = navData;
    params->lcdData = lcdData;
    if ((retval = xTaskCreate( vMotorControlTask, ( signed char * ) "Motor Control", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE sendMoveForwardMsg(motorControlStruct *motorControlData,uint8_t centimeters)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    buffer.buf[0] = centimeters;
    buffer.msgType = moveForwardMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendMoveBackwardMsg(motorControlStruct *motorControlData,uint8_t centimeters)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    buffer.buf[0] = centimeters;
    buffer.msgType = moveBackwardMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendRotateClockwiseMsg(motorControlStruct *motorControlData,uint8_t degrees)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    buffer.buf[0] = degrees;
    buffer.msgType = rotateClockwiseMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendRotateCounterclockwiseMsg(motorControlStruct *motorControlData,uint8_t degrees)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    buffer.buf[0] = degrees;
    buffer.msgType = rotateCounterclockwiseMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendMotorAbortMsg(motorControlStruct *motorControlData)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = 0;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    buffer.msgType = abortMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendEncoderDataMsg(motorControlStruct *motorControlData,uint8_t *data, uint8_t length)
{
    if (motorControlData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = length;
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MOTOR_CONTROL_MSG_FORMAT);
    }
    memcpy(buffer.buf,data,length);
    buffer.msgType = encoderDataMsg;
    return(xQueueSend(motorControlData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendmotorTimerMsg(motorControlStruct *motorData, portTickType ticksElapsed, portTickType ticksToBlock){
    if (motorData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    motorControlMsg buffer;
    buffer.length = sizeof(ticksElapsed);
    if (buffer.length > maxMotorMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(buffer.length);
    }
    memcpy(buffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
    buffer.msgType = motorTimerMsg;
    return(xQueueSend(motorData->inQ,(void *) (&buffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/

//Motor and Encoder constants
#define COUNTS_PER_CENTIMETER 30
#define COUNTS_PER_DEGREE 2    // 10
#define TIMER_COUNTS_PER_CENTIMETER 1
#define DEGREES_PER_TIMER_COUNT 1.0     // 1.9
#define MOTOR_FORWARD_SPEED 34
#define MOTOR_BACKWARD_SPEED 94
#define MOTOR_STOP_SPEED 64
#define RIGHT_MOTOR_OFFSET 128

//Operations
#define NONE 0
#define FORWARD 1
#define BACKWARD 2
#define CLOCKWISE 3
#define COUNTERCLOCKWISE 4

//#define SEND_COUNTS_TO_LCD

uint8_t getDegrees(unsigned int right,unsigned int left){
    return ((right + left)/2)/COUNTS_PER_DEGREE;
}

uint8_t getCentimeters(unsigned int right,unsigned int left){
    return ((right + left)/2)/COUNTS_PER_CENTIMETER;
}

uint8_t getRightCount(motorControlMsg *buffer){
    return buffer->buf[0];
}

uint8_t getLeftCount(motorControlMsg *buffer){
    return buffer->buf[1];
}

unsigned int getTargetVal(motorControlMsg *buffer){
    return (unsigned int) buffer->buf[0];
}

static uint8_t currentOp;
static uint8_t lastOp;
static motorControlStruct *param;
static myI2CStruct *i2cData;
static navigationStruct *navData;

// Buffer for receiving messages
static motorControlMsg msgBuffer;
static unsigned int leftEncoderCount, rightEncoderCount;
static unsigned int targetVal;
static unsigned int currentTime;

static unsigned int forward, backward, clockwise;

static uint8_t delay;

static char msg[12];

// This is the actual task that is run
static portTASK_FUNCTION( vMotorControlTask, pvParameters )
{
    // Get the parameters
    param = (motorControlStruct *) pvParameters;
    // Get the I2C task pointer
    i2cData = param->i2cData;
    // Get the Navigation task pointer
    navData = param->navData;
    // Get the LCD task pointer
    vtLCDStruct *lcdData = param->lcdData;

    currentOp = NONE;
    lastOp = NONE;
    leftEncoderCount = 0;
    rightEncoderCount = 0;
    currentTime = 0;
    forward = 0;
    backward = 0;
    clockwise = 0;
    delay = 0;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from the I2C (Encoder data) or from the Navigation Task (motor command)
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(msgBuffer.msgType){
            case motorTimerMsg:
            {
                currentTime++;
                if(currentOp != NONE)
                {
                    if(currentTime == 5){
                        switch(currentOp){
                            case FORWARD:
                            {
                                //sendi2cMotorMsg(i2cData,MOTOR_FORWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_FORWARD_SPEED, portMAX_DELAY);
                                break;
                            }
                            case BACKWARD:
                            {
                                //sendi2cMotorMsg(i2cData,MOTOR_BACKWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_BACKWARD_SPEED, portMAX_DELAY);
                                break;
                            }
                            case CLOCKWISE:
                            {
                                //sendi2cMotorMsg(i2cData,MOTOR_BACKWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_FORWARD_SPEED, portMAX_DELAY);
                                break;
                            }
                            case COUNTERCLOCKWISE:
                            {
                                //sendi2cMotorMsg(i2cData,MOTOR_FORWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_BACKWARD_SPEED, portMAX_DELAY);
                                break;
                            }
                        }
                    }
                }
                break;
            }
            case moveForwardMsg:
            {
                currentOp = FORWARD;
                lastOp = FORWARD;
                currentTime = 0;
                leftEncoderCount = 0;
                rightEncoderCount = 0;
                //targetVal = getTargetVal(&msgBuffer)*TIMER_COUNTS_PER_CENTIMETER;
                //sendi2cMotorMsg(i2cData,MOTOR_FORWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_FORWARD_SPEED, portMAX_DELAY);
                break;
            }
            case moveBackwardMsg:
            {
                currentOp = BACKWARD;
                lastOp = BACKWARD;
                currentTime = 0;
                leftEncoderCount = 0;
                rightEncoderCount = 0;
                //targetVal = getTargetVal(&msgBuffer)*TIMER_COUNTS_PER_CENTIMETER;
                //sendi2cMotorMsg(i2cData,MOTOR_BACKWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_BACKWARD_SPEED, portMAX_DELAY);
                break;
            }
            case rotateClockwiseMsg:
            {
                currentOp = CLOCKWISE;
                lastOp = CLOCKWISE;
                currentTime = 0;
                leftEncoderCount = 0;
                rightEncoderCount = 0;
                //targetVal = getTargetVal(&msgBuffer)/DEGREES_PER_TIMER_COUNT;
                //sendi2cMotorMsg(i2cData,MOTOR_BACKWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_FORWARD_SPEED, portMAX_DELAY);
                break;
            }
            case rotateCounterclockwiseMsg:
            {
                currentOp = COUNTERCLOCKWISE;
                lastOp = COUNTERCLOCKWISE;
                currentTime = 0;
                leftEncoderCount = 0;
                rightEncoderCount = 0;
                //targetVal = getTargetVal(&msgBuffer)/DEGREES_PER_TIMER_COUNT;
                //sendi2cMotorMsg(i2cData,MOTOR_FORWARD_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_BACKWARD_SPEED, portMAX_DELAY);
                break;
            }
            case abortMsg:
            {
                currentOp = NONE;
                //sendi2cMotorMsg(i2cData,MOTOR_STOP_SPEED + RIGHT_MOTOR_OFFSET,MOTOR_STOP_SPEED, portMAX_DELAY);
                currentTime = 0;
                break;
            }
            case encoderDataMsg:
            {
                rightEncoderCount += getRightCount(&msgBuffer);
                leftEncoderCount += getLeftCount(&msgBuffer);
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_MOTOR_CONTROL_MSG_TYPE);
                break;
            }
        }
    }
}
#endif
