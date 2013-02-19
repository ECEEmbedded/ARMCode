#include "myDefs.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* include files. */
#include "GLCD.h"
#include "vtUtilities.h"
#include "lcdTask.h"
#include "string.h"
#include "myTypes.h"

// I have set this to a larger stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the LCD operations
// I actually monitor the stack size in the code to check to make sure I'm not too close to overflowing the stack
//   This monitoring takes place if INPSECT_STACK is defined (search this file for INSPECT_STACK to see the code for this)
#define INSPECT_STACK 1
#define baseStack 3
#if PRINTF_VERSION == 1
#define lcdSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define lcdSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif

// definitions and data structures that are private to this file
// Length of the queue to this task
#define vtLCDQLen 10

static unsigned int pic2680MsgDropCount = 0;
static unsigned int pic26J50MsgDropCount = 0;
static unsigned int pic2680errorCount = 0;
static unsigned int pic26J50errorCount = 0;

/* definition for the LCD task. */
static portTASK_FUNCTION_PROTO( vLCDUpdateTask, pvParameters );

/*-----------------------------------------------------------*/

void StartLCDTask(vtLCDStruct *ptr, unsigned portBASE_TYPE uxPriority)
{
	if (ptr == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}

	// Create the queue that will be used to talk to this task
	if ((ptr->inQ = xQueueCreate(vtLCDQLen,sizeof(vtLCDMsg))) == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	/* Start the task */
	portBASE_TYPE retval;
	if ((retval = xTaskCreate( vLCDUpdateTask, ( signed char * ) "LCD", lcdSTACK_SIZE, (void*)ptr, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

// portBASE_TYPE SendLCDTimerMsg(vtLCDStruct *lcdData,portTickType ticksElapsed,portTickType ticksToBlock)
// {
// 	if (lcdData == NULL) {
// 		VT_HANDLE_FATAL_ERROR(0);
// 	}
// 	vtLCDMsg lcdBuffer;
// 	lcdBuffer.length = sizeof(ticksElapsed);
// 	if (lcdBuffer.length > vtLCDMaxLen) {
// 		// no room for this message
// 		VT_HANDLE_FATAL_ERROR(lcdBuffer.length);
// 	}
// 	memcpy(lcdBuffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
// 	lcdBuffer.msgType = LCDMsgTypeTimer;
// 	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
// }

// portBASE_TYPE SendLCDPrintMsg(vtLCDStruct *lcdData,int length,char *pString,portTickType ticksToBlock)
// {
// 	if (lcdData == NULL) {
// 		VT_HANDLE_FATAL_ERROR(0);
// 	}
// 	vtLCDMsg lcdBuffer;

// 	if (length > vtLCDMaxLen) {
// 		// no room for this message
// 		VT_HANDLE_FATAL_ERROR(lcdBuffer.length);
// 	}
// 	lcdBuffer.length = strnlen(pString,vtLCDMaxLen);
// 	lcdBuffer.msgType = LCDMsgTypePrint;
// 	strncpy((char *)lcdBuffer.buf,pString,vtLCDMaxLen);
// 	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
// }

portBASE_TYPE SendLCDADCMsgvtLCDStruct *lcdData,int data, uint8_t type, uint8_t errCount, portTickType ticksToBlock)
{
	if (lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	vtLCDMsg lcdBuffer;

	lcdBuffer.length = sizeof(data);
	lcdBuffer.msgType = type;
	switch(type){
		case PIC2680_ADC_MESSAGE:{
			pic2680MsgDropCount = pic2680MsgDropCount + errCount;
			break;
		}
		case PIC26J50_ADC_MESSAGE:{
			pic26J50MsgDropCount = pic26J50MsgDropCount + errCount;
			break;
		}
		default:{
		}
	}
	lcdBuffer.buf[0] = (uint8_t) data;
	lcdBuffer.buf[1] = (uint8_t)(data>>8);
	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
}

portBASE_TYPE SendLCDErrorMsg(vtLCDStruct *lcdData, uint8_t type, portTickType ticksToBlock)
{
	if (lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	vtLCDMsg lcdBuffer;
	lcdBuffer.msgType = type;
	switch(type){
		case PIC2680_ADC_MESSAGE:{
			pic2680errorCount++;
			break;
		}
		case PIC26J50_ADC_MESSAGE:{
			pic26J50errorCount++;
			break;
		}
		default:{
		}
	}
	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
}

// Private routines used to unpack the message buffers
//   I do not want to access the message buffer data structures outside of these routines
// portTickType unpackTimerMsg(vtLCDMsg *lcdBuffer)
// {
// 	portTickType *ptr = (portTickType *) lcdBuffer->buf;
// 	return(*ptr);
// }

int getMsgType(vtLCDMsg *lcdBuffer)
{
	return(lcdBuffer->msgType);
}

int getMsgValue(vtLCDMsg *lcdBuffer)
{
	return((int)(lcdBuffer->buf[1]<<8)) | ((int) lcdBuffer->buf[0]);
}

void adcIntToString(int value, unsigned char *returnVal){
	returnVal[0] = (value/0xCC) + 48;	//Ones Place
	returnVal[1] = '.';
	returnVal[2] = (((10*value)/0xCC)%10) + 48; //Tenths place
	returnVal[3] = ((((100*value)/0xCC)%100)%10) + 48; //Hundredths place
	returnVal[4] = 0;
}

void errorCountIntToString(int value, unsigned char *returnVal){
	int index = 0;
	int place;
	int zeroSoFar = 0;
	for(place = 1000000; place > 0; place = place/10){
		returnVal[index] = ((value/place)%10) + 48;
		if((returnVal[index] == 48) && (zeroSoFar == 0)){
			returnVal[index] = ' ';
		}else{
			zeroSoFar = 1;
		}
		index++;
	}
	returnVal[8] = 0;
}

// int getMsgLength(vtLCDMsg *lcdBuffer)
// {
// 	return(lcdBuffer->msgType);
// }

// void copyMsgString(char *target,vtLCDMsg *lcdBuffer,int targetMaxLen)
// {
// 	strncpy(target,(char *)(lcdBuffer->buf),targetMaxLen);
// }

// End of private routines for message buffers

// This is the actual task that is run
static portTASK_FUNCTION( vLCDUpdateTask, pvParameters )
{
	unsigned short screenColor = 0;
	unsigned short tscr;

	vtLCDMsg msgBuffer;
	vtLCDStruct *lcdPtr = (vtLCDStruct *) pvParameters;

	/* Initialize the LCD and set the initial colors */
	GLCD_Init();
	tscr = Black; // may be reset in the LCDMsgTypeTimer code below
	screenColor = White; // may be reset in the LCDMsgTypeTimer code below
	GLCD_SetTextColor(tscr);
	GLCD_SetBackColor(screenColor);
	GLCD_Clear(screenColor);

	//Set up constant text fields
	GLCD_DisplayString(PIC2680_LINE,0,1, (unsigned char*) "2680 ADC:");
	GLCD_DisplayString(PIC2680_LINE,15,1,(unsigned char*) "V");
	GLCD_DisplayString(PIC2680_LINE + 1,0,1, (unsigned char*) "Msgs Missed:      0");
	GLCD_DisplayString(PIC2680_LINE + 2,0,1, (unsigned char*) "Rqsts Drop:       0");
	GLCD_DisplayString(PIC26J50_LINE,0,1, (unsigned char*) "26J50 ADC:");
	GLCD_DisplayString(PIC26J50_LINE,16,1,(unsigned char*) "V");
	GLCD_DisplayString(PIC26J50_LINE + 1,0,1, (unsigned char*) "Msgs Missed:      0");
	GLCD_DisplayString(PIC26J50_LINE + 2,0,1, (unsigned char*) "Rqsts Drop:       0");

	// This task should never exit
	for(;;)
	{
		// Wait for a message
		if (xQueueReceive(lcdPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		switch(getMsgType(&msgBuffer)){
			case PIC2680_ADC_MESSAGE:{
				unsigned char displayVal[8];
				adcIntToString(getMsgValue(&msgBuffer), displayVal);
				GLCD_DisplayString(PIC2680_LINE,10,1,displayVal);
				errorCountIntToString(pic2680MsgDropCount, displayVal);
				GLCD_DisplayString(PIC2680_LINE + 1,12,1,displayVal);
				break;
			}
			case PIC26J50_ADC_MESSAGE:{
				unsigned char displayVal[5];
				adcIntToString(getMsgValue(&msgBuffer), displayVal);
				GLCD_DisplayString(PIC26J50_LINE,11,1,displayVal);
				errorCountIntToString(pic26J50MsgDropCount, displayVal);
				GLCD_DisplayString(PIC26J50_LINE + 1,12,1,displayVal);
			}
			case PIC2680_ERROR:{
				pic2680errorCount++;
				unsigned char displayVal[8];
				errorCountIntToString(pic2680errorCount, displayVal);
				GLCD_DisplayString(PIC2680_LINE + 2,12,1,displayVal);
				break;
			}
			case PIC26J50_ERROR:{
				pic26J50errorCount++;
				unsigned char displayVal[8];
				errorCountIntToString(pic26J50errorCount, displayVal);
				GLCD_DisplayString(PIC26J50_LINE + 2,12,1,displayVal);
				break;
			}
			default:{
				break;
			}
		}

		// Here is a way to do debugging output via the built-in hardware -- it requires the ULINK cable and the
		//   debugger in the Keil tools to be connected.  You can view PORT0 output in the "Debug(printf) Viewer"
		//   under "View->Serial Windows".  You have to enable "Trace" and "Port0" in the Debug setup options.  This
		//   should not be used if you are using Port0 for printf()
		// There are 31 other ports and their output (and port 0's) can be seen in the "View->Trace->Records"
		//   windows.  You have to enable the prots in the Debug setup options.  Note that unlike ITM_SendChar()
		//   this "raw" port write is not blocking.  That means it can overrun the capability of the system to record
		//   the trace events if you go too quickly; that won't hurt anything or change the program execution and
		//   you can tell if it happens because the "View->Trace->Records" window will show there was an overrun.
		//vtITMu16(vtITMPortLCD,screenColor);
	}
}
