#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "ktype.h"

void ds_init();
INT16 ds_get_id_temperaturex16(UINT8 index);

INT16 ds_get_temperature_sample(char index);
INT16 ds_get_temperature_read(char index);

#endif
