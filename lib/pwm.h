#ifndef __PWM_DRV_H__
#define __PWM_DRV_H__
#include "rtc.h"
#include "ktype.h"

/* fan PWM: PD5   OC0B
 * PWM1:  PB2 OC1B
 * PWM2:  PB1 OC1A
 */
void pwm_init();

/* 
 * 0: FAN PWM
 * 1: light1
 * 2: light2
 */
void pwm_set(char channel, unsigned char value);
UINT8 pwm_get(char channel);

void local_time_set(struct rtc_time * time);

void local_time_get(struct rtc_time * time);

UINT32 local_ms_get();
UINT32 local_time_diff_ms(UINT32 last);

#endif
