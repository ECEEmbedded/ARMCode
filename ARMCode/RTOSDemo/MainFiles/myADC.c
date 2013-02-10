#include "myDefs.h"
#if MILESTONE_1==1
#include "myADC.h"

void getMsgValue(int *target,vtLCDMsg *lcdBuffer){
	*(target) = (int) lcdBuffer->buf[0];
}
#endif