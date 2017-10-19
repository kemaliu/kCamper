/*
 *
 * TANK2---------temp_tank2--->pump2------flow_tank2--|
 *                                                    +--switch main                 +---switch2---->TANK2
 *                                                    |            |                  |                 
 * TANK1-----+--temp_tank1--->pump1------flow_tank1---|            |                  |
 *                                                                 |                  +---switch1---->TANK1
 *                                                                 |                  |
 *                                                                 ---main pump---->HEATER-------->normal usage
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
 *  scene:
 *                   | bit5   |  bit4    |       bit2     |    bit1    |    bit0   |
 *                   | pump1  |  pump2   |   switch main  |  switch 1  |  switch 2 |
 *  normal           |   off  |   off    |     off        |  off       |   off     |
 *  water tank1->2   |   on   |   off    |     on         |  off       |   on      |
 *  water tank1 loop |   on   |   off    |     on         |  on        |   off     |
 *  water tank2 loop |   off  |   on     |     on         |  off       |   on      |
 *  water tank2->1   |   off  |   on     |     on         |  on        |   off     |
 */


#include <stdlib.h>
#include "lib/uart.h"
#include "lib/timer.h"
#include "lib/flow.h"
#include "lib/kconfig.h"
#include "ui.h"
#include "lib/ds18b20_simple_fix.h"
#include <util/delay.h>
#include <avr/interrupt.h>

static short temperature[TEMPERATURE_SENSOR_MAX_NUM];

/* - scene:
 *                   |  bit4    |       bit2     |    bit1    |    bit0   |
 *                   |  pump2   |   switch main  |  switch 1  |  switch 2 |
 *  normal           |   off    |     off        |  off       |   off     |
 *  water tank1->2   |   off    |     on         |  off       |   on      |
 *  water tank1 loop |   off    |     on         |  on        |   off     |
 *  water tank2 loop |   on     |     on         |  off       |   on      |
 *  water tank2->1   |   on     |     on         |  on        |   off     |

 */

enum{
    SWITCH_MASK_PUMP = 0x10,                /* bit4 */
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
    SCENE_TOTAL_NUM,
};

unsigned char __scene_cfg[SCENE_TOTAL_NUM] = {
    SWITCH_MASK_PUMP,           /* normal */
    SWITCH_MASK_PUMP | SWITCH_MASK_TANK2, /* tank1->tank2 */
    SWITCH_MASK_PUMP | SWITCH_MASK_TANK1, /* tank1 loop */
    SWITCH_MASK_PUMP | SWITCH_MASK_MAIN | SWITCH_MASK_TANK2, /* tank2 loop */
    SWITCH_MASK_PUMP | SWITCH_MASK_MAIN | SWITCH_MASK_TANK1, /* tank2 loop */
};

void valve_setup(UINT8 scene)
{
    UINT8 cfg = __scene_cfg[scene];
    screen_const_puts("LABL(16,175,116,239,'进水:");
    if(cfg & SWITCH_MASK_MAIN){
        screen_const_puts("2");
    }else{
        screen_const_puts("1");
    }
    screen_const_puts("',15,0);\n");
    
    screen_const_puts("LABL(16,175,133,239,'出水1:");
    if(cfg & SWITCH_MASK_TANK1){
        screen_const_puts("开");
    }else{
        screen_const_puts("关");
    }
    screen_const_puts("',15,0);\n");

    screen_const_puts("LABL(16,175,150,239,'出水2:");
    if(cfg & SWITCH_MASK_TANK2){
        screen_const_puts("开");
    }else{
        screen_const_puts("关");
    }
    screen_const_puts("',15,0);\n");
}
enum{
    PUMP_OFF,
    PUMP_LOW_SPEED,
    PUMP_FULL_SPEED
};

/* 0:off  1:low speed 2:full speed */
void pump_enable(char enable)
{
    pump_status_show(enable);
}

#define WATER_TEMPERATURE_UNIT 1

#define WATER_WARM_TEMPERATURE_MAX 10
#define WATER_WARM_TEMPERATURE_MIN 2


#define WATER_HEAT_TEMPERATURE_MAX 45
#define WATER_HEAT_TEMPERATURE_MIN 25


#define WATER_LITE_UNIT 10

#define TANK1_WATER_LITE_MAX 70
#define TANK1_WATER_LITE_MIN 10


#define TANK2_WATER_LITE_MAX 90
#define TANK2_WATER_LITE_MIN 10





char * int_to_float_str(long val, int xv)
{
    static char buf[8];
    unsigned char pos = 0;
    if(val < 0){
        buf[pos++] = '-';
        val = 0-val;
    }
    sprintf(buf+pos, "%ld.%.2ld", val/xv, (val % xv) * 100/xv);
    return buf;
}

void ui_temp_update(unsigned char index)
{
    screen_cmd_printf("CELS(24,%d,1,'", TEMPERATURE_SENSOR_TANK1==index?1:2);
    if(temperature[index] <= -1000)
        screen_const_puts("NA");
    else
        screen_cmd_puts(int_to_float_str(temperature[index], 16));
    screen_const_puts("',15,0,1);\n");
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

void flow_speed_update()
{
    UINT32 speed;
    static UINT32 old_speed[FLOW_NUM] = {0xffffffff, 0xffffffff};
    
    speed = flow_cnt_speed(FLOW_TANK1_OUT);
    if(speed != old_speed[FLOW_TANK1_OUT]){
        screen_const_puts("CELS(24,1,2,'");
        screen_cmd_puts(int_to_float_str(speed, 1071));
        screen_const_puts("',15,0,1);\n");
        old_speed[FLOW_TANK1_OUT] = speed;
    }

    speed = flow_cnt_speed(FLOW_TANK2_OUT);
    if(speed != old_speed[FLOW_TANK2_OUT]){
        screen_const_puts("CELS(24,2,2,'");
        screen_cmd_puts(int_to_float_str(speed, 1071));
        screen_const_puts("',15,0,1);\n");
        old_speed[FLOW_TANK2_OUT] = speed;
    }
    
}

static char rx_buf[16];
static unsigned char rx_pos = 0;
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
        }else if(rx_pos>4 && c==']'){ /* done */
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

void ui_status_update(char * mode_str, char * status_str)
{
    screen_const_puts(AREA_1_START);
        /* update data mode */
    screen_const_puts("LABL(24,40,9,174,'");
    screen_cmd_puts(mode_str);
    screen_const_puts("',4,0);");
        /* update status */
    screen_const_puts("LABL(24,0,50,174,'");
    screen_cmd_puts(status_str);
    screen_const_puts("',2,0);"); /* red, left align */
    screen_const_puts("SXY(0,0);\n");
}
static char working_info[32];
static char working_info_pending = 0;
void ui_working_info_show()
{
    if((working_info_pending)){
        screen_const_puts(AREA_1_START);
            /* update data mode */
        screen_const_puts("LABL(24,0,80,180,'");
        screen_cmd_puts(working_info);
        screen_const_puts("',15,0);SXY(0,0);\n");
        working_info_pending = 0;
    }
}

void ui_working_info_update(char * str)
{
    memcpy(working_info, str, 31);
    working_info[31] = 0;
    working_info_pending = 1;
}

void ui_mode_setting_show(char button, char destination)
{
    char status[16];
    char *mode;
    switch(button){
        case 0:
            mode = "正常用水";
            status[0] = '\0';
            break;
        case 2:             /* keep warm */
            mode = "自动保温";
            sprintf(status, "保温温度:%d", destination);
            break;
        case 3:             /* heat tank1 */
            mode = "主箱加温";
            sprintf(status, "加温温度:%d", destination);
            break;
        case 4:             /* tank2->tank1 */
            mode = "主箱加水";
            sprintf(status, "加水量:%dL", destination);
            break;
        case 5:             /* tank1->tank2 */
            mode = "副箱加水";
            sprintf(status, "加水量:%dL", destination);
            break;
        default:
            return;
    }
    ui_status_update(mode, status);
    
}

void button_blink(int button, int blink)
{
    char * fmt = AREA_2_START"LABL(24,%d,%d,%d,'%s',%d,1);SXY(0,0);\n";
    static UINT32 last_time = 0;
    static char color = 1;
    if(!blink){
        color = 15;
        goto do_update;
    }
    if(time_diff_ms(last_time) > 1000){
        last_time = timebase_get();
        color = (color==1)?2:1;
        goto do_update;
    }
    return;
  do_update:
    switch(button){
        case 2:             /* keep warm */
            screen_cmd_printf(fmt,2,17,118,BUTTON2_CONTENT,color);
            break;
        case 3:             /* heat tank1 */
            screen_cmd_printf(fmt,122,17,238,BUTTON3_CONTENT,color);
            break;
        case 4:             /* tank2->tank1 */
            screen_cmd_printf(fmt,2,67,118,BUTTON4_CONTENT,color);
            break;
        case 5:             /* tank1->tank2 */
            screen_cmd_printf(fmt,122,67,238,BUTTON5_CONTENT,color);
            break;
    }
}


static UINT32 scene_start_time = 0;
static UINT32 scene_done_time = 0;
static char scene_step = 0;
void scene_reset()
{
    scene_start_time = 0;
    scene_done_time = 0;
    scene_step = 0;
}

#define scene_time() ((UINT32)(sys_run_seconds() - scene_start_time))

/* return 0:processing 1:end */
#define STEP1_START_TIME
#define STEP2_START_TIME 1
#define STEP3_START_TIME 6
#define STEP4_START_TIME
int scene_process(UINT8 scene, char dest)
{
    char buf[32];
    UINT32 val;
    static UINT32 old_val = 0xffffffff;
    if(!scene_step){
        scene_start_time = sys_run_seconds();
        scene_step = 1;
            /* stop pump */
        pump_enable(PUMP_OFF);
        ui_working_info_update("停泵");
    }else if(scene_step==1 && scene_time()>STEP2_START_TIME){
        valve_setup(scene);
        scene_step=2;
        ui_working_info_update("设置阀门");
    }else if(scene_step==2 && scene_time()>STEP3_START_TIME){
        if(scene == SCENE_NORMAL){
            pump_enable(PUMP_FULL_SPEED);
            ui_working_info_update("开泵(全速)");
        }else{
            pump_enable(PUMP_LOW_SPEED);
            ui_working_info_update("开泵(低速)");
        }
        if(scene == SCENE_WATER_TANK1_TO_TANK2)
            flow_reset(FLOW_TANK2_OUT);
        if(scene == SCENE_WATER_TANK2_TO_TANK1)
            flow_reset(FLOW_TANK1_OUT);
        scene_step = 3;
    }else if(scene_step == 3){
            /* wait to reach destination */
        switch(scene){
            case SCENE_NORMAL:
                return 0;
            case SCENE_WATER_TANK1_TO_TANK2:
                break;
            case SCENE_WATER_TANK1_LOOP:
                
                break;
            case SCENE_WATER_TANK2_LOOP:
                
                break;
            case SCENE_WATER_TANK2_TO_TANK1:
                val = flow_cnt(FLOW_TANK1_OUT);
                if(val != old_val){
                    sprintf(buf,"已加:%sL", int_to_float_str(val, 1071));
                    ui_working_info_update(buf);
                    old_val = val;
                }
                break;
        }
    }
    return 0;
}


char operation_loop(char button, char destination)
{
    char scene_ret;
    switch(button){
        case 0:
            scene_ret = scene_process(SCENE_NORMAL, destination);
        case 2:                 /* keep warm */
                /* loop SCENE_WATER_TANK1_LOOP &  SCENE_WATER_TANK2_LOOP; */
            break;
        case 3:
                /* heat tank1 */
            scene_ret = scene_process(SCENE_WATER_TANK1_LOOP, destination);
            break;
        case 4:
                /* tank2->tank1 */
            scene_ret = scene_process(SCENE_WATER_TANK2_TO_TANK1, destination);
            break;
        case 5:
                /* tank1->tank2 */
            scene_ret = scene_process(SCENE_WATER_TANK1_TO_TANK2, destination);
            break;
    }
    return 0;
}


static inline void main_opr()
{
    static unsigned char active_button = 0;
    static char param_lock = 1; /* 0: param changing allowed
                                   1:param changing forbidened 
                                   default active_button forbiden param changing
                                */
    static unsigned int destination; /* operation destination
                                      * SCENE_NORMAL: useless
                                      * SCENE_WATER_TANK1_TO_TANK2: how many lites to add
                                      * SCENE_WATER_TANK1_LOOP: tank1 heat/warm temperature
                                      * SCENE_WATER_TANK2_LOOP: tank2 wam temperature
                                      * SCENE_WATER_TANK2_TO_TANK1: how many lites to add
                                      */
    char status_need_update = 0;
    
    static UINT32 param_mod_time = 0;
        /* poll & display key info */
    if(button>=0){
        switch(button){
            case 0:             /* screen press */
                break;
            case 1:             /* change setting */
                if((param_lock)) 
                    return;   /* ignore button 1 input*/
                switch(active_button){
                    case 2:             /* keep warm */
                            /* warm tank1/tank2 warming */
                        destination = WATER_WARM_TEMPERATURE_MIN + 
                            (destination - WATER_WARM_TEMPERATURE_MIN + 1)%
                            (WATER_WARM_TEMPERATURE_MAX-WATER_WARM_TEMPERATURE_MIN+WATER_TEMPERATURE_UNIT);
                        break;
                    case 3:             /* heat tank1 */
                        destination = WATER_HEAT_TEMPERATURE_MIN + 
                            (destination - WATER_HEAT_TEMPERATURE_MIN + 1)%
                            (WATER_HEAT_TEMPERATURE_MAX-WATER_HEAT_TEMPERATURE_MIN+WATER_TEMPERATURE_UNIT);
                            /* change heat temperature */
                        break;
                    case 4:             /* tank2->tank1 */
                            /* change Lites to add */
                        destination = TANK1_WATER_LITE_MIN + 
                            (destination - TANK1_WATER_LITE_MIN + 10)%
                            (TANK1_WATER_LITE_MAX-TANK1_WATER_LITE_MIN+WATER_LITE_UNIT);
                        break;
                    case 5:             /* tank1->tank2 */
                        destination = TANK2_WATER_LITE_MIN + 
                            (destination - TANK2_WATER_LITE_MIN + 10)%
                            (TANK2_WATER_LITE_MAX-TANK2_WATER_LITE_MIN+WATER_LITE_UNIT);
                            /* change Lites to add */
                        break;
                }
                param_mod_time = sys_run_seconds();
                status_need_update = 1;
                break;
            case 2:             /* keep warm */
                destination = 4;
                status_need_update = 1;
                scene_reset();
                param_mod_time = sys_run_seconds();
                break;
            case 3:             /* heat tank1 */
                destination = 35;
                status_need_update = 1;
                scene_reset();
                param_mod_time = sys_run_seconds();
                break;
            case 4:             /* tank2->tank1 */
                destination = 30;
                status_need_update = 1;
                scene_reset();
                param_mod_time = sys_run_seconds();
                break;
            case 5:             /* tank1->tank2 */
                destination = 30;
                status_need_update = 1;
                scene_reset();
                param_mod_time = sys_run_seconds();
                break;
        }
        if(button>=2 && button <= 5){
            button_blink(active_button, 0); /* stop blinking */
            if(active_button == button){
                    /* disable the function */
                param_lock = 1; /* scene 0 need not param setting */
                active_button = 0;
                scene_reset();
                param_mod_time = sys_run_seconds();
            }else{
                param_lock = 0;
                active_button = button;
                param_mod_time = sys_run_seconds();
            }
        }
        if((status_need_update))
            ui_mode_setting_show(active_button, destination);
            /* clr button value */
        button = -1;
    }

    /* button blink */
    button_blink(active_button, 1); /* blinking current button */
    
    if(sys_run_seconds() - param_mod_time < 5){
        static char last_diff = 10;
        if(sys_run_seconds() - param_mod_time != last_diff){
            char *time_buf = "0秒后开始工作";
            time_buf[0] = '0' + (5 - (sys_run_seconds() - param_mod_time));
            ui_working_info_update(time_buf);
            last_diff = sys_run_seconds() - param_mod_time;
        }
        ui_working_info_show();
        return;
    }
    param_lock = 1;
    operation_loop(active_button, destination);
    ui_working_info_show();
}



int main()
{
        /* init uart */
    init_uart(19200);
        /* init timer */
    timer_init();
        /* init temperature */
    ds_init();
        /* init flow ctrl */
    flow_init();
    screen_const_puts("SPG(1);\n"); /* display main page */    
    _delay_ms(300);               /* wait 100ms for main page drawing */
    draw_main_page();

#if 0
    screen_const_puts("TERM;\n"); /* display main page */
#endif
    while(1){
        temp_update();
        flow_speed_update();
        main_opr();
        _delay_ms(200);
    }
}


char realTimeFmtBuf[32];
