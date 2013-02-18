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
#include "LCDtask.h"
#include "string.h"
// Added the myDefs.h include here by Matthew Ibarra 2/4/2013
#include "myDefs.h"

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
// a timer message -- not to be printed
#define LCDMsgTypeTimer 1
// a message to be printed
#define LCDMsgTypePrint 2

// Added by Matthew Ibarra for ADC LCD Task Message 2/4/2013
#define LCDMsgTypeADC 3

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

portBASE_TYPE SendLCDTimerMsg(vtLCDStruct *lcdData,portTickType ticksElapsed,portTickType ticksToBlock)
{
	if (lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	vtLCDMsg lcdBuffer;
	lcdBuffer.length = sizeof(ticksElapsed);
	if (lcdBuffer.length > vtLCDMaxLen) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(lcdBuffer.length);
	}
	memcpy(lcdBuffer.buf,(char *)&ticksElapsed,sizeof(ticksElapsed));
	lcdBuffer.msgType = LCDMsgTypeTimer;
	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
}

portBASE_TYPE SendLCDPrintMsg(vtLCDStruct *lcdData,int length,char *pString,portTickType ticksToBlock)
{
	if (lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	vtLCDMsg lcdBuffer;

	if (length > vtLCDMaxLen) {
		// no room for this message
		VT_HANDLE_FATAL_ERROR(lcdBuffer.length);
	}
	lcdBuffer.length = strnlen(pString,vtLCDMaxLen);
	lcdBuffer.msgType = LCDMsgTypePrint;
	strncpy((char *)lcdBuffer.buf,pString,vtLCDMaxLen);
	return(xQueueSend(lcdData->inQ,(void *) (&lcdBuffer),ticksToBlock));
}

// Added by Matthew Ibarra 2/4/2013
portBASE_TYPE SendLCDADCMsg(vtLCDStruct *lcdData, int data, portTickType ticksToBlock)
{
	if(lcdData == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	}

	vtLCDMsg lcdBuffer;

	lcdBuffer.length = sizeof(data);
	lcdBuffer.msgType = LCDMsgTypeADC;
	lcdBuffer.buf[0] = (uint8_t) data;
	lcdBuffer.buf[1] = (uint8_t)(data>>8);
	return(xQueueSend(lcdData->inQ, (void *) (&lcdBuffer), ticksToBlock));
}

// Private routines used to unpack the message buffers
//   I do not want to access the message buffer data structures outside of these routines
portTickType unpackTimerMsg(vtLCDMsg *lcdBuffer)
{
	portTickType *ptr = (portTickType *) lcdBuffer->buf;
	return(*ptr);
}

int getMsgType(vtLCDMsg *lcdBuffer)
{
	return(lcdBuffer->msgType);
}

int getMsgLength(vtLCDMsg *lcdBuffer)
{
	return(lcdBuffer->msgType);
}

void copyMsgString(char *target,vtLCDMsg *lcdBuffer,int targetMaxLen)
{
	strncpy(target,(char *)(lcdBuffer->buf),targetMaxLen);
}

// End of private routines for message buffers

// If LCD_EXAMPLE_OP=0, then accept messages that may be timer or print requests and respond accordingly
// If LCD_EXAMPLE_OP=1, then do a rotating ARM bitmap display
#define LCD_EXAMPLE_OP 0
#if LCD_EXAMPLE_OP==1
// This include the file with the definition of the ARM bitmap
#include "ARM_Ani_16bpp.c"
#endif

#if LCD_EXAMPLE_OP==0
// Buffer in which to store the memory read from the LCD
	#define MAX_RADIUS 15
	#define BUF_LEN (((MAX_RADIUS*2)+1)*((MAX_RADIUS*2)+1))
	static unsigned short int buffer[BUF_LEN];
#endif

// This is the actual task that is run
static portTASK_FUNCTION( vLCDUpdateTask, pvParameters )
{
	unsigned short screenColor = 0;
	unsigned short tscr;
	unsigned char curLine;
	unsigned timerCount = 0;
	int xoffset = 0, yoffset = 0;
	unsigned int xmin=0, xmax=0, ymin=0, ymax=0;
	unsigned int x, y;
	int i, j;
	float hue=0, sat=0.2, light=0.2;

	vtLCDMsg msgBuffer;
	vtLCDStruct *lcdPtr = (vtLCDStruct *) pvParameters;

	/* Initialize the LCD and set the initial colors */
	GLCD_Init();
	tscr = Red; // may be reset in the LCDMsgTypeTimer code below
	screenColor = White; // may be reset in the LCDMsgTypeTimer code below
	GLCD_SetTextColor(tscr);
	GLCD_SetBackColor(screenColor);
	GLCD_Clear(screenColor);

	int xPos = 0;

	curLine = 5;
	// This task should never exit
	for(;;)
	{
		// Wait for a message
		if (xQueueReceive(lcdPtr->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}
		switch(getMsgType(&msgBuffer)) {
			case LCDMsgTypePrint: {
				// Nothing at the moment
				break;
			}
			case LCDMsgTypeTimer: //{
			//	// Nothing at the moment
			//	break;
			//}
			case LCDMsgTypeADC: {		// I think this should work like it did before.
										// Will simply put it back to the way it was before
										// if things start not working.
				if(timerCount==0) {
					GLCD_Clear(screenColor);

					// Draw the vertical gridlines onto the LCD
					int i;
					for(i = 320/6; i < 315; i = i + 320/6) {
						GLCD_ClearWindow(i, 0, 1, 240, Red);
					}

					// Draw the vertical gridlines onto the LCD
					for(i = 240/4; i < 235; i = i + 240/4) {
						GLCD_ClearWindow(0, i, 320, 1, Red);
					}

					//Output Scale on LCD
					GLCD_DisplayString(29, 0, 0, (unsigned char*) "V/div=2.5 s/div=3");
					timerCount++;
				}
				int adcValue;
				getMsgValue(&adcValue, &msgBuffer);
				int displayValue;
				displayValue = 120 - (adcValue * 120)/(0x100);
				GLCD_ClearWindow(xPos, displayValue, 2, 2, Black);
				xPos += 2;
				if(xPos > 320) {
					timerCount = 0;
					xPos = 0;
				}
				break;
			}
			default: {
				// In this configuration, we are only expecting to receive timer messages
				VT_HANDLE_FATAL_ERROR(getMsgType(&msgBuffer));
				break;
			}
		} // end of switch()

		#if MILESTONE_1==1
		// Added by Matthew Ibarra 2/2/2013
			if(timerCount==0) {
				GLCD_Clear(screenColor);

				// Draw the vertical gridlines onto the LCD
				int i;
				for(i = 320/6; i < 315; i = i + 320/6) {
					GLCD_ClearWindow(i, 0, 1, 240, Red);
				}

				// Draw the vertical gridlines onto the LCD
				for(i = 240/4; i < 235; i = i + 240/4) {
					GLCD_ClearWindow(0, i, 320, 1, Red);
				}

				//Output Scale on LCD
				GLCD_DisplayString(29, 0, 0, (unsigned char*) "V/div=2.5 s/div=3");
				timerCount++;
			}
			int adcValue;
			getMsgValue(&adcValue, &msgBuffer);
			int displayValue;
			displayValue = 120 - (adcValue * 120)/(0x100);
			GLCD_ClearWindow(xPos, displayValue, 2, 2, Black);
			xPos += 2;
			if(xPos > 320) {
				timerCount = 0;
				xPos = 0;
			}
		#endif

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
