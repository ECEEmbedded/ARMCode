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
#include "navigation.h"
#include "mapping.h"
#include "speedLimit.h"
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
#define mappingQLen 10
// actual data structure that is sent in a message
typedef struct __mappingMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message
    uint8_t buf[maxMappingMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} mappingMsg;

/* The Navigation task. */
static portTASK_FUNCTION_PROTO( vMappingTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartMappingTask(mappingStruct *params,unsigned portBASE_TYPE uxPriority, navigationStruct *navData, speedLimitControlStruct *speedData, vtLCDStruct *lcdData)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(mappingQLen,sizeof(mappingMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->navData = navData;
    params->speedData = speedData;
    params->lcdData = lcdData;
    if ((retval = xTaskCreate( vMappingTask, ( signed char * ) "Mapping", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(TASK_CREATION_ERROR);
    }
}

portBASE_TYPE sendAngleMsg(mappingStruct *mapData,uint8_t degrees)
{
    if (mapData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    mappingMsg buffer;
    buffer.length = 1;
    if (buffer.length > maxMappingMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_MAPPING_MSG_FORMAT);
    }
    buffer.buf[0] = degrees;
    buffer.msgType = angleMsg;
    return(xQueueSend(mapData->inQ,(void *) (&buffer),portMAX_DELAY));
}

// End of Public API
/*-----------------------------------------------------------*/

// Here is where the declaration of any custom helper functions occurs:
// ...

// Here is where the declaration of any custom #define statements occurs:
// ...

// Here is where the declaration of any necessary variables occurs:
// ...
static unsigned int t;
static int x;
static int y;

// This is the actual task that is run
static portTASK_FUNCTION( vMappingTask, pvParameters )
{
    // Get the parameters
    mappingStruct *param = (mappingStruct *) pvParameters;
    // Get the other necessary tasks' task pointers like this:
    navigationStruct *navData = param->navData;
    // Repeat as necessary
    // ...
    // Buffer for receiving messages
    mappingMsg msgBuffer;

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
            case angleMsg:
            {
                break;
            }
            default:
            {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_MAPPING_MSG_TYPE);
                break;
            }
        }
    }
}
#endif
