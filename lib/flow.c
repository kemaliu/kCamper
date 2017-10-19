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
}__info[FLOW_NUM] = {0};

static UINT8 last_pinc = 3;
SIGNAL(PCINT1_vect)
{
    UINT8 val = (PINC >> 3) & 3;
    UINT8 change;
    change = val ^ last_pinc;
    last_pinc = val;
    if((change & 1) && ((val & 1) == 0)){ /* PCINT11 low interrupt */
        __info[0].cnt++;
    }
    if((change & 2) && ((val & 2) == 0)){ /* PCINT11 low interrupt */
        __info[1].cnt++;
    }
}

void flow_init()
{
        /* set PC3(PCINT11)/PC4(PCINT12) to input & pull-up */
    gpio_input(GPIO_GROUP_C, 3, 1);
    gpio_input(GPIO_GROUP_C, 4, 1);
    
    PCMSK1 |= (1 << (11 - 8)) | (1 << (12 - 8));
    
    PCIFR |= 2;			/* clear PCINT1 flag*/
    PCICR |= 2;			/* enable PCINT1 */
}


void flow_reset(FLOW_CTRL_T index)
{
    if(index > FLOW_TANK2_OUT || index < FLOW_TANK1_OUT)
        return;
    cli();			/* 关中断 */
    __info[index].cnt = __info[index].last_cnt = 0;
    __info[index].start_time = __info[index].last_time = timebase_get();
    sei();			/* 开中断 */
}



/* return speed (cnt inc number)/1min  */
UINT32 flow_cnt_speed(FLOW_CTRL_T index)
{

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
        printf("cnt %lu speed %lu\n", __info[index].cnt, __info[index].speed);
        return speed;
    }
}
