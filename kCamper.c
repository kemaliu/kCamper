/*
 *
 *
 * TANK1---->|
 *           |                                            
 *       valve_input-->temp_cold----->flow_input--->pump----->HEATER---->temp hot------>normal usae
 *           |                                                                      |
 * TANK2---->|                                                                      |
 *                                                                                  |
 *                                                                                  +-------->valve 1----->TANK1
 *                                                                                  |
 *                                                                                  |
 *                                                                                  |
 *                                                                                  +-------->valve 2----->TANK2
 *
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
 *   flow_input:PC5(PCINT13)
 *   
 *   pump low: PD2
 *   pump low output: PD3
 *   valve_power: PD5, enable during valve setup
 *   valve main: PD6
 *   valve 1: PD7
 *   valve 2: PB0
 *
 *  scene:
 *                   |       bit2     |    bit1    |    bit0   |
 *                   |   valve main   |  valve 1   |  valve  2 |
 *  normal           |     off        |  off       |   off     |
 *  water tank1->2   |     on         |  off       |   on      |
 *  water tank1 loop |     on         |  on        |   off     |
 *  water tank2 loop |     on         |  off       |   on      |
 *  water tank2->1   |     on         |  on        |   off     |
 */


#include <stdlib.h>
#include "lib/uart.h"
#include "lib/timer.h"
#include "lib/flow.h"
#include "sensor.h"
#include "pump.h"
#include "valve.h"
#include "lib/kconfig.h"
#include "lib/gpio.h"
#include "ui.h"
#include "lib/ds18b20_simple_fix.h"
#include <util/delay.h>
#include <avr/interrupt.h>





enum{
    MODE_NORMAL = 0,
    MODE_WARM = 2,
    MODE_TANK1_HEAT,
    MODE_TANK2_TO_TANK1,
    MODE_TANK1_TO_TANK2,
    MODE_TANK2_HEAT,
};




#define WATER_TEMPERATURE_UNIT 1

#define WATER_WARM_TEMPERATURE_MAX 10
#define WATER_WARM_TEMPERATURE_MIN 2

/* attention heat min temperature must >  WATER_WARM_TEMPERATURE_MAX*/
#define WATER1_HEAT_TEMPERATURE_MAX 40
#define WATER1_HEAT_TEMPERATURE_MIN 25


#define WATER2_HEAT_TEMPERATURE_MIN (WATER_WARM_TEMPERATURE_MAX+1)
#define WATER2_HEAT_TEMPERATURE_MAX 25


#define WATER_LITE_UNIT 10

#define TANK1_WATER_LITE_MAX 70
#define TANK1_WATER_LITE_MIN 10


#define TANK2_WATER_LITE_MAX 90
#define TANK2_WATER_LITE_MIN 10



#define MODE_BOOT 0x80
static unsigned char current_mode = MODE_BOOT;

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
static char working_info[48];
static char working_info1[48];
static char working_info_pending = 0;
void ui_working_info_show()
{
    if((working_info_pending)){
        screen_const_puts(AREA_1_START);
            /* update data mode */
        screen_const_puts("LABL(16,0,80,174,'");
        screen_cmd_puts(working_info);
        screen_const_puts("',15,0);");
        screen_const_puts("LABL(16,0,100,174,'");
        screen_cmd_puts(working_info1);
        screen_const_puts("',15,0);SXY(0,0);\n");
        working_info_pending = 0;
    }
}

void ui_working_info_update(char * str)
{
    strncpy(working_info, str, 48);
    working_info1[0] = 0;
    working_info_pending = 1;
}

void ui_working_info_pending()
{
    working_info_pending = 1;
}

void ui_mode_setting_show(char button, char destination)
{
    char status[16];
    char *mode;
    switch(button){
        case MODE_NORMAL:
            mode = "正常用水";
            status[0] = '\0';
            break;
        case MODE_WARM:             /* keep warm */
            mode = "自动保温";
            sprintf(status, "保温温度:%d", destination);
            break;
        case MODE_TANK1_HEAT:             /* heat tank1 */
            mode = "主箱加温";
            sprintf(status, "加温温度:%d", destination);
            break;
        case MODE_TANK2_TO_TANK1:             /* tank2->tank1 */
            mode = "主箱加水";
            sprintf(status, "加水量:%dL", destination);
            break;
        case MODE_TANK1_TO_TANK2:             /* tank1->tank2 */
            mode = "副箱加水";
            sprintf(status, "加水量:%dL", destination);
            break;
        case MODE_TANK2_HEAT:             /* heat tank2 */
            mode = "副箱加温";
            sprintf(status, "加温温度:%d", destination);
            break;
        default:
            return;
    }
    ui_status_update(mode, status);
    
}

void button_blink(int mode, int blink)
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
    switch(mode){
        case MODE_WARM:             /* keep warm */
            screen_cmd_printf(fmt,2,17,118,BUTTON2_CONTENT,color);
            break;
        case MODE_TANK1_HEAT:             /* heat tank1 */
        case MODE_TANK2_HEAT:             /* heat tank1 */
            screen_cmd_printf(fmt,122,17,238,BUTTON3_CONTENT,color);
            break;
        case MODE_TANK2_TO_TANK1:             /* tank2->tank1 */
            screen_cmd_printf(fmt,2,67,118,BUTTON4_CONTENT,color);
            break;
        case MODE_TANK1_TO_TANK2:             /* tank1->tank2 */
            screen_cmd_printf(fmt,122,67,238,BUTTON5_CONTENT,color);
            break;
    }
}


static UINT32 scene_start_time = 0;
static UINT32 scene_done_time = 0;
static char scene_step = 3;
void scene_reset()
{
    scene_start_time = 0;
    scene_done_time = 0;
    scene_step = 0;
}

#define scene_time() ((UINT32)(sys_run_seconds() - scene_start_time))

/* return 0:processing 1:end */
#define STEP1_START_TIME 0
#define STEP2_START_TIME (STEP1_START_TIME + VALVE_OPR_TIME)
#define STEP3_START_TIME (STEP2_START_TIME + 0)
#define STEP_RUNNING 3
int scene_process(UINT8 scene, char dest)
{

    if(!scene_step){
        scene_start_time = sys_run_seconds();
        scene_step = 1;
            /* stop pump */
        pump_mode_set(PUMP_OFF);
        valve_setup(scene);
        ui_working_info_update("设置阀门");
    }else if(scene_step==1 && scene_time()>STEP2_START_TIME){
        scene_step=2;
        if(scene == SCENE_NORMAL){
            pump_mode_set(PUMP_FULL_SPEED);
            ui_working_info_update("开泵(全速)");
        }else{
            pump_mode_set(PUMP_LOW_SPEED);
            ui_working_info_update("开泵(低速)");
        }
        if(scene == SCENE_WATER_TANK1_TO_TANK2)
            flow_reset(FLOW_TANK2_OUT);
        if(scene == SCENE_WATER_TANK2_TO_TANK1)
            flow_reset(FLOW_TANK1_OUT);
    }else if(scene_step==2 && scene_time()>STEP3_START_TIME){
        scene_step = STEP_RUNNING;
    }else if(scene_step == 3){
    }
    return scene_step;
}


#define TANK1_WARM_INTERVAL 1800 /* 30min */
#define TANK2_WARM_INTERVAL 1800 /* 30min */
struct tank_warm_param_struct{
    UINT32 warm_end_time;
    UINT32 interval;
};

static struct warm_param_struct{
    struct tank_warm_param_struct tank[2];
    UINT32 start_time;
    char runing_flag;
    unsigned char ok_cnt;
}warm_param = {
    .tank[0].warm_end_time = 100000,
    .tank[0].interval = TANK1_WARM_INTERVAL,
    .tank[1].warm_end_time = 100000,
    .tank[1].interval = TANK2_WARM_INTERVAL,
    .start_time = 0,
    .runing_flag = 0,
    .ok_cnt = 0
};

#define WARM_RUN_SECONDS() (sys_run_seconds() - warm_param.start_time)
enum{
    WARM_IDLE = 0,
    WARM_RUNNING,
    WARM_DONE,
    
};


/* 
   index: 0:tank1 loop 1:tank2 loop 2:idle
 */
INT8 warm_temperature_process(UINT8 index, char destination, char * type)
{
    static UINT32 sec;
    static char empty_loop = 0;     /* warming water, heater is cold, only run pump for 60 seconds */
    char scene_ret;
    char isheat = 0;
    if(current_mode == MODE_TANK1_HEAT || current_mode == MODE_TANK2_HEAT){
        isheat = 1;             /* heating operation */
    }
    if(index >= 2){
        pump_mode_set(PUMP_OFF);
        if(sys_run_seconds() - sec > 5){
            sprintf(working_info,"主水箱%s%lu秒后运行", type, 
                    warm_param.tank[0].interval - (sys_run_seconds() - warm_param.tank[0].warm_end_time));
            sprintf(working_info1,"副水箱%s%lu秒后运行", type, 
                    warm_param.tank[1].interval - (sys_run_seconds() - warm_param.tank[1].warm_end_time));
            ui_working_info_pending();
        } 
        return WARM_IDLE;
    }
    if(!warm_param.runing_flag){
        scene_reset();
        warm_param.start_time = sys_run_seconds();
        warm_param.runing_flag = 1;
        warm_param.ok_cnt = 0;
        sec = 0;
    }
        /* do tank loop */
    scene_ret = scene_process(SCENE_WATER_TANK1_LOOP+index, destination);
    if(scene_ret >= 3){
            /*温度高过目标温度10度就运行*/
        if(get_temperature_int(TEMPERATURE_HEATER_ID) >= destination + 10){
                /* start working, start pump */
            if(pump_mode_get() != PUMP_LOW_SPEED)
                pump_mode_set(PUMP_LOW_SPEED);
            empty_loop = 0;
        }else  if(get_temperature_int(TEMPERATURE_HEATER_ID) < destination + 3){
            if((isheat)){
                    /* heat water */
                    /* heat temperature too low, stop pump */
                if(pump_mode_get() != PUMP_OFF){
                    pump_mode_set(PUMP_OFF);
                    strcpy(working_info,"加热器温度过低，暂停");
                    strcpy(working_info1,"");
                    ui_working_info_pending();
                }
                empty_loop = 0;
            }else{
                    /* warm water, at leat run 60 seconds to avoid pipe frozen */
                if(pump_mode_get() != PUMP_LOW_SPEED){
                    pump_mode_set(PUMP_LOW_SPEED);
                }
                empty_loop = 1;
            }
        }else{
            empty_loop = 0;
        }
        if(pump_mode_get() != PUMP_LOW_SPEED)
            return WARM_RUNNING;            
        if(WARM_RUN_SECONDS() > sec){
            sec = WARM_RUN_SECONDS();
            sprintf(working_info,"%s水箱%s,运行%lu秒", (!index)?"主":"副", type, sec);
            sprintf(working_info1,"达标计数:%d/50,%d/%d", warm_param.ok_cnt, get_temperature_int(TEMPERATURE_COLD_ID), destination);
            ui_working_info_pending();
        }
        if(WARM_RUN_SECONDS() > 120){
            if(get_temperature_int(TEMPERATURE_COLD_ID) >= destination){
                warm_param.ok_cnt++;
            }else{
                warm_param.ok_cnt = 0;
            }
            if(warm_param.ok_cnt >= 50 || 
               ((empty_loop) && WARM_RUN_SECONDS() > 60)){
                    /* warm done, power off pump */
                pump_mode_set(PUMP_OFF);
                    /* update tank1 warm end time */
                warm_param.tank[index].warm_end_time = sys_run_seconds();
                warm_param.runing_flag = 0;
                return WARM_DONE;
            }
            
        }
    }
    return WARM_RUNNING;
}


void warm_process(char destination)
{
    if(sys_run_seconds() - warm_param.tank[0].warm_end_time > warm_param.tank[0].interval){
            /* do tank1 warm */
        warm_temperature_process(0, destination, "保温");
    }else if(sys_run_seconds() - warm_param.tank[1].warm_end_time > warm_param.tank[1].interval){
            /* do tank2 warm */
        warm_temperature_process(1, destination, "保温");
    }else{
            /* warm idle */
        warm_temperature_process(2, 0, "保温");
    }
}



char mode_process(char mode, char destination)
{
    char scene_ret;
    char buf[48];
    UINT32 val;
    static UINT32 old_val = 0xffffffff;
    switch(mode){
        case 0:
            scene_ret = scene_process(SCENE_NORMAL, destination);
            break;
        case MODE_WARM:                 /* keep warm */
            warm_process(destination);
            break;
        case MODE_TANK1_HEAT:
        case MODE_TANK2_HEAT:
                /* heat tank1/tank2 */
                /* check cold water temperature until reaching destination */
            if(WARM_DONE == warm_temperature_process(MODE_TANK1_HEAT==mode?0:1, destination, "加热")){ /* done */
                mode_switch(MODE_NORMAL);
                ui_mode_setting_show(MODE_NORMAL, 0);
            }
            break;
        case MODE_TANK2_TO_TANK1:
                /* tank2->tank1 */
            scene_ret = scene_process(SCENE_WATER_TANK2_TO_TANK1, destination);
            
            if(scene_ret >= 3){
                val = flow_cnt(FLOW_TANK1_OUT);
                if(val > (UINT32)destination * 1071){
                        /* done, return to mode normal */
                    mode_switch(MODE_NORMAL);
                    ui_mode_setting_show(MODE_NORMAL, 0);
                }else{
                        /* update working info */
                    if(val != old_val){
                        sprintf(buf,"已加:%sL/%dl", int_to_float_str(val, 1071), destination);
                        ui_working_info_update(buf);
                        old_val = val;
                    }
                }
            }
            
            break;
        case MODE_TANK1_TO_TANK2:
                /* tank1->tank2 */
            scene_ret = scene_process(SCENE_WATER_TANK1_TO_TANK2, destination);
            if(scene_ret >= 3){
                    /* update working info */
                val = flow_cnt(FLOW_TANK2_OUT);
                if(val > (UINT32)destination * 1071){
                        /* done, return to mode normal */
                    mode_switch(MODE_NORMAL);
                    ui_mode_setting_show(MODE_NORMAL, 0);
                }else{
                        /* update flow amount info */
                    if(val != old_val){
                        sprintf(buf,"已加:%sL/%dl", int_to_float_str(val, 1071), destination);
                        ui_working_info_update(buf);
                        old_val = val;
                    }
                }
                
            }
            break;
    }
    return 0;
}





static char param_lock = 1; /* 0: param changing allowed
                               1:param changing forbidened 
                               default current_mode forbiden param changing*/
static UINT32 param_mod_time = 100000;
void mode_switch(char new_mode)
{
    button_blink(current_mode, 0); /* disable old blink */
    if(new_mode == MODE_NORMAL)
        param_lock = 1; /* scene 0 need not param setting */
    else
        param_lock = 0;
    current_mode = new_mode;
    scene_reset();
    param_mod_time = sys_run_seconds();
    warm_param.runing_flag = 0;
}

static inline void main_opr()
{
    static unsigned int destination; /* operation destination
                                      * SCENE_NORMAL: useless
                                      * SCENE_WATER_TANK1_TO_TANK2: how many lites to add
                                      * SCENE_WATER_TANK1_LOOP: tank1 heat/warm temperature
                                      * SCENE_WATER_TANK2_LOOP: tank2 wam temperature
                                      * SCENE_WATER_TANK2_TO_TANK1: how many lites to add
                                      */
    char status_need_update = 0;
    
        /* poll & display key info */
    if(button>=0){
        switch(button){
            case 0:             /* screen press */
                break;
            case 1:             /* change setting */
                if((param_lock)) 
                    break;;   /* ignore button 1 input*/
                switch(current_mode){
                    case MODE_WARM:             /* keep warm */
                            /* warm tank1/tank2 warming */
                        destination = WATER_WARM_TEMPERATURE_MIN + 
                            (destination - WATER_WARM_TEMPERATURE_MIN + 1)%
                            (WATER_WARM_TEMPERATURE_MAX-WATER_WARM_TEMPERATURE_MIN+WATER_TEMPERATURE_UNIT);
                        break;
                    case MODE_TANK1_HEAT:             /* heat tank1 */
                        destination = WATER1_HEAT_TEMPERATURE_MIN + 
                            (destination - WATER1_HEAT_TEMPERATURE_MIN + 1)%
                            (WATER1_HEAT_TEMPERATURE_MAX-WATER1_HEAT_TEMPERATURE_MIN+WATER_TEMPERATURE_UNIT);
                            /* change heat temperature */
                    case MODE_TANK2_HEAT:             /* heat tank2 */
                        destination = WATER2_HEAT_TEMPERATURE_MIN + 
                            (destination - WATER2_HEAT_TEMPERATURE_MIN + 1)%
                            (WATER2_HEAT_TEMPERATURE_MAX-WATER2_HEAT_TEMPERATURE_MIN+WATER_TEMPERATURE_UNIT);                     
                        break;
                    case MODE_TANK2_TO_TANK1:             /* tank2->tank1 */
                            /* change Lites to add */
                        destination = TANK1_WATER_LITE_MIN + 
                            (destination - TANK1_WATER_LITE_MIN + 10)%
                            (TANK1_WATER_LITE_MAX-TANK1_WATER_LITE_MIN+WATER_LITE_UNIT);
                        break;
                    case MODE_TANK1_TO_TANK2:             /* tank1->tank2 */
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
            case 3:             /* heat tank1/tank2 */
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
            if(button == 3){
                if(current_mode != MODE_TANK1_HEAT && current_mode != MODE_TANK2_HEAT){
                    mode_switch(MODE_TANK1_HEAT);
                    destination = 35;
                }else if(current_mode == MODE_TANK1_HEAT){
                    mode_switch(MODE_TANK2_HEAT);
                    destination = 15;
                }else{
                    mode_switch(MODE_NORMAL);
                }
            }else if(current_mode == button){
                    /* disable the function */
                mode_switch(MODE_NORMAL);
            }else{
                mode_switch(button);
            }
        }
        if((status_need_update))
            ui_mode_setting_show(current_mode, destination);
            /* clr button value */
        button = -1;
    }
    if(current_mode == MODE_BOOT){
        mode_switch(MODE_NORMAL);
    }
    /* button blink */
    button_blink(current_mode, 1); /* blinking current button */
    
    if(sys_run_seconds() - param_mod_time < 3){
        static char last_diff = 10;
        if(sys_run_seconds() - param_mod_time != last_diff){
            char *time_buf = "运行倒数:0";
            time_buf[9] = '0' + (3 - (sys_run_seconds() - param_mod_time));
            ui_working_info_update(time_buf);
            last_diff = sys_run_seconds() - param_mod_time;
        }
        ui_working_info_show();
        return;
    }
    param_lock = 1;
    mode_process(current_mode, destination);
    ui_working_info_show();
}



int main()
{
        /* init uart */
    init_uart(19200);
    pump_init();
        /* init timer */
    timer_init();
    draw_main_page();    
        /* init temperature */
    ds_init();
    
    temp_update();              /* begin temperature sample */
        /* init flow ctrl */
    flow_init();
        /* wait UI init done */
    _delay_ms(2000);
    valve_init();
    pump_mode_set(PUMP_OFF);
#if 0
    screen_const_puts("TERM;\n"); /* display main page */
#endif
    while(1){
        temp_update();
        flow_speed_update();
        main_opr();
        valve_check();          /* check pump */
        _delay_ms(200);
    }
}


char realTimeFmtBuf[32];
