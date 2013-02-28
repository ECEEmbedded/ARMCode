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
/* *********************************************** */

#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// Length of the queue to this task
#define powerQLen 10
// actual data structure that is sent in a message
typedef struct __powerMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxPowerMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} powerMsg;

/* The Navigation task. */
static portTASK_FUNCTION_PROTO( vPowerTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API

void vStartPowerTask(powerStruct *params, unsigned portBASE_TYPE uxPriority)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(powerQLen,sizeof(powerMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    if ((retval = xTaskCreate( vPowerTask, ( signed char * ) "Power", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE conductorSendPowerDataMsg(powerStruct *powerData, uint8_t *data, uint8_t length)
{
    if (powerData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    powerMsg buffer;
    buffer.length = length;
    if (buffer.length > maxPowerMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_POWER_MSG_FORMAT);
    }
    memcpy(buffer.buf,data,length);
    buffer.msgType = powerDataMsg;
    return(xQueueSend(powerData->inQ,(void *) (&buffer),portMAX_DELAY));
}

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

// Here is where the declaration of any custom #define statements occurs:
// ...

// Here is where the declaration of any necessary variables occurs:
// ...
static powerStruct *param;
// Buffer for receiving messages
static powerMsg msgBuffer;

static unsigned int t;
static int x;
static int y;

// This is the actual task that is run
static portTASK_FUNCTION( vPowerTask, pvParameters )
{
    // Get the parameters
    param = (powerStruct *) pvParameters;
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
            case powerDataMsg:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_POWER_MSG_TYPE);
                break;
            }
        }
    }
}

#endif