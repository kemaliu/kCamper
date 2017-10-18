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
 * 制冷液----------temp_hot
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


#include <stdlib.h>
#include "lib/uart.h"
#include "lib/ds18b20.h"
#include "lib/timer.h"
#include "lib/flow.h"
#include "lib/kconfig.h"
#include "lib/ds18b20_simple_fix.h"
#include <util/delay.h>
#include <avr/interrupt.h>

static short temperature[TEMPERATURE_SENSOR_MAX_NUM];

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
};

/* unsigned char scene[4] = { */
/*     0, */
/*     SWITCH_MASK_PUMP_1 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK2, */
/*     SWITCH_MASK_PUMP_1 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK1, */
/*     SWITCH_MASK_PUMP_2 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK2, */
/*     SWITCH_MASK_PUMP_2 | SWITCH_MAIN_MASK | SWITCH_MASK_TANK1, */
/* }; */

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
    if(temperature[index] <= -1000)
        screen_cmd_puts("NA");
    else
        screen_cmd_puts(int_to_float_str(temperature[index], 16));
    screen_cmd_puts("',15,0,1);\n");
}

void temp_update()
{
    static UINT32 last_sample_time = 0;
    static short old_temperature[TEMPERATURE_SENSOR_MAX_NUM] = {-1000, -1000, -1000};
    if(!last_sample_time){
            /* do sample */
        last_sample_time = timebase_get();
        temperature[TEMPERATURE_SENSOR_TANK1] = ds_get_temperature_sample(TEMPERATURE_SENSOR_TANK1);
        temperature[TEMPERATURE_SENSOR_TANK2] = ds_get_temperature_sample(TEMPERATURE_SENSOR_TANK2);
        temperature[TEMPERATURE_SENSOR_HEATER] = ds_get_temperature_sample(TEMPERATURE_SENSOR_HEATER);
    }else{
        if(time_diff_ms(last_sample_time) <= 2000) /* sample need wait 2 seconds */
            return;
        if(!temperature[TEMPERATURE_SENSOR_TANK1]) /* start sample return OK */
            temperature[TEMPERATURE_SENSOR_TANK1] = ds_get_temperature_read(TEMPERATURE_SENSOR_TANK1);
        if(!temperature[TEMPERATURE_SENSOR_TANK2]) /* start sample return OK */
            temperature[TEMPERATURE_SENSOR_TANK2] = ds_get_temperature_read(TEMPERATURE_SENSOR_TANK2);
        if(!temperature[TEMPERATURE_SENSOR_HEATER]) /* start sample return OK */
            temperature[TEMPERATURE_SENSOR_HEATER] = ds_get_temperature_read(TEMPERATURE_SENSOR_HEATER);
        if(old_temperature[TEMPERATURE_SENSOR_TANK1] != temperature[TEMPERATURE_SENSOR_TANK1]){
            ui_temp_update(TEMPERATURE_SENSOR_TANK1);
            old_temperature[TEMPERATURE_SENSOR_TANK1] = temperature[TEMPERATURE_SENSOR_TANK1];
        }
        if(old_temperature[TEMPERATURE_SENSOR_TANK2] != temperature[TEMPERATURE_SENSOR_TANK2]){
            ui_temp_update(TEMPERATURE_SENSOR_TANK2);
            old_temperature[TEMPERATURE_SENSOR_TANK2] = temperature[TEMPERATURE_SENSOR_TANK2];
        }
        last_sample_time = 0;
    }
}

void flow_update()
{
    static UINT32 last_flow[FLOW_NUM] = {0, 0};
    UINT32 speed;
    static UINT32 old_speed[FLOW_NUM] = {0xffffffff, 0xffffffff};
    static UINT32 time_last = 0;
    UINT32 ms = time_diff_ms(time_last);
    time_last = timebase_get();
    
    speed = flow_num(FLOW_TANK1_OUT) - last_flow[FLOW_TANK1_OUT];
    speed = speed *60000/ms;
    
    last_flow[FLOW_TANK1_OUT] = flow_num(FLOW_TANK1_OUT);
    if(speed != old_speed[FLOW_TANK1_OUT]){
        screen_cmd_printf("CELS(24,1,2,'");
        screen_cmd_puts(int_to_float_str(speed, 1071));
        screen_cmd_puts("',15,0,1);\n");
        old_speed[FLOW_TANK1_OUT] = speed;
    }

    speed = flow_num(FLOW_TANK2_OUT) - last_flow[FLOW_TANK2_OUT];
    speed = speed *60000/ms;
    if(speed != old_speed[FLOW_TANK2_OUT]){
        last_flow[FLOW_TANK2_OUT] = flow_num(FLOW_TANK2_OUT); 
        screen_cmd_printf("CELS(24,2,2,'");
        screen_cmd_puts(int_to_float_str(speed, 1071));
        screen_cmd_puts("',15,0,1);\n");
        old_speed[FLOW_TANK2_OUT] = speed;
    }
    
}

static char rx_buf[16];
static char rx_pos = 0;
static char button=-1;
void uartRcv(uint8 c)
{
        /* filter [BN:4] */
    if(!rx_pos){
        if(c=='['){
            rx_buf[rx_pos++] = c;
        }else{
            goto do_clr;
        }
    }else{
        if(rx_pos==1 && c=='B'){
            rx_buf[rx_pos++] = c;
        }else if(rx_pos==2 && c=='N'){
            rx_buf[rx_pos++] = c;
        }else if(rx_pos==3 && c==':'){
            rx_buf[rx_pos++] = c;
        }else if(rx_pos>3 && c<='9' && c>='0'){
            rx_buf[rx_pos++] = c;
        }else if(rx_buf>4 && c==']'){ /* done */
            rx_buf[rx_pos++] = c;
            button = strtol(rx_buf+4, NULL, 10);
            goto do_clr;
        }else{
            goto do_clr;
        }
    }
    return;
  do_clr:
    rx_buf[0] = '\0';
    rx_pos = 0;
    
}

void ui_status_update(char * status_str)
{
    screen_cmd_puts("SXY(0,90);");
    screen_cmd_puts("LABL(24,0,50,239,'");
    screen_cmd_puts(status_str);
    screen_cmd_puts("',1,1);"); /* red, center */
    screen_cmd_puts("SXY(0,0);\n");
}




static inline void main_opr()
{
    static char scene;
    static blink = 0;
    char buf[16];
    if(button<0){
        
    }else{
        sprintf(buf, "%d", button);
        button = -1;
        ui_status_update(buf);
    }
    
}



static char main_ui_cmd[] = "DR3;"
"TPN(2);"
"CLS(0);"
"TABL(0,0,79,30,3,3,19);"
"CELS(24,0,0,'',15,0,1);"
"CELS(24,0,1,'温度',2,0,1);"
"CELS(24,0,2,'流速',2,0,1);"
"CELS(24,1,0,'主水箱',2,0,1);"
"CELS(24,1,1,'',2,0,1);"
"CELS(24,1,2,'',2,0,1);"
"CELS(24,2,0,'副水箱',2,0,1);"
"CELS(24,2,1,'',2,0,1);"
"CELS(24,2,2,'',2,0,1);"
"SBC(0);"
"SXY(0,90);BOXF(0,0,329,3,2);DS16(1,17,'状态:',2,0);LABL(24,40,9,179,'自动保温 ',1,0);BTN(1,180,5,239,35,4);LABL(24,181,9,238,'设置',15,1);SXY(0,0);"
    "SXY(0,210);BOXF(0,0,319,3,2);BTN(2,0,10,119,50,4);LABL(24,2,17,118,'自动保温',15,1);BTN(3,120,10,239,50,4);LABL(24,122,17,238,'水箱加热',15,1);BTN(4,0,60,119,100,4);LABL(24,2,67,118,'主箱加水',15,1);BTN(5,120,60,239,100,4);LABL(24,122,67,238,'副箱加水',15,1);SXY(0,0);\r\n";



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
    screen_cmd_puts(main_ui_cmd);
    _delay_ms(2000);               /* wait 100ms for main page drawing */
#if 0
    screen_cmd_puts("TERM;\n"); /* display main page */
#endif
    char blink = 0;
    while(1){
        temp_update();
        flow_update();
            /* do blink */

        main_opr();
        /* screen_cmd_printf("SXY(0,210);LABL(24,2,17,118,'自动保温',%d,1);SXY(0,0);\n", blink?2:1); */
        /* blink = !blink; */
        _delay_ms(100);
    }
}



