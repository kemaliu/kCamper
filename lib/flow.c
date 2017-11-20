#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ktype.h"
#include "uart.h"
#include "flow.h"
#include "gpio.h"
#include "kconfig.h"
#include "timer.h"


struct flow_info{
    UINT32 cnt;
    UINT32 last_cnt;
    UINT32 start_time;
    UINT32 last_time;
    UINT32 speed;
}__info[FLOW_NUM];

static UINT8 last_pinc = 7;
SIGNAL(PCINT1_vect)
{
    UINT8 val = (PINC >> 3) & 7;
    UINT8 change;
    change = val ^ last_pinc;
    last_pinc = val;
    if((change & 1) && ((val & 1) == 0)){ /* PCINT11 low interrupt */
        __info[0].cnt++;
    }
    if((change & 2) && ((val & 2) == 0)){ /* PCINT12 low interrupt */
        __info[1].cnt++;
    }
    if((change & 4) && ((val & 4) == 0)){ /* PCINT13 low interrupt */
        __info[2].cnt++;
    }
}

void flow_init()
{
        /* set PC3(PCINT11)/PC4(PCINT12)/PC5(PCINT13) to input & pull-up */
    gpio_input(GPIO_GROUP_C, 3, 1);
    gpio_input(GPIO_GROUP_C, 4, 1);
    gpio_input(GPIO_GROUP_C, 5, 1);
    
    PCMSK1 |= (1 << (11 - 8)) | (1 << (12 - 8)) | (1 << (13 - 8));
    
    PCIFR |= 2;			/* clear PCINT1 flag*/
    PCICR |= 2;			/* enable PCINT1 */
}


void flow_reset(UINT8 index)
{
    if(index >= FLOW_NUM)
        return 0;
    cli();			/* 关中断 */
    __info[index].cnt = __info[index].last_cnt = 0;
    __info[index].start_time = __info[index].last_time = timebase_get();
    sei();			/* 开中断 */
}



UINT32 flow_cnt(UINT8 index)
{
    if(index >= FLOW_NUM)
        return 0;
    return __info[index].cnt;
}

/* return speed (cnt inc number)/1min  */
UINT32 flow_cnt_speed(UINT8 index)
{
    if(index >= FLOW_NUM)
        return 0;
    if(time_diff_ms(__info[index].last_time) < 100){
        printf(".");
        return __info[index].speed;
    }else{
        UINT32 ms = time_diff_ms(__info[index].last_time);
        UINT32 speed;
        speed = __info[index].cnt - __info[index].last_cnt;
        speed = speed *60000/ms;
        __info[index].speed = speed;
        __info[index].last_cnt = __info[index].cnt;
        __info[index].last_time = timebase_get();
        return speed;
    }
}
