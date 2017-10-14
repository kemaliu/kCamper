#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <string.h>
#include <stdlib.h>
#include <compat/deprecated.h>
#include "timer.h"
#include "rtc.h"
#include "ktype.h"

static volatile UINT32 __isr_cnt = 0;
void timer_clear()
{
    __isr_cnt = 0;
}

SIGNAL(TIMER2_OVF_vect)
{
    __isr_cnt++;

}


void timer_init()
{
    /* config internal timer interrupt, when TCNT2 reach max value,
       it will trigger a internal interrupt */

    TCCR2A = 0 | 		/* OC2A/OC2B disconnected */
	     0;			/* normal mode */
    TCCR2B = 0x4; 		/* 8M div 64 = 125K*/ 
    TCNT2=0;                    /* orignal count = 0 */
    timer_enable_int (_BV (TOIE2));
    TIMSK2 = 0x1;
    sei();
}

UINT32 timebase_get()
{
    UINT32 time = __isr_cnt;
    UINT8 low;
    do{
	time = __isr_cnt;
	low = TCNT2;
    }while(__isr_cnt != time);
    return (time << 8)|low;
}


UINT32 time_diff_us(UINT32 last)
{
    UINT32 now = timebase_get();
    now = (now - last);
    now = now << 3;
    return now;
}

UINT32 time_diff_ms(UINT32 last)
{
    UINT32 now = timebase_get();
    now = (now - last);
    now = (now << 3)/1000L;
    return now;
}



UINT32 sys_run_seconds()
{
        /* for timer 125K freq, 488 isr = 1s*/
    return __isr_cnt / 488;
}
