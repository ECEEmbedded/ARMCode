// The include statement and MILESTONE_1 #if added by
// Matthew Ibarra 2/2/2013
#include "myDefs.h"
#if MILESTONE_1==1

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "timers.h"

/* include files. */
#include "vtUtilities.h"
#include "LCDtask.h"
#include "myTimers.h"

/* **************************************************************** */
// WARNING: Do not print in this file -- the stack is not large enough for this task
/* **************************************************************** */

/* *********************************************************** */
// Removed the LCD stuff because that isn't required in this file for milestone 1

/* *********************************************************** */
// Functions for the Temperature Task related timer
//
// how often the timer that sends messages to the LCD task should run
// Set the task up to run every 500 ms
#define tempWRITE_RATE_BASE	( ( portTickType ) 500 / portTICK_RATE_MS)
// I wonder if this should be set to 100ms to maybe run better?

// Callback function that is called by the TemperatureTimer
//   Sends a message to the queue that is read by the Temperature Task
void TempTimerCallback(xTimerHandle pxTimer)
{
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		// When setting up this timer, I put the pointer to the 
		//   Temperature structure as the "timer ID" so that I could access
		//   that structure here -- which I need to do to get the 
		//   address of the message queue to send to 
		vtTempStruct *ptr = (vtTempStruct *) pvTimerGetTimerID(pxTimer);
		// Make this non-blocking *but* be aware that if the queue is full, this routine
		// will not care, so if you care, you need to check something
		if (SendTempTimerMsg(ptr,tempWRITE_RATE_BASE,0) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void startTimerForTemperature(vtTempStruct *vtTempdata) {
	if (sizeof(long) != sizeof(vtTempStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle TempTimerHandle = xTimerCreate((const signed char *)"Temp Timer",tempWRITE_RATE_BASE,pdTRUE,(void *) vtTempdata,TempTimerCallback);
	if (TempTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(TempTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}
#endif