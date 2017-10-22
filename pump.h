#ifndef __PUMP_H__
#define __PUMP_H__
enum{
    PUMP_OFF = 0,
    PUMP_LOW_SPEED,
    PUMP_FULL_SPEED
};


void pump_init();
void pump_mode_set(char mode);
void pump_mode_show(char mode);
#endif  /* #ifndef __PUMP_H__ */
