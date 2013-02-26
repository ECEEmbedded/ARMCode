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
#include "LCDtask.h"
#include "i2c_milestone2.h"
#include "types_milestone2.h"

#define baseStack 3
#if PRINTF_VERSION == 1
#define i2cSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define i2cSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

/* *********************************************** */
// definitions and data structures that are private to this file
// Length of the queue to this task
#define vti2cQLen 10
// actual data structure that is sent in a message
typedef struct __myi2cMsg {
	uint8_t msgType;
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[vti2cMaxLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} myi2cMsg;

// end of defs
/* *********************************************** */

/* The i2cTemp task. */
static portTASK_FUNCTION_PROTO( vi2cUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void starti2cTask(myI2CStruct *params,unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c,vtLCDStruct *lcd)
{
	// Create the queue that will be used to talk to this task
	if ((params->inQ = xQueueCreate(vti2cQLen,sizeof(myi2cMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	params->dev = i2c;
	params->lcdData = lcd;
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

portBASE_TYPE sendi2cValueMsg(myI2CStruct *i2cData,uint8_t msgType,uint8_t* value, uint8_t length, portTickType ticksToBlock)
{
	myi2cMsg buffer;

	if (i2cData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	buffer.length = length;
	if (length > vti2cMaxLen) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(buffer.length);
	}
	memcpy(buffer.buf,(char *)value,length);
	buffer.msgType = msgType;
	return(xQueueSend(i2cData->inQ,(void *) (&buffer),ticksToBlock));
}

// End of Public API
/*-----------------------------------------------------------*/
int getMsgType(myi2cMsg *buffer)
{
	return(buffer->msgType);
}

uint8_t geti2cType(myi2cMsg *buffer){
	return buffer->buf[0];
}

uint8_t geti2cCount(myi2cMsg *buffer){
	return buffer->buf[1];
}

int geti2cADCValue(myi2cMsg *buffer){
	return (((int)buffer->buf[3])<<8)|(int)buffer->buf[2];
}

// I2C commands for the ADC
	const uint8_t i2cCmdInit[]= {0xAC,0x00};
	const uint8_t i2cCmdStartConvert[]= {0xEE};
	const uint8_t i2cCmdReadVals[]= {0xAA};
// end of I2C command definitions
// This is the actual task that is run
static portTASK_FUNCTION( vi2cUpdateTask, pvParameters )
{
	// Get the parameters
	myI2CStruct *param = (myI2CStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the LCD information pointer
	vtLCDStruct *lcdData = param->lcdData;
	// Buffer for receiving messages
	myi2cMsg msgBuffer;

	uint8_t sent2680ADCCount = 0;
	uint8_t sent26J50ADCCount = 0;
	uint8_t received2680ADCCount = 0;
	uint8_t received26J50ADCCount = 0;

	// Assumes that the I2C device (and thread) have already been initialized

	//Send initialization message
//	if (vtI2CEnQ(devPtr,vtI2CMsgTypeInit,0x4F,sizeof(i2cCmdInit),i2cCmdInit,0) != pdTRUE) {
//		VT_HANDLE_FATAL_ERROR(0);
//	}
	int pic2680reqSent = 0;
	int pic26J50reqSent = 0;
	int boardToPoll = PIC2680;
	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from either a timer or from an I2C operation
		if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		switch(getMsgType(&msgBuffer)) {
	//	case vtI2CMsgTypeInit: {
	//		if(pic2680init == 0){
	//			pic2680init = 1;
	//			if (vtI2CEnQ(devPtr,vtI2CMsgTypeInit,0x4E,sizeof(i2cCmdInit),i2cCmdInit,0) != pdTRUE) {
	//				VT_HANDLE_FATAL_ERROR(0);
	//			}
	//		}
	//		break;
	//	}
		case i2cMsgTypeTimer: {
			switch(boardToPoll){
			case PIC2680:{
				sent2680ADCCount++;
				if(pic2680reqSent != 0){
					if(SendLCDErrorMsg(lcdData, PIC2680_ERROR, portMAX_DELAY) != pdTRUE){
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				pic2680reqSent = 1;
				if (vtI2CEnQ(devPtr,vtI2CMsgTypeRead,0x4F,sizeof(i2cCmdReadVals),i2cCmdReadVals,ADC_MSG_SIZE) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				boardToPoll = PIC26J50;
				break;
			}
			case PIC26J50:{
				sent26J50ADCCount++;
				if(pic26J50reqSent != 0){
					if(SendLCDErrorMsg(lcdData, PIC26J50_ERROR, portMAX_DELAY) != pdTRUE){
						VT_HANDLE_FATAL_ERROR(0);
					}
				}
				pic26J50reqSent = 1;
				if (vtI2CEnQ(devPtr,vtI2CMsgTypeRead,0x4E,sizeof(i2cCmdReadVals),i2cCmdReadVals,ADC_MSG_SIZE) != pdTRUE) {
					VT_HANDLE_FATAL_ERROR(0);
				}
				boardToPoll = PIC2680;
				break;
			}
			break;
			}
		}
		case vtI2CMsgTypeRead: {
			// Read value from ADC and send to LCD thread
			uint8_t type = geti2cType(&msgBuffer);
			uint8_t count = geti2cCount(&msgBuffer);
			switch(type){
				case PIC2680_ADC_MESSAGE:{
					received2680ADCCount++;
					int value = geti2cADCValue(&msgBuffer);
					uint8_t errCount = count - received2680ADCCount;
					received2680ADCCount = count;
					pic2680reqSent = 0;
					if(SendLCDADCMsg(lcdData,value,type,errCount, portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
					break;
				}
				case PIC26J50_ADC_MESSAGE:{
					received26J50ADCCount++;
					int value = geti2cADCValue(&msgBuffer);
					uint8_t errCount = count - received26J50ADCCount;
					received26J50ADCCount = count;
					pic26J50reqSent = 0;
					if(SendLCDADCMsg(lcdData,value,type,errCount, portMAX_DELAY) != pdTRUE) {
						VT_HANDLE_FATAL_ERROR(0);
					}
					break;
				}
				case PIC2680_EMPTY_MESSAGE:{
					pic2680reqSent = 0;
					break;
				}
				case PIC26J50_EMPTY_MESSAGE:{
					pic26J50reqSent	= 0;
					break;
				}
				default:{
				//handle error
				break;
				}
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
#endif