/*
 *
 *    ��ˮ��<----------<---switch_tank2<--pumb_tank2<--flow_tank2<--temp_tank2<--��ˮ�����
 *                    ^
 *                    |
 *                    |                            
 *                  switch_tank1_loop
 *                    ^
 *                    |
 *                    |
 *    ��ˮ---------------switchB---------------------------->��ˮ���ˮ
 *      ^
 *      +---------------------------------------------
 *                                                   ^
 *    ��ˮ��ѹ---------temp_cold--->pump_Cold---flow_tank1---->������
 *              |                          |
 *              |                          |
 *              +-----ԭ��ˮ��-------------->
 *
 *
 *    ����Һ----------temp_hot
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
 *   flow_tank2_out:PC3(PCINT12)
 *   switch_tank2&pumb_tank2:  PD2
 *   switchB:PD3
 *   switch_tank1_loop:PD4
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

char * int_to_float_str(int val, int xv)
{
    static char buf[8];
    char pos = 0;
    if(val < 0){
        buf[pos++] = '-';
        val = 0-val;
    }
    sprintf(buf+pos, "%d.%2d", val/xv, (val % xv) * 100/xv);
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



