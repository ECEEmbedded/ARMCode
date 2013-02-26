#include "myDefs.h"
#if MILESTONE_2==1
#define vtI2CMsgTypeInit 1
#define vtI2CMsgTypeRead 2
// below is not actually an i2c message, but the value is reserved
#define i2cMsgTypeTimer 3

//I2C message types
#define PIC2680_EMPTY_MESSAGE 0x50
#define PIC26J50_EMPTY_MESSAGE 0x51
#define PIC2680_ADC_MESSAGE 0x10
#define PIC26J50_ADC_MESSAGE 0x11
#define PIC2680_ERROR 0xF0
#define PIC26J50_ERROR 0xF1

//Error codes
#define Q_FULL 1
#endif