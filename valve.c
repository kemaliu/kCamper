#include "lib/gpio.h"
#include "lib/ktype.h"
#include "lib/kconfig.h"
#include "lib/timer.h"
#include "valve.h"

#define SWITCH_MASK_PUMP  0x10                /* bit4 */
#define VALVE_INPUT_TANK1  0                /* bit2 */
#define VALVE_INPUT_TANK2  4                /* bit2 */
#define VALVE_OUTPUT_TANK1  2                /* bit1 */
#define VALVE_OUTPUT_TANK2  1                /* bit0 */



/* - scene:
 *                   | pump low |   valve main   |  valve 1   |  valve  2 |
 *  normal           |   off    |     off        |  off       |   off     |
 *  water tank1->2   |   on     |     off        |  off       |   on      |
 *  water tank1 loop |   on     |     off        |  on        |   off     |
 *  water tank2 loop |   on     |     on         |  off       |   on      |
 *  water tank2->1   |   on     |     on         |  on        |   off     |
 */
unsigned char __scene_cfg[SCENE_TOTAL_NUM] = {
    SWITCH_MASK_PUMP | VALVE_INPUT_TANK1,           /* normal */
    SWITCH_MASK_PUMP | VALVE_INPUT_TANK1 | VALVE_OUTPUT_TANK2, /* tank1->tank2 */
    SWITCH_MASK_PUMP | VALVE_INPUT_TANK1 | VALVE_OUTPUT_TANK1, /* tank1 loop */
    SWITCH_MASK_PUMP | VALVE_INPUT_TANK2 | VALVE_OUTPUT_TANK2, /* tank2 loop */
    SWITCH_MASK_PUMP | VALVE_INPUT_TANK2 | VALVE_OUTPUT_TANK1, /* tank2 loop */
};

#define VALVE_ON 0
#define VALVE_OFF 1
#define VALVE_POWER_GRP GPIO_GROUP_D
#define VALVE_POWER_INDEX 5

#define VALVE_MAIN_GRP GPIO_GROUP_D
#define VALVE_MAIN_INDEX 6

#define VALVE_1_GRP GPIO_GROUP_D
#define VALVE_1_INDEX 7

#define VALVE_2_GRP GPIO_GROUP_B
#define VALVE_2_INDEX 0


static UINT32 valve_mod_time = 0x0;
static unsigned char valve_status = 0xf;

void valve_init()
{
    valve_setup(SCENE_NORMAL);
    _delay_ms(5000);
    valve_power_down();
}

void valve_power_down()
{
        /* powe down all */
    gpio_output(VALVE_POWER_GRP, VALVE_POWER_INDEX, VALVE_OFF);
        /* set all IO to VALVE_OFF mode for power saving */
    gpio_output(VALVE_MAIN_GRP, VALVE_MAIN_INDEX, VALVE_OFF);
    gpio_output(VALVE_1_GRP, VALVE_1_INDEX, VALVE_OFF);
    gpio_output(VALVE_2_GRP, VALVE_2_INDEX, VALVE_OFF);
}


void valve_check()
{
    if(sys_run_seconds() - valve_mod_time > 5){
            /* poweroff all the valve */
        valve_power_down();
    }
}

void valve_setup(UINT8 scene)
{
    UINT8 cfg = __scene_cfg[scene];

    if((cfg & 0xf) == valve_status){
        return;
    }
    valve_mod_time = sys_run_seconds();
    valve_status = cfg & 0xf;
    
    screen_const_puts("LABL(16,175,116,239,'进水:");
    if(cfg & VALVE_INPUT_TANK2){
        gpio_output(VALVE_MAIN_GRP, VALVE_MAIN_INDEX, VALVE_ON);
        screen_const_puts("2");
    }else{
        gpio_output(VALVE_MAIN_GRP, VALVE_MAIN_INDEX, VALVE_OFF);
        screen_const_puts("1");
    }
    screen_const_puts("',15,0);\n");
    
    screen_const_puts("LABL(16,175,133,239,'出水1:");
    if(cfg & VALVE_OUTPUT_TANK1){
        gpio_output(VALVE_1_GRP, VALVE_1_INDEX, VALVE_ON);
        screen_const_puts("开");
    }else{
        gpio_output(VALVE_1_GRP, VALVE_1_INDEX, VALVE_OFF);
        screen_const_puts("关");
    }
    screen_const_puts("',15,0);\n");

    screen_const_puts("LABL(16,175,150,239,'出水2:");
    if(cfg & VALVE_OUTPUT_TANK2){
        gpio_output(VALVE_2_GRP, VALVE_2_INDEX, VALVE_ON);
        screen_const_puts("开");
    }else{
        gpio_output(VALVE_2_GRP, VALVE_2_INDEX, VALVE_OFF);
        screen_const_puts("关");
    }
    screen_const_puts("',15,0);\n");
        /* now power up all pumps */
    gpio_output(VALVE_POWER_GRP, VALVE_POWER_INDEX, VALVE_ON);
}
