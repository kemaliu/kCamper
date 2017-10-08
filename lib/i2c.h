#ifndef __I2C_H__
#define __I2C_H__
#include "ktype.h"


void I2C_init();

int I2C_Read_(uint16 wrDAdr,uint8 wordAdr,
	      uint8 rdDAdr,uint8 *pRdDat,uint8 num);


int I2C_Write_(uint16 wrDAdr,uint8 wordAdr,
	       uint8 *pWrDat,uint8 num);

#endif
