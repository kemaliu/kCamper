/*
 *
 * TANK2---------temp_tank2--->pump2------flow_tank2--|
 *                                                    +----switch main-----                 +---switch2---->TANK2
 *                                                    |                   |                 |                 
 * TANK1-----+--temp_tank1--->pump1------flow_tank1---|                   |                 |
 *           |                                                            |                 +---switch1---->TANK1
 *           |                                                            |                 |
 *           +-----large pump---------------------------------------------+-------->HEATER--+------->normal usage
 *
 * ����Һ----------temp_hot
 *
 * - ���ܣ�
 *  - ��ˮ�䱣��
 *  - ��ˮ�䱣��
 *  - ��ˮ����ȵ���ˮ�¶�
 *  - ��ˮ�����ʱ���븱ˮ��ѭ�����ϵ��¶�
 *  - �쳣ʱ����������
 *   - ��ˮ����ˮ  ����
 *   - ��ˮ����ˮ  ����
 *   - ����Һ����  ����
 * 
 * use internal OSC:
 * PB2/3/4/5 reserved for SPI
 * PB6/7 reserved for external OSC
 * PD0/1 reserved for UART1
 * PC6 reserved for reset
 * 
 * 
 * pinmux desc:
 *               QD
 *   temp_tank1: PC0
 *   temp_tank2: PC1
 *   temp_heater:PC2
 *   flow_tank1_out:PC3(PCINT11)
 *   flow_tank2_out:PC4(PCINT12)
 *   
 *   switch main: PC5
 *   switch 1: PD3
 *   switch 2: PD4
 *   pump 1: PD6
 *   pump 2: PD7
 *
 * - scene:
 *                   | pump1  |  pump2   |   switch main  |  switch 1  |  switch 2 |
 *  water tank1->2   |   on   |   off    |     on         |  off       |   on      |
 *  water tank1 loop |   on   |   off    |     on         |  on        |   off     |
 *  water tank2 loop |   off  |   on     |     on         |  off       |   on      |
 *  water tank2->1   |   off  |   on     |     on         |  on        |   off     |
 */



#include "lib/uart.h"
#include "lib/ds18b20.h"
#include "lib/timer.h"
#include "lib/flow.h"
#include "lib/kconfig.h"
#include "lib/ds18b20_simple_fix.h"
#include <util/delay.h>
#include <avr/interrupt.h>

static short temp[TEMPERATURE_SENSOR_MAX_NUM];

/* - scene:
 *                   | bit5   |  bit4    |       bit2     |    bit1    |    bit0   |
 *                   | pump1  |  pump2   |   switch main  |  switch 1  |  switch 2 |
 *  normal           |   off  |   off    |     off        |  off       |   off     |
 *  water tank1->2   |   on   |   off    |     on         |  off       |   on      |
 *  water tank1 loop |   on   |   off    |     on         |  on        |   off     |
 *  water tank2 loop |   off  |   on     |     on         |  off       |   on      |
 *  water tank2->1   |   off  |   on     |     on         |  on        |   off     |

 */

enum{
    SWITCH_MASK_PUMP_1 = 0x20,                /* bit5 */
    SWITCH_MASK_PUMP_2 = 0x10,                /* bit4 */
    SWITCH_MASK_MAIN = 4,                /* bit2 */
    SWITCH_MASK_TANK1 = 2,                /* bit1 */
    SWITCH_MASK_TANK2 = 1                /* bit0 */
};

enum{
    SCENE_NORMAL = 0,
    SCENE_WATER_TANK1_TO_TANK2 = 1,
    SCENE_WATER_TANK1_LOOP = 2,
    SCENE_WATER_TANK2_LOOP = 3,
    SCENE_WATER_TANK2_TO_TANK1 = 4,
}

unsigned char scene[4] = {
    0,
    SWITCH_MASK_PUMP_1 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK2,
    SWITCH_MASK_PUMP_1 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK1,
    SWITCH_MASK_PUMP_2 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK2,
    SWITCH_MASK_PUMP_2 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK1,
};

char * int_to_float_str(long val, int xv)
{
    static char buf[8];
    char pos = 0;
    if(val < 0){
        buf[pos++] = '-';
        val = 0-val;
    }
    sprintf(buf+pos, "%ld.%.2ld", val/xv, (val % xv) * 100/xv);
    return buf;
}

void ui_temp_update(char index)
{
    screen_cmd_printf("CELS(24,%d,1,'", TEMPERATURE_SENSOR_TANK1==index?1:2);
    if(temp[index] <= -1000)
        screen_cmd_puts("NA");
    else
        screen_cmd_puts(int_to_float_str(temp[index], 16));
    screen_cmd_puts("',15,0,1);\n");
}

void temp_update()
{
    temp[TEMPERATURE_SENSOR_TANK1] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_TANK1);
    ui_temp_update(TEMPERATURE_SENSOR_TANK1);
    temp[TEMPERATURE_SENSOR_TANK2] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_TANK2);
    ui_temp_update(TEMPERATURE_SENSOR_TANK2);
    temp[TEMPERATURE_SENSOR_HEATER] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_HEATER);
}

void flow_update()
{
    static UINT32 last_flow[FLOW_NUM] = {0, 0};
    UINT32 speed = flow_num(FLOW_TANK1_OUT);
    static UINT32 time_last = 0;
    UINT32 ms = time_diff_ms(time_last);
    time_last = timebase_get();
    
    speed = flow_num(FLOW_TANK1_OUT) - last_flow[FLOW_TANK1_OUT];
    speed = speed *60000/ms;
    screen_cmd_printf("tank1 speed %lu\n", speed/1071);
    last_flow[FLOW_TANK1_OUT] = flow_num(FLOW_TANK1_OUT);
    screen_cmd_printf("CELS(24,1,2,'");
    screen_cmd_puts(int_to_float_str(speed, 1071));
    screen_cmd_puts("',15,0,1);\n");

    speed = flow_num(FLOW_TANK2_OUT) - last_flow[FLOW_TANK2_OUT];
    speed = speed *60000/ms;
    screen_cmd_printf("tank2 speed %lu\n", speed/1071);
    last_flow[FLOW_TANK2_OUT] = flow_num(FLOW_TANK2_OUT); 
    screen_cmd_printf("CELS(24,2,2,'");
    screen_cmd_puts(int_to_float_str(speed, 1071));
    screen_cmd_puts("',15,0,1);\n");
    
}


int main()
{
    int cnt = 0;
    int i;
        /* init uart */
    init_uart(19200);
        /* init timer */
    timer_init();
        /* init temperature */
    ds_init();
        /* init flow ctrl */
    flow_init();
    screen_cmd_puts("SPG(1);\n"); /* display main page */    
    _delay_ms(300);               /* wait 100ms for main page drawing */
#if 0
    screen_cmd_puts("TERM;\n"); /* display main page */
#endif
    while(1){
        printf("--kCamper %d time %lu\n", cnt++, sys_run_seconds());
        temp_update();
        flow_update();
#if 0
        puts("  temperature:\n");
        puts("    TANK1:");
        if(temp[TEMPERATURE_SENSOR_TANK1] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_TANK1]/16);
        puts("    TANK2:");
        if(temp[TEMPERATURE_SENSOR_TANK2] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_TANK2]/16);
        
        puts("    HEATER:");
        if(temp[TEMPERATURE_SENSOR_HEATER] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_HEATER]/16);
        
        printf("  flow: TANK1  %lu TANK2  %lu\n", flow_num(FLOW_TANK1_OUT), flow_num(FLOW_TANK2_OUT));
#endif
        
        _delay_ms(1000);
    }
}



