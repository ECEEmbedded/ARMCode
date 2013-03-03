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
#include "irControl.h"
#include "myTypes.h"
#include "navigation.h"
/* *********************************************** */

#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// Length of the queue to this task
#define irQLen 10
// actual data structure that is sent in a message
typedef struct __irMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxIRMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} irMsg;

/* The Navigation task. */
static portTASK_FUNCTION_PROTO( vIRTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartIRTask(irControlStruct *params, unsigned portBASE_TYPE uxPriority, navigationStruct *navData)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(irQLen,sizeof(irMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->navData = navData;
    if ((retval = xTaskCreate( vIRTask, ( signed char * ) "IR", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE conductorSendIRSensorDataMsg(irControlStruct *irData, uint8_t *data, uint8_t length)
{
    if (irData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    irMsg buffer;
    buffer.length = length;
    if (buffer.length > maxIRMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_IR_MSG_FORMAT);
    }
    memcpy(buffer.buf, data, length);
    buffer.msgType = irDataMsgType;
    return(xQueueSend(irData->inQ,(void *) (&buffer),portMAX_DELAY));
}

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

// Here is where the declaration of any custom #define statements occurs:
// ...


// Here is where the declaration of static task pointers occurs; they will be initialized below.
static irControlStruct *param;
static navigationStruct *navData;

// Buffer for receiving messages - declaration
static irMsg msgBuffer;

// Here is where the declaration of any necessary variables occurs:
// ...
static unsigned int t;
static int x;
static int y;

// This is the actual task that is run
static portTASK_FUNCTION( vIRTask, pvParameters )
{
    // Get the parameters
    param = (irControlStruct *) pvParameters;
    // Get the other necessary tasks' task pointers like this:
    navData = param->navData;
    // Repeat as necessary
    // ...

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
            case irDataMsgType:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_IR_MSG_TYPE);
                break;
            }
        }
    }
}
#endif
