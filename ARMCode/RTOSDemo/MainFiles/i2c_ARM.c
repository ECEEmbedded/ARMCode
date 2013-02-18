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
#include "vtI2C.h"
#include "i2c_ARM.h"
#include "myTypes.h"

#define baseStack 3
#if PRINTF_VERSION == 1
#define i2cSTACK_SIZE       ((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define i2cSTACK_SIZE       (baseStack*configMINIMAL_STACK_SIZE)
#endif

/* *********************************************** */
// definitions and data structures that are private to this file
// Length of the queue to this task
#define vti2cQLen 10
// actual data structure that is sent in a message
typedef struct __myi2cMsg {
    uint8_t msgType;
    uint8_t length;  // Length of the message to be printed
    uint8_t buf[vti2cMaxLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} myi2cMsg;

// end of defs
/* *********************************************** */

/* The i2c_ARM task. */
static portTASK_FUNCTION_PROTO( vi2cUpdateTask, pvParameters );

uint8_t requestSent = 0;

/*-----------------------------------------------------------*/
// Public API
void starti2cTask(myI2CStruct *params,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c)
{
    // Create the queue that will be used to talk to this task
    if ((params->inQ = xQueueCreate(vti2cQLen,sizeof(myi2cMsg))) == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    /* Start the task */
    portBASE_TYPE retval;
    params->dev = i2c;
    if ((retval = xTaskCreate( vi2cUpdateTask, ( signed char * ) "i2c", i2cSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
        VT_HANDLE_FATAL_ERROR(retval);
    }
}

portBASE_TYPE sendi2cTimerMsg(myI2CStruct *i2cData,portTickType ticksElapsed,portTickType ticksToBlock)
{
    if (i2cData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    myi2cMsg buffer;
    buffer.length = sizeof(ticksElapsed);
    if (buffer.length > vti2cMaxLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(buffer.length);
    }
    memcpy(buffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
    buffer.msgType = i2cMsgTypeTimer;
    return(xQueueSend(i2cData->inQ,(void *) (&buffer),ticksToBlock));
}

portBASE_TYPE sendi2cMotorMsg(myI2CStruct *i2cData,uint8_t rightValue,uint8_t leftValue,portTickType ticksToBlock)
{
    if (i2cData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    myi2cMsg buffer;
    buffer.length = 4;
    if (buffer.length > vti2cMaxLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_I2C_MSG_FORMAT);
    }
    buffer.buf[0] = 0xBB;
    buffer.buf[1] = 0x00;
    buffer.buf[2] = leftValue;
    buffer.buf[3] = rightValue;
    buffer.msgType = vtI2CMsgTypeMotor;
    return(xQueueSend(i2cData->inQ,(void *) (&buffer),ticksToBlock));
}

// portBASE_TYPE sendi2cADCMsg(myI2CStruct *i2cData,uint8_t msgType,uint8_t value, portTickType ticksToBlock) {
//     if (i2cData == NULL) {
//         VT_HANDLE_FATAL_ERROR(0);
//     }
//     myi2cMsg buffer;
//     buffer.length = 4;
//     if (buffer.length > vti2cMaxLen) {
//         // no room for this message
//         VT_HANDLE_FATAL_ERROR(INCORRECT_I2C_MSG_FORMAT);
//     }
//     buffer.buf[0] = 0xBB;
//     buffer.buf[1] = 0x00;
//     buffer.buf[2] = leftValue;
//     buffer.buf[3] = rightValue;
//     buffer.msgType = vtI2CMsgTypeADC;
//     return(xQueueSend(i2cData->inQ,(void *) (&buffer),ticksToBlock));
// }

portBASE_TYPE notifyRequestRecvd(myI2CStruct *i2cData,portTickType ticksToBlock)
{
    if (i2cData == NULL) {
        VT_HANDLE_FATAL_ERROR(0);
    }
    myi2cMsg buffer;
    buffer.length = 0;
    if (buffer.length > vti2cMaxLen) {
        // no room for this message
        VT_HANDLE_FATAL_ERROR(INCORRECT_I2C_MSG_FORMAT);
    }
    buffer.msgType = notifyRqstRecvdType;
    return(xQueueSend(i2cData->inQ,(void *) (&buffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/

void notifyRequestSent(){
    if(requestSent == 1){
        // Send I2C Error Message to Web Server
    }
    requestSent = 1;
}

uint8_t getMsgType(myi2cMsg *buffer)
{
    return(buffer->msgType);
}

// This is the actual task that is run
static portTASK_FUNCTION( vi2cUpdateTask, pvParameters )
{
    // Get the parameters
    myI2CStruct *param = (myI2CStruct *) pvParameters;
    // Get the I2C device pointer
    vtI2CStruct *devPtr = param->dev;
    // Buffer for receiving messages
    myi2cMsg msgBuffer;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from either a timer or from an I2C operation
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(getMsgType(&msgBuffer)) {
            case i2cMsgTypeTimer: {
                //Poll local 2680 for data
                notifyRequestSent();
                if (vtI2CEnQ(devPtr,vtI2CMsgTypeRead,SLAVE_ADDR,0,0,I2C_MSG_SIZE) != pdTRUE) {
                    VT_HANDLE_FATAL_ERROR(VT_I2C_Q_FULL);
                }
                break;
            }
            case vtI2CMsgTypeMotor: {
                //Send motor command to local 2680
                if (vtI2CEnQ(devPtr,vtI2CMsgTypeMotor,SLAVE_ADDR,msgBuffer.length,msgBuffer.buf,0) != pdTRUE){
                    VT_HANDLE_FATAL_ERROR(VT_I2C_Q_FULL);
                }
                break;
            }
            case vtI2CMsgTypeADC: {
                if (vtI2CEnQ(devPtr,vtI2CMsgTypeADC,SLAVE_ADDR,msgBuffer.length,msgBuffer.buf,0) != pdTRUE){
                    VT_HANDLE_FATAL_ERROR(VT_I2C_Q_FULL);
                }
                break;
            }
            case notifyRqstRecvdType: {
                if(requestSent == 0){
                    // Send I2C Error Message to Web Server
                }
                requestSent = 0;
                break;
            }
            default: {
                VT_HANDLE_FATAL_ERROR(UNKNOWN_I2C_MSG_TYPE);
                break;
            }
        }
    }
}
#endif