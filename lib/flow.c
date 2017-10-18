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


static UINT32 __flow_cnt[2] = {0, 0};
static UINT8 last_pinc = 3;
SIGNAL(PCINT1_vect)
{

    UINT8 val = (PINC >> 3) & 3;
    UINT8 change;
    change = val ^ last_pinc;
    last_pinc = val;
    if((change & 1) && ((val & 1) == 0)){ /* PCINT11 low interrupt */
        __flow_cnt[0]++;
    }
    if((change & 2) && ((val & 2) == 0)){ /* PCINT11 low interrupt */
        __flow_cnt[1]++;
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

UINT32 flow_num(FLOW_CTRL_T index)
{
    if(index > FLOW_TANK2_OUT || index < FLOW_TANK1_OUT)
        return 0;
    return __flow_cnt[index];
}


void flow_reset(FLOW_CTRL_T index)
{
    if(index > FLOW_TANK2_OUT || index < FLOW_TANK1_OUT)
        return;
    __flow_cnt[index] = 0;
}
