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
#include "navigation.h"
#include "speedLimit.h"

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
void vStartConductorTask(vtConductorStruct *_params,unsigned portBASE_TYPE _uxPriority, vtI2CStruct *_i2c, myI2CStruct *_myi2c, motorControlStruct *_mc, navigationStruct *_nav, speedLimitControlStruct *_speed, myADCStruct *_adc)
{
	/* Start the task */
	portBASE_TYPE retval;
	_params->dev = _i2c;
	_params->i2cData = _myi2c;
	_params->motorControl = _mc;
	_params->navData = _nav;
	_params->speedData = _speed;
	_params->adcData = _adc;
	//_params->ADC_HERE_FFS  THIS IS FINALLY WHAT I WAS LOOKING FOR FFS!!
	if ((retval = xTaskCreate( vConductorUpdateTask, ( signed char * ) "Conductor", conSTACK_SIZE, (void *) _params, _uxPriority, ( xTaskHandle * ) NULL )) != pdPASS) {
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

// This is the actual task that is run
static portTASK_FUNCTION( vConductorUpdateTask, pvParameters )
{
	uint8_t rxLen, status;
	uint8_t Buffer[vtI2CMLen];

	// Get the parameters
	vtConductorStruct *param = (vtConductorStruct *) pvParameters;
	// Get the I2C device pointer
	vtI2CStruct *devPtr = param->dev;
	// Get the I2C task pointer
	myI2CStruct *i2cData = param->i2cData;
	// Get the Motor Control task pointer
	motorControlStruct *motorControl = param->motorControl;
	// Get the Navigation task pointer
	navigationStruct *nav = param->navData;
	// Get the Speed Limit task pointer
	speedLimitControlStruct *speed = param->speedData;
	// Get the ADC task pointer
	myADCStruct *adc = param->adcData;

	uint8_t recvMsgType;

	// Message counts
	uint8_t colorSensorMsgCount = 0, encodersMsgCount = 0, IRMsgCount = 0; adcMsgCount = 0;

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
					case ADC_EMPTY_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						adcMsgCount++;
						if(adcMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - adcMsgCount
							adcMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case GENERIC_EMPTY_MESSAGE: {
					break;
					}
					case COLOR_SENSOR_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						sendColorSensorDataMsg(nav, (Buffer + 2), 2);
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
						sendIRDataMsg(speed, (Buffer + 2), 2);
						IRMsgCount++;
						if(IRMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - IRMsgCount
							IRMsgCount = getI2CMsgCount(Buffer);
						}
					break;
					}
					case ADC_MESSAGE: {
						notifyRequestRecvd(i2cData,portMAX_DELAY);
						sendADCDataMsg(adc, (Buffer + 2), 2);
						adcMsgCount++;
						if(adcMsgCount != getI2CMsgCount(Buffer)){
							//Send Web Server an error with getI2CMsgCount(Buffer) - IRMsgCount
							adcMsgCount = getI2CMsgCount(Buffer);
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
