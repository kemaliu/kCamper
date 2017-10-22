#ifndef __DS18B20_SIMPLIE_FIX_H__
#define __DS18B20_SIMPLIE_FIX_H__
#include "ktype.h"
enum{
    TEMPERATURE_COLD_ID = 0,
    TEMPERATURE_HOT_ID,
    TEMPERATURE_HEATER_ID,
    TEMPERATURE_SENSOR_MAX_NUM
};

void ds_init();
INT16 ds_get_temperature_x16(char index);
INT16 ds_get_temperature_sample(char index);
INT16 ds_get_temperature_read(char index);

#endif	/* #ifndef __DS18B20_SIMPLIE_FIX_H__ */




