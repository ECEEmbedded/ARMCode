#include "myDefs.h"

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
#define navigationQLen 10
// actual data structure that is sent in a message
typedef struct __navigationMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxNavigationMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} navigationMsg;

/* The Navigation task. */
static portTASK_FUNCTION_PROTO( vNavigationTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartNavigationTask(navigationStruct *params, unsigned portBASE_TYPE uxPriority, motorControlStruct *motorControl, vtLCDStruct *lcdData)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(navigationQLen,sizeof(navigationMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->motorControl = motorControl;
    params->lcdData = lcdData;
    if ((retval = xTaskCreate( vNavigationTask, ( signed char * ) "Navigation", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE AIUpdateDistances(navigationStruct *navData, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t r1, uint8_t r2, uint8_t r3)
{
    if (navData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    navigationMsg buffer;
    buffer.length = 6;
    if (buffer.length > maxNavigationMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_NAVIGATION_MSG_FORMAT);
    }
    buffer.buf[0] = l1;
    buffer.buf[1] = l2;
    buffer.buf[2] = l3;
    buffer.buf[3] = r1;
    buffer.buf[4] = r2;
    buffer.buf[5] = r3;
    buffer.msgType = AIUpdateDistancesMsgType;
    return(xQueueSend(navData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE AIUpdateWallAngles(navigationStruct *navData, uint8_t l, uint8_t r)
{
    if (navData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    navigationMsg buffer;
    buffer.length = 2;
    if (buffer.length > maxNavigationMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_NAVIGATION_MSG_FORMAT);
    }
    buffer.buf[0] = l;
    buffer.buf[1] = r;
    buffer.msgType = AIUpdateWallAnglesMsgType;
    return(xQueueSend(navData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE AIUpdateIsWalls(navigationStruct *navData, uint8_t wallFront, uint8_t wallBack, uint8_t wallLeft, uint8_t wallRight)
{
    if (navData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    navigationMsg buffer;
    buffer.length = 4;
    if (buffer.length > maxNavigationMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_NAVIGATION_MSG_FORMAT);
    }
    buffer.buf[0] = wallFront;
    buffer.buf[1] = wallBack;
    buffer.buf[2] = wallLeft;
    buffer.buf[3] = wallRight;
    buffer.msgType = AIUpdateIsWallsMsgType;
    return(xQueueSend(navData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE AIUpdateFinishLine(navigationStruct *navData, uint8_t finishLine)
{
    if (navData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    navigationMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxNavigationMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_NAVIGATION_MSG_FORMAT);
    }
    buffer.buf[0] = finishLine;
    buffer.msgType = AIUpdateFinishLineMsgType;
    return(xQueueSend(navData->inQ,(void *) (&buffer),portMAX_DELAY));
}

portBASE_TYPE sendNavTimerMsg(navigationStruct *navData, portTickType ticksElapsed, portTickType ticksToBlock)
{
    if (navData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    navigationMsg buffer;
    buffer.length = sizeof(ticksElapsed);
    if (buffer.length > maxNavigationMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(buffer.length);
    }
    memcpy(buffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
    buffer.msgType = AITimerMsgType;
    return(xQueueSend(navData->inQ,(void *) (&buffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/

// Private routines used to unpack the message buffers.
// I do not want to access the message buffer data structures outside of these routines.
// These routines are specific to accessing our packet protocol from the task struct.

// For accessing data sent between ARM local tasks:

// When AIUpdateDistancesMsgType type, l1 is in buf[0]
uint8_t getUpdatedDistance_L1_Info(navigationMsg *navBuf){
    return navBuf->buf[0];
}

// When AIUpdateDistancesMsgType type, l2 is in buf[1]
uint8_t getUpdatedDistance_L2_Info(navigationMsg *navBuf){
    return navBuf->buf[1];
}

// When AIUpdateDistancesMsgType type, l3 is in buf[2]
uint8_t getUpdatedDistance_L3_Info(navigationMsg *navBuf){
    return navBuf->buf[2];
}

// When AIUpdateDistancesMsgType type, r1 is in buf[3]
uint8_t getUpdatedDistance_R1_Info(navigationMsg *navBuf){
    return navBuf->buf[3];
}

// When AIUpdateDistancesMsgType type, r2 is in buf[4]
uint8_t getUpdatedDistance_R2_Info(navigationMsg *navBuf){
    return navBuf->buf[4];
}

// When AIUpdateDistancesMsgType type, r3 is in buf[5]
uint8_t getUpdatedDistance_R3_Info(navigationMsg *navBuf){
    return navBuf->buf[5];
}

// When AIUpdateWallAnglesMsgType type, l is in buf[0]
uint8_t getUpdatedWallAngle_L_Info(navigationMsg *navBuf){
    return navBuf->buf[0];
}

// When AIUpdateWallAnglesMsgType type, r is in buf[1]
uint8_t getUpdatedWallAngle_R_Info(navigationMsg *navBuf){
    return navBuf->buf[1];
}

// When AIUpdateIsWallsMsgType type, wallFront is in buf[0]
uint8_t getUpdateIsWall_F_Info(navigationMsg *navBuf){
    return navBuf->buf[0];
}

// When AIUpdateIsWallsMsgType type, wallBack is in buf[1]
uint8_t getUpdateIsWall_B_Info(navigationMsg *navBuf){
    return navBuf->buf[1];
}

// When AIUpdateIsWallsMsgType type, wallLeft is in buf[2]
uint8_t getUpdateIsWall_L_Info(navigationMsg *navBuf){
    return navBuf->buf[2];
}

// When AIUpdateIsWallsMsgType type, wallRight is in buf[3]
uint8_t getUpdateIsWall_R_Info(navigationMsg *navBuf){
    return navBuf->buf[3];
}

// When AIUpdateFinishLineMsgType type, finishLine is in buf[0]
uint8_t getUpdatedFinishLineInfo(navigationMsg *navBuf){
    return navBuf->buf[0];
}

// For accessing data sent between Rover PIC(s) and the ARM:

/**     N/A     **/
// This means no data goes straight from Rover PIC(s) to the web server

// End of private routines for message buffers
/*-----------------------------------------------------------*/

// Private routines used for data manipulation, etc.
// There should be NO accessing of our packet protocol from the task struct in these routines.

int getMsgType(navigationMsg *navBuf)
{
    return(navBuf->msgType);
}

// End of private routines for data manipulation, etc.
/*-----------------------------------------------------------*/

static navigationStruct *param;
static motorControlStruct *motorControl;
static vtLCDStruct *lcdData;

// Buffer for receiving messages
static navigationMsg msgBuffer;

// This is the actual task that is run
static portTASK_FUNCTION( vNavigationTask, pvParameters )
{
    // Get the parameters
    param = (navigationStruct *) pvParameters;
    // Get the Motor Control task pointer
    motorControl = param->motorControl;
    // Get the LCD task pointer
    lcdData = param->lcdData;

    int timerCount = -20;
    int roverSpeed = 34;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from whomever we decide will need to talk to this task
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(getMsgType(&msgBuffer)){
            case AITimerMsgType:
            {
                if(timerCount != 250){
                    timerCount++;
                }
                if(timerCount < 0){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount >= 10 && timerCount <= 20){
                    sendMotorStop(motorControl);
                }
                else if(timerCount < 25){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount == 25){
                    sendMotorTurnRight(motorControl, 10);
                }
                else if(timerCount < 30){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount == 30){
                    roverSpeed *= 2;
                    sendMotorSetSpeed(motorControl, roverSpeed);
                }
                else if(timerCount < 45){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount >= 45 && timerCount < 55){
                    sendMotorStop(motorControl);
                }
                else if(timerCount == 55){
                    roverSpeed /= 2;
                    sendMotorSetSpeed(motorControl, roverSpeed);
                }
                else if(timerCount < 65){
                    sendMotorSetDirReverse(motorControl);
                }
                else if(timerCount >= 75 && timerCount <= 80){
                    sendMotorStop(motorControl);
                }
                else if(timerCount < 85){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount == 95){
                    sendMotorTurnLeft(motorControl, 20);
                }
                else if(timerCount < 100){
                    sendMotorSetDirForward(motorControl);
                }
                else if(timerCount == 100){
                    sendMotorStop(motorControl);
                    timerCount = 250;
                }
                break;
            }
            case AIUpdateDistancesMsgType:
            {
                break;
            }
            case AIUpdateWallAnglesMsgType:
            {
                break;
            }
            case AIUpdateIsWallsMsgType:
            {
                break;
            }
            case AIUpdateFinishLineMsgType:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_NAVIGATION_MSG_TYPE);
                break;
            }
        }
    }
}
