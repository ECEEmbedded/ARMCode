#include "myDefs.h"
#ifndef LCD_TASK_H
#define LCD_TASK_H
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

// NOTE: This is a reasonable API definition file because there is nothing in it that the
//   user of the API does not need (e.g., no private definitions) and it defines the *only*
//   way a user of the API is allowed to interact with the task


// Define a data structure that is used to pass and hold parameters for this task
// Functions that use the API should not directly access this structure, but rather simply
//   pass the structure as an argument to the API calls
typedef struct __vtLCDStruct {
	xQueueHandle inQ;					   	// Queue used to send messages from other tasks to the LCD task to print
} vtLCDStruct;

// Added by Matthew Ibarra 2/2/2013
// These define the width and height of the LCD screen
#define WIDTH 320;
#define HEIGHT 240;

// These define the lines on the LCD where the 2680 & 26J50 ADC data gets displayed
#define PIC2680_LINE 0
#define PIC26J50_LINE 4

// Structure used to define the messages that are sent to the LCD thread
//   the maximum length of a message to be printed is the size of the "buf" field below
#define vtLCDMaxLen 20

// actual data structure that is sent in a message
typedef struct __vtLCDMsg {
	uint8_t msgType;
	uint8_t	length;	 // Length of the message to be printed
	uint8_t buf[vtLCDMaxLen+1]; // On the way in, message to be sent, on the way out, message received (if any)
} vtLCDMsg;
// end of defs

/* ********************************************************************* */
// The following are the public API calls that other tasks should use to work with the LCD task
//   Note: This is *not* the API for actually manipulating the graphics -- that API is defined in GLCD.h
//         and is accessed by the LCD task (other tasks should not access it or conflicts may occur).
//
// Start the task
// Args:
//   lcdData -- a pointer to a variable of type vtLCDStruct
//   uxPriority -- the priority you want this task to be run at
void StartLCDTask(vtLCDStruct *lcdData, unsigned portBASE_TYPE uxPriority);

portBASE_TYPE SendLCDADCMsg(vtLCDStruct *lcdData,int data, uint8_t type, uint8_t errCount, portTickType ticksToBlock);

portBASE_TYPE SendLCDErrorMsg(vtLCDStruct *lcdData, uint8_t type, portTickType ticksToBlock);

/* ********************************************************************* */

void LCDTimerCallback(xTimerHandle);

#endif
