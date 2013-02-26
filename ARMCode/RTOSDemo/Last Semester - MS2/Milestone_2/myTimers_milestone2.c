#include "myDefs.h"
#if MILESTONE_2==1

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"
#include "projdefs.h"
#include "timers.h"

/* include files. */
#include "vtUtilities.h"
#include "LCDtask.h"
#include "myTimers_milestone2.h"
#include "types_milestone2.h"

/* **************************************************************** */
// WARNING: Do not print in this file -- the stack is not large enough for this task
/* **************************************************************** */

/* *********************************************************** */

/* *********************************************************** */
// Functions for the i2c Task related timer
//
// how often the timer that sends messages to the i2c Task should run
// Set the task up to run every 100 ms
#define i2cWRITE_RATE_BASE	( ( portTickType ) 100 / portTICK_RATE_MS)

// Callback function that is called by the i2c Timer
//   Sends a message to the queue that is read by the i2c Task
void i2cTimerCallback(xTimerHandle pxTimer)
{
	if (pxTimer == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		// When setting up this timer, I put the pointer to the 
		//   i2c structure as the "timer ID" so that I could access
		//   that structure here -- which I need to do to get the 
		//   address of the message queue to send to 
		myI2CStruct *ptr = (myI2CStruct *) pvTimerGetTimerID(pxTimer);
		// Make this non-blocking *but* be aware that if the queue is full, this routine
		// will not care, so if you care, you need to check something
		if (sendi2cTimerMsg(ptr,i2cWRITE_RATE_BASE,0) == errQUEUE_FULL) {
			// Here is where you would do something if you wanted to handle the queue being full
			//VT_HANDLE_FATAL_ERROR(0);
		}
	}
}

void startTimerFori2c(myI2CStruct *i2cdata) {
	if (sizeof(long) != sizeof(myI2CStruct *)) {
		VT_HANDLE_FATAL_ERROR(0);
	}
	xTimerHandle i2cTimerHandle = xTimerCreate((const signed char *)"i2c Timer",i2cWRITE_RATE_BASE,pdTRUE,(void *) i2cdata,i2cTimerCallback);
	if (i2cTimerHandle == NULL) {
		VT_HANDLE_FATAL_ERROR(0);
	} else {
		if (xTimerStart(i2cTimerHandle,0) != pdPASS) {
			VT_HANDLE_FATAL_ERROR(0);
		}
	}
}
#endif