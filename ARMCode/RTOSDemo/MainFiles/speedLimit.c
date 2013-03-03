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
#include "motorControl.h"
#include "navigation.h"
#include "speedLimit.h"
/* *********************************************** */

#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// Length of the queue to this task
#define speedQLen 10
// actual data structure that is sent in a message
typedef struct __speedLimitMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxSpeedLimitMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} speedLimitMsg;

/* The Speed Limit Control task. */
static portTASK_FUNCTION_PROTO( vSpeedLimitTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartSpeedLimitTask(speedLimitControlStruct *params, unsigned portBASE_TYPE uxPriority, motorControlStruct *motorControl, navigationStruct *navData, webServerStruct *webData)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(speedQLen,sizeof(speedLimitMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->motorControl = motorControl;
    params->navData = navData;
    params->webData = webData;
    if ((retval = xTaskCreate( vSpeedLimitTask, ( signed char * ) "Speed Limit", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE conductorSendColorSensorDataMsg(speedLimitControlStruct *speedData,uint8_t *data, uint8_t length)
{
    if (speedData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    speedLimitMsg buffer;
    buffer.length = length;
    if (buffer.length > maxSpeedLimitMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_SPEED_LIMIT_MSG_FORMAT);
    }
    memcpy(buffer.buf,data,length);
    buffer.msgType = colorSensorDataMsgType;
    return(xQueueSend(speedData->inQ,(void *) (&buffer),portMAX_DELAY));
}

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

// Here is where the declaration of any custom #define statements occurs:
// ...

// Here is where the declaration of any necessary variables occurs:
// ...
static speedLimitControlStruct *param;
static motorControlStruct *motorControl;
static navigationStruct *navData;
static webServerStruct *webData;

// Buffer for receiving messages
static speedLimitMsg msgBuffer;

// This is the actual task that is run
static portTASK_FUNCTION( vSpeedLimitTask, pvParameters )
{
    // Get the parameters
    param = (speedLimitControlStruct *) pvParameters;
    // Get the other necessary tasks' task pointers like this:
    // Get the Motor Control task pointer
    motorControl = param->motorControl;
    // Get the Navigation task pointer
    navData = param->navData;
    // Get the Web Server task pointer
    webData = param->webData;
    // Repeat as necessary
    // ...

    // Initialize variables you declared above this function if necessary
    // ...

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from whomever we decide will need to talk to this task
        // More than likely, this will involve I2C (Encoder data) or the Navigation Task (motor command)
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(msgBuffer.msgType){
            case colorSensorDataMsgType:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_SPEED_LIMIT_MSG_TYPE);
                break;
            }
        }
    }
}
#endif
