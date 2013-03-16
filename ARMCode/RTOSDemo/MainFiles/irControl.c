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

// Compile time constants for physical sensor positions
#define SIDE_SEN_SEPARATION	250		// mm, vertical distance between side sensors "0"
#define COS_SEN_ANGLE_FS	0.707f	// cos(sensor angle front side)
#define SEN_OFFSET_FS		10.0f	// mm, distance from edge to sensor's "0"
#define COS_SEN_ANGLE_BS	0.985f	// cos(sensor angle back side)
#define SEN_OFFSET_BS		5.0f	// mm, distance from edge to sensor's "0"
#difine COS_SEN_ANGLE_F		0.966f	// cos(sensor angle front)
#define SEN_OFFSET_F		0.0f	// mm, distance from edge to sensor's "0"

// Maximum errors before shutting off.
#defint MAX_IR_ERRORS		10

typedef struct __cur_ir_data {
	unsigned short BLS,		// Back Left Side
	unsigned short FLS,		// Front Left Side
	unsigned short LF,		// Left Front
	unsigned short RF,		// Right Front
	unsigned short FRS,		// Front Right Side
	unsigned short BRS		// Back Right Side
} cur_ir_data;

// Private routines used to unpack the message buffers.
// I do not want to access the message buffer data structures outside of these routines.
// These routines are specific to accessing our packet protocol from the task struct.

// For accessing data sent between ARM local tasks:

/**     N/A     **/
// This means no ARM tasks (excluding the conductor) are sending data to this task

// For accessing data sent between Rover PIC(s) and the ARM:

uint8_t getPcktProtoID(irMsg *irBuf){
    return irBuf->buf[0];
}

uint8_t getPcktProtoSensorNum(irMsg *irBuf){
    return irBuf->buf[1];
}

uint8_t getPcktProtoParity(irMsg *irBuf){
    return irBuf->buf[2];
}

uint8_t getPcktProtoCount(irMsg *irBuf){
    return irBuf->buf[3];
}

uint8_t getPcktProtoData1(irMsg *irBuf){
    return irBuf->buf[4];
}

uint8_t getPcktProtoData2(irMsg *irBuf){
    return irBuf->buf[5];
}

uint8_t getPcktProtoData3(irMsg *irBuf){
    return irBuf->buf[6];
}

uint8_t getPcktProtoData4(irMsg *irBuf){
    return irBuf->buf[7];
}

unsigned short getPcktProtoIRDistance(irMsg *irBuf) {
	unsigned short dist = irBuf->buf[4];
	dist = (dist << 8) | irBuf->buf[5];
	return dist;
}

// End of private routines for message buffers
/*-----------------------------------------------------------*/

// Private routines used for data manipulation, etc.
// There should be NO accessing of our packet protocol from the task struct in these routines.

int getMsgType(irMsg *irBuf)
{
    return(irBuf->msgType);
}

// End of private routines for data manipulation, etc.
/*-----------------------------------------------------------*/

// Here is where the declaration of static task pointers occurs; they will be initialized below.
static irControlStruct *param;
static navigationStruct *navData;

// Buffer for receiving messages
static irMsg msgBuffer;

// --- Private helper functions ----------------------------------------------
void updateAndSendSideWall() {
	// Calculate distance to wall from front of rover
	float distance = (float)ir_data.FLS * COS_SEN_FS - SEN_OFFSET_FS;
	// Calculate the angle the wall is relative to the rover. 
	// -positive is clockwise, negative is counterclockwise
	float angle = atan2(((float)ir_data.BLS * COS_SEN_BS - SEN_OFFSET_BS) - distance, SIDE_SEN_SEPARATION);	
}
void updateAndSendRightWall() {
	// Calculate distance to wall from front of rover
	float distance = (float)ir_data.FRS * COS_SEN_FS - SEN_OFFSET_FS;
	// Calculate the angle the wall is relative to the rover. 
	// -positive is clockwise, negative is counterclockwise
	float angle = atan2(((float)ir_data.BRS * COS_SEN_BS - SEN_OFFSET_BS) - distance, SIDE_SEN_SEPARATION);
}
void updateAndSendFrontWall() {
	// Implemented in MS 4. Not needed for midterm demonstration
}
// --- End Private helper functions ------------------------------------------

// This is the actual task that is run
static portTASK_FUNCTION( vIRTask, pvParameters )
{
    // Get the parameters
    param = (irControlStruct *) pvParameters;
    // Get the other necessary tasks' task pointers like this:
    navData = param->navData;
	
	// Private storage
	cur_ir_data ir_data;
	unsigned char count;
	unsigned char errorCount;
	
	// Private storage init
	ir_data.BLS = 0;
	ir_data.FLS = 0;
	ir_data.LF  = 0;
	ir_data.RF  = 0;
	ir_data.FRS = 0;
	ir_data.BRS = 0;
	count = 0;
	errorCount = 0;

    // Like all good tasks, this should never exit
    for(;;)
    {
        // Wait for a message from whomever we decide will need to talk to this task
        if (xQueueReceive(param->inQ,(void *) &msgBuffer,portMAX_DELAY) != pdTRUE) {
            VT_HANDLE_FATAL_ERROR(Q_RECV_ERROR);
        }
        switch(getMsgType(&msgBuffer)){
            case irDataMsgType:
            {
				// Error check message
				if (getPcktProtoCount(&msgBuffer) != count) {
					count = getPcktProtoCount(&msgBuffer);
					++errorCount;
					if (errorCount > MAX_IR_ERRORS) {
						//VT_HANDLE_FATAL_ERROR(); // Get Matt to add new error type
					}
				}
				if (getPcktProtoParity(&msgBuffer) != getPcktProtoData1(&msgBuffer) ^ getPcktProtoData2(&msgBuffer)) {
					++errorCount;
					if (errorCount > MAX_IR_ERRORS) {
						//VT_HANDLE_FATAL_ERROR(); // Get Matt to add new error type
					}
				}
				
				// Save reading and update coresponding wall
				switch (getPcktProtoSensorNum(&msgBuffer)) {
					case 1: // Back Left Side
					{
						ir_data.BLS = getPcktProtoDistance(&msgBuffer);
						updateAndSendLeftWall();
						break;
					}
					case 2: // Front Left Side
					{
						ir_data.FLS = getPcktProtoDistance(&msgBuffer);
						updateAndSendLeftWall();
						break;
					}
					case 3: // Left Front
					{
						ir_data.LF = getPcktProtoDistance(&msgBuffer);
						updateAndSendFrontWall();
						break;
					}
					case 4: // Right Front
					{
						ir_data.RF = getPcktProtoDistance(&msgBuffer);
						updateAndSendFrontWall();
						break;
					}
					case 5: // Front Right Side
					{
						ir_data.FRS = getPcktProtoDistance(&msgBuffer);
						updateAndSendRightWall();
						break;
					}
					case 6: // Back Right Side
					{
						ir_data.BRS = getPcktProtoDistance(&msgBuffer);
						updateAndSendRightWall();
						break;
					}
				}
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
