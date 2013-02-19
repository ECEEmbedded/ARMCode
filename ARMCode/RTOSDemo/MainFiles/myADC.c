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
#include "myADC.h"
#include "vtUtilities.h"
#include "vtI2C.h"
#include "lcdTask.h"
#include "myTypes.h"

/* *********************************************** */
// definitions and data structures that are private to this file
// Length of the queue to this task
#define myADCQLen 10
// actual data structure that is sent in a message
typedef struct __myADCMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message to be printed
    uint8_t buf[maxADCMsgLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} myADCMsg;

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations -- almost certainly too large, see LCDTask.c for details on how to check the size
#define baseStack 3
#if PRINTF_VERSION == 1
#define adcSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define adcSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

// end of defs
/* *********************************************** */

/* The ADC task. */
static portTASK_FUNCTION_PROTO( vADCUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartADCTask(myADCStruct *params,unsigned portBASE_TYPE uxPriority, myI2CStruct *myi2c,vtLCDStruct *lcd)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(myADCQLen,sizeof(myADCMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->i2cData = myi2c;
    params->lcdData = lcd;
    if ((retval = xTaskCreate( vADCUpdateTask, ( signed char * ) "ADC", adcSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(retval);
    }
}

portBASE_TYPE SendADCTimerMsg(myADCStruct *adcData,portTickType ticksElapsed,portTickType ticksToBlock)
{
    if (adcData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    myADCMsg adcBuffer;
    adcBuffer.length = sizeof(ticksElapsed);
    if (adcBuffer.length > maxADCMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(adcBuffer.length);
    }
    memcpy(adcBuffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
    adcBuffer.msgType = adcTimerMsg;
    return(xQueueSend(adcData->inQ,(void *) (&adcBuffer),ticksToBlock));
}

portBASE_TYPE SendADCDataMsg(myADCStruct *adcData,uint8_t msgType,uint8_t value,portTickType ticksToBlock)
{
    myADCMsg adcBuffer;

    if (adcData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    adcBuffer.length = sizeof(value);
    if (adcBuffer.length > maxADCMsgLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(adcBuffer.length);
    }
    memcpy(adcBuffer.buf,(char *)&value,sizeof(value));
    adcBuffer.msgType = msgType;
    return(xQueueSend(adcData->inQ,(void *) (&adcBuffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/
int getMsgType(myADCMsg *Buffer)
{
    return(Buffer->msgType);
}

// The below getValue function was changed by Matthew Ibarra on 2/2/2013
void getValue(int *target, myADCMsg *msgBuffer)
{
    *(target) = (int) msgBuffer->buf[0];
}

static myADCStruct *param;
static myI2CStruct *i2cData;
static vtLCDStruct *lcdData;

const uint8_t i2cCmdReadVals[]= {0xAA};

// This is the actual task that is run
static portTASK_FUNCTION( vADCUpdateTask, pvParameters )
{
    // Get the parameters
    param = (myADCStruct *) pvParameters;
    // Get the I2C task pointer
    i2cData = param->i2cData;
    // Get the LCD information pointer
    lcdData = param->lcdData;

    // Buffer for receiving messages
    myADCMsg msgBuffer;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from either a timer or from an I2C operation
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(0);
        }

        // Now, based on the type of the message and the state, we decide on the new state and action to take
        switch(getMsgType(&msgBuffer)) {
	        case ADCMsgTypeTimer: {
	            if (vtI2CEnQ(i2cData,vtI2CMsgTypeADCSend,0x4F,sizeof(i2cCmdReadVals),i2cCmdReadVals,1) != pdTRUE) {
	                VT_HANDLE_FATAL_ERROR(0);
	            }
	            break;
	        }
	        case vtI2CMsgTypeADCSend: {
	            // Read value from ADC and send to LCD thread
	            int value = -1;
	            getValue(&value, &msgBuffer);
	            if(SendLCDADCMsg(lcdData,value,portMAX_DELAY) != pdTRUE) {
	                VT_HANDLE_FATAL_ERROR(0);
	            }
	            break;
	        }
	        default: {
	            VT_HANDLE_FATAL_ERROR(getMsgType(&msgBuffer));
	            break;
	        }
        }
    }
}

