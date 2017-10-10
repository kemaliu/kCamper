/*
 *
 *    加水口<----------<---switch_tank2<--pumb_tank2<--flow_tank2<--temp_tank2<--副水箱出口
 *                    ^
 *                    |
 *                    |                            
 *                  switch_tank1_loop
 *                    ^
 *                    |
 *                    |
 *    热水---------------switchB---------------------------->副水箱进水
 *      ^
 *      +---------------------------------------------
 *                                                   ^
 *    冷水无压---------temp_cold--->pump_Cold---flow_tank1---->换热器
 *              |                          |
 *              |                          |
 *              +-----原配水泵-------------->
 *
 *
 *    制冷液----------temp_hot
 *
 * - 功能：
 *  - 主水箱保温
 *  - 副水箱保温
 *  - 主水箱加热到温水温度
 *  - 主水箱过热时，与副水箱循环到较低温度
 *  - 异常时蜂鸣器报警
 *   - 副水箱无水  两长
 *   - 主水箱无水  长鸣
 *   - 制冷液过冷  三长

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
int main()
{
    int cnt = 0;
    short temp[TEMPERATURE_SENSOR_MAX_NUM];
    int i;
        /* init uart */
    init_uart(19200);
        /* init temperature */
    ds_init();
        /* init flow ctrl */
    flow_init();
    while(1){
        _delay_us(100);
        printf("--kCamper %d\n", cnt++);
        temp[TEMPERATURE_SENSOR_TANK1] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_TANK1);
        temp[TEMPERATURE_SENSOR_TANK2] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_TANK2);
        temp[TEMPERATURE_SENSOR_HEATER] = ds_get_temperature_x16((char)TEMPERATURE_SENSOR_HEATER);
#if 1
            /* display info */
        printf("status:\n");
        put_s("  temperature:\n");
        put_s("    TANK1:");
        if(temp[TEMPERATURE_SENSOR_TANK1] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_TANK1]/16);

        put_s("    TANK2:");
        if(temp[TEMPERATURE_SENSOR_TANK2] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_TANK2]/16);
        
        put_s("    HEATER:");
        if(temp[TEMPERATURE_SENSOR_HEATER] <= -1000)
            printf("NA\n");
        else
            printf("%d\n", temp[TEMPERATURE_SENSOR_HEATER]/16);
        
        
        printf("  flow: TANK1  %lu TANK2  %lu\n", flow_num(FLOW_TANK1_OUT), flow_num(FLOW_TANK2_OUT));
#endif
        
        _delay_ms(1000);
    }
}
