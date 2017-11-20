#include "pump.h"
#include "lib/gpio.h"
#include "lib/kconfig.h"

static char __current_pump_mode = 0;

void pump_init()
{
        /* pump control bit PC5 */
    pump_mode_set(PUMP_OFF);
}



void pump_mode_show(char mode)
{
    screen_const_puts("LABL(16,175,99,239,'泵:");
    if(mode == 0)
        screen_const_puts("关闭");
    else if(mode == 2)
        screen_const_puts("全速");
    else
        screen_const_puts("低速");
    screen_const_puts("',15,0);\n");
}


/* PUMP_LOW_SPEED:low speed PUMP_FULL_SPEED:full speed */
void pump_mode_set(char mode)
{
        /* PD2 1:12V ------- pump
         *     0:12V ------- DC-DC 12->5v
         * PD3 1:5v output disable
         *     0:5v output enable
         */   
    switch(mode){
        case PUMP_OFF:
            gpio_output(GPIO_GROUP_D, 2, 0);
            gpio_output(GPIO_GROUP_D, 3, 1);
            __current_pump_mode = PUMP_OFF;
            break;
        case PUMP_LOW_SPEED:
            gpio_output(GPIO_GROUP_D, 2, 0);
            gpio_output(GPIO_GROUP_D, 3, 0);
            __current_pump_mode = PUMP_LOW_SPEED;
            break;
        case PUMP_FULL_SPEED:
            gpio_output(GPIO_GROUP_D, 2, 1);
            gpio_output(GPIO_GROUP_D, 3, 1);
            __current_pump_mode = PUMP_FULL_SPEED;
            break;
    }
    if(mode == PUMP_FULL_SPEED){
        
    }else{
        gpio_output(GPIO_GROUP_D, 2, 0);
    }
    pump_mode_show(mode);
}



char pump_mode_get()
{
    return __current_pump_mode;
}
