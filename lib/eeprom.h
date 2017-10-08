#ifndef __EEPROM_H__
#define __EEPROM_H__
#include "ktype.h"
UINT8 EEPROM_put(UINT16 uiAddress, UINT8 ucData);
UINT8 EEPROM_get(UINT16 uiAddress);

void eeprom_read(uint16 addr, uint8 * buf, uint8 size);
UINT8 eeprom_write(uint16 addr, uint8 * buf, uint8 size);
#endif
