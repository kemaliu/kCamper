#ifndef __RTC_H__
#define __RTC_H__
#include "ktype.h"
struct rtc_time{
    uint8 year;
    uint8 mon;
    uint8 day;
    uint8 week;
    uint8 hour;
    uint8 min;
    uint8 second;
};

void rtc_set(struct rtc_time * time);
void rtc_get(struct rtc_time * time);

#endif
