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
#include "conductor.h"
#include "motorControl.h"
#include "irControl.h"
#include "speedLimit.h"
// #include "power.h"
#include "lcdTask.h"

/* *********************************************** */
// definitions and data structures that are private to this file

// I have set this to a large stack size because of (a) using printf() and (b) the depth of function calls
//   for some of the i2c operations	-- almost certainly too large, see LCDTask.c for details on how to check the size
#define INSPECT_STACK 1
#define baseStack 2
#if PRINTF_VERSION == 1
#define conSTACK_SIZE		((baseStack+5)*configMINIMAL_STACK_SIZE)
#else
#define conSTACK_SIZE		(baseStack*configMINIMAL_STACK_SIZE)
#endif
// end of defs
/* *********************************************** */

/* The Conductor task. */
static portTASK_FUNCTION_PROTO( vConductorUpdateTask, pvParameters );

/*-----------------------------------------------------------*/
// Public API
void vStartConductorTask(vtConductorStruct *params, unsigned portBASE_TYPE uxPriority, vtI2CStruct *i2c, myI2CStruct * myi2c, motorControlStruct *motorControl, irControlStruct *irData, speedLimitControlStruct *speedData, powerStruct *powerData, vtLCDStruct *lcdData)
{
	/* Start the task */
	portBASE_TYPE retval;
	params->dev = i2c;
	params->i2cData = myi2c;
	params->motorControl = motorControl;
	params->irData = irData;
	params->speedData = speedData;
	params->powerData = powerData;
	params->lcdData = lcdData;
	if ((retval = xTaskCreate( vConductorUpdateTask, ( signed char * ) "Conductor", conSTACK_SIZE, (void *) params, uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
		VT_HANDLE_FATAL_ERROR(retval);
	}
}

// End of Public API
/*-----------------------------------------------------------*/

uint8_t getI2CMsgType(uint8_t *buffer) {
	return buffer[0];
}

uint8_t getI2CMsgCount(uint8_t *buffer) {
	return buffer[1];
}

int geti2cADCValue(myi2cMsg *buffer){
    return (((int)buffer->buf[3])<<8)|(int)buffer->buf[2];
}

// This is the actual task that is run
static portTASK_FUNCTION( vConductorUpdateTask, pvParameters )
{
	uint8_t rxLen, status;
	uint8_t Buffer[vtI2CMLen];
	uint8_t *valPtr = &(Buffer[0]);

	// Get the parameters
	vtConductorStruct *param = (vtConductorStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the I2C task pointer
	myI2CStruct *i2cData = param->i2cData;
	// Get the Motor Control task pointer
	motorControlStruct *motorControl = param->motorControl;
	// Get the IR Control task pointer
	irControlStruct *irData = params->irData;
	// Get the Speed Limit task pointer
	speedLimitControlStruct *speedData = param->speedData;
	//Get the Power task pointer
	powerStruct *powerData = param->powerData;
    // Get the LCD pointer information
    vtLCDStruct *lcdData = param->lcdData;

	// Message counts
	uint8_t colorSensorMsgCount = 0, encodersMsgCount = 0, IRMsgCount = 0;

	uint8_t recvMsgType;

	int pic2680reqSent = 0;
    int pic26J50reqSent = 0;

    uint8_t sent2680ADCCount = 0;
    uint8_t sent26J50ADCCount = 0;
    uint8_t received2680ADCCount = 0;
    uint8_t received26J50ADCCount = 0;

	// Like all good tasks, this should never exit
	for(;;)
	{
		// Wait for a message from an I2C operation
		if (vtI2CDeQ(devPtr,vtI2CMLen,Buffer,&rxLen,&recvMsgType,&status) != pdTRUE) {
			VT_HANDLE_FATAL_ERROR(0);
		}

		// Decide where to send the message
		// This isn't a state machine, it is just acting as a router for messages
		switch(recvMsgType) {
			case vtI2CMsgTypeRead: {
				// If it is a read, send it to the appropriate task
				switch(getI2CMsgType(Buffer)){
					case COLOR_SENSOR_EMPTY_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						colorSensorMsgCount++;
						if(colorSensorMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - colorSensorMsgCount
							colorSensorMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case ENCODERS_EMPTY_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						encodersMsgCount++;
						if(encodersMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - encodersMsgCount
							encodersMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case IR_EMPTY_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						IRMsgCount++;
						if(IRMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - IRMsgCount
							IRMsgCount = getI2CMsgCount(Buffer);
						}
					}
					case PIC2680_EMPTY_MESSAGE:{
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						pic2680reqSent = 0;
						break;
					}
					case PIC26J50_EMPTY_MESSAGE:{
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						pic26J50reqSent	= 0;
						break;
					}
					case GENERIC_EMPTY_MESSAGE: {
					break;
					}
					case COLOR_SENSOR_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						sendColorSensorDataMsg(speed, (Buffer + 2), 2);
						colorSensorMsgCount++;
						if(colorSensorMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - colorSensorMsgCount
							colorSensorMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case ENCODERS_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						sendEncoderDataMsg(motorControl, (Buffer + 2), 2);
						encodersMsgCount++;
						if(encodersMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - encodersMsgCount
							encodersMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case IR_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						sendIRDataMsg(nav, (Buffer + 2), 2);
						IRMsgCount++;
						if(IRMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - IRMsgCount
							IRMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
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
				}
			break;
			}
			case vtI2CMsgTypeMotor: {
				// If it is a I2C motor message, just recognize that this is an ack from the slave
				// Nothing else to do here
				break;
			}
			default: {
				VT_HANDLE_FATAL_ERROR(recvMsgType);
				break;
			}
		}
	}
}
#endif

