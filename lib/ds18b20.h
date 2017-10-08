#ifndef __DS18B20_H__
#define __DS18B20_H__

#include "ktype.h"

void ds_init();
INT8 ds_reset();
INT16 ds_get_temperature();
INT16 ds_get_id_temperaturex16(UINT8 index);

INT16 ds_get_id_temperaturex16_unblock(UINT8 * pIndex);
void ds_sample_check();

INT8 ds_identify();

void ds_sample_check();
#endif
