#ifndef __TIMER_H__
#define __TIMER_H__
#include "ktype.h"
#include "rtc.h"
typedef void (*TIMECB)(uint8);


void timer_clear();
void timer_init();
void timer_cb_set(TIMECB cb);

UINT32 timebase_get();
UINT32 time_diff_us(UINT32 last);

UINT32 time_diff_ms(UINT32 last);



#endif
