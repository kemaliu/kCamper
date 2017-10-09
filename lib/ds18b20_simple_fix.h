#ifndef __DS18B20_SIMPLIE_FIX_H__
#define __DS18B20_SIMPLIE_FIX_H__
typedef enum{
  TEMPERATURE_SENSOR_TANK1 = 0,
  TEMPERATURE_SENSOR_TANK2 = 1,
  TEMPERATURE_SENSOR_HEATER = 2
};

void ds_init();
INT16 ds_get_temperature_x16(char index);
#endif	/* #ifndef __DS18B20_SIMPLIE_FIX_H__ */
