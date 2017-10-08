#ifndef __SPI_H__
#define __SPI_H__
#include "ktype.h"
void spi_reg_write(UINT8 reg, UINT8 *d, UINT8 len);
UINT8 spi_reg_read(unsigned char reg, UINT8 * d, UINT8 len);
void spi_init();



#endif
