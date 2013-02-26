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
#include "myTypes.h"
#include "motorControl.h"
#include "navigation.h"
#include "lcdTask.h"
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

portBASE_TYPE AIUpdateDistences(navigationStruct *navData, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t r1, uint8_t r2, uint8_t r3)
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
    buffer.msgType = AIUpdateDistancesMsg;
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

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

uint8_t getCentimeters(navigationMsg *buffer){
    return buffer->buf[0];
}

uint8_t getDegrees(navigationMsg *buffer){
    return buffer->buf[0];
}

// Here is where the declaration of any custom #define statements occurs:
// ...

#define None 0
#define MoveForward 1
#define RotateClockwise 2
#define RotateCounterClockwise 3
#define MoveBackward 4

// Here is where the declaration of any necessary variables occurs:
// ...

static navigationStruct *param;
static motorControlStruct *motorControl;
static vtLCDStruct *lcdData;
static navigationMsg msgBuffer;

// This is the actual task that is run
static portTASK_FUNCTION( vNavigationTask, pvParameters )
{
    // Get the parameters
    param = (navigationStruct *) pvParameters;
    // Get the other necessary tasks' task pointers like this:
    // Get the Motor Control task pointer
    motorControl = param->motorControl;
    // Get the LCD task pointer
    lcdData = param->lcdData;

    // Initialize variables you declared above this function if necessary
    // ...

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from whomever we decide will need to talk to this task
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(msgBuffer.msgType){
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
#endif
