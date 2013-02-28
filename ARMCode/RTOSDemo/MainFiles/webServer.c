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
#include "webServer.h"
/* *********************************************** */

#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// Length of the queue to this task
#define webServerQLen 10
// actual data structure that is sent in a message
typedef struct __webServerMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxWebServerMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} webServerMsg;

/* The Web Server task. */
static portTASK_FUNCTION_PROTO( vWebServerTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartWebServerTask(webServerStruct *params, unsigned portBASE_TYPE uxPriority)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(webServerQLen,sizeof(webServerMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    if ((retval = xTaskCreate( vWebServerTask, ( signed char * ) "Web Server", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

//Notify the web server of the current speed, 0xFF is the maximum
portBASE_TYPE webNotifyCurrentSpeed(webServerStruct *webServerData, uint8_t speed)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = speed;
    buffer.msgType = webNotifyCurrentSpeedMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

//Notify the web server of the current speed limit zone
// uint8_t "zone" value corresponds to current speed limit zone
portBASE_TYPE webNotifySpeedLimitZone(webServerStruct *webServerData, uint8_t zone)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = zone;
    buffer.msgType = webNotifySpeedLimitZoneMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

//Notify the web server that the finish line is crossed
//***finish value is boolean!!!!
portBASE_TYPE webNotifyFinishLine(webServerStruct *webServerData, uint8_t finish)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = finish;
    buffer.msgType = webNotifyFinishLineMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

//Notify the web server of the current power requirements
portBASE_TYPE webNotifyPower(webServerStruct *webServerData, uint8_t watts)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = watts;
    buffer.msgType = webNotifyPowerMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

//Notify the web server the fastest time the rover could complete the course
//with the data gathered.
portBASE_TYPE webNotifyFastestTime(webServerStruct *webServerData, uint8_t fastMin, uint8_t fastSec)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 2;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = fastMin;
    buffer.buf[1] = fastSec;
    buffer.msgType = webNotifyFastestTimeMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

//Notify the web server when a speed limit violation is occurring
//***speedViolation value is boolean!!!!
portBASE_TYPE webNotifySpeedViolation(webServerStruct *webServerData, uint8_t speedViolation)
{
    if (webServerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    webServerMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxWebServerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_WEB_SERVER_MSG_FORMAT);
    }
    buffer.buf[0] = speedViolation;
    buffer.msgType = webNotifySpeedViolationMsgType;
    return(xQueueSend(webServerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

// Here is where the declaration of any custom #define statements occurs:
// ...

// Here is where the declaration of any necessary variables occurs:
// ...
static webServerStruct *param;
// Buffer for receiving messages
webServerMsg msgBuffer;

static unsigned int t;
static int x;
static int y;

// This is the actual task that is run
static portTASK_FUNCTION( vWebServerTask, pvParameters )
{
    // Get the parameters
    param = (webServerStruct *) pvParameters;
    // Get any other necessary tasks' task pointers (but this task shouldn't need any more)

    // Initialize variables you declared above this function
    t = 0;
    x = 0;
    y = 0;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from whomever we decide will need to talk to this task
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(msgBuffer.msgType){
            case webNotifyCurrentSpeedMsgType:
            {
                break;
            }
            case webNotifySpeedLimitZoneMsgType:
            {
                break;
            }
            case webNotifyFinishLineMsgType:
            {
                break;
            }
            case webNotifyPowerMsgType:
            {
                break;
            }
            case webNotifyFastestTimeMsgType:
            {
                break;
            }
            case webNotifySpeedViolationMsgType:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_WEB_SERVER_MSG_TYPE);
                break;
            }
        }
    }
}

#endif