#include "lib/ds18b20_simple_fix.h"
#include "lib/flow.h"
#include "lib/kconfig.h"
#include "lib/timer.h"
extern char * int_to_float_str(long val, int xv);
static short temperature[TEMPERATURE_SENSOR_MAX_NUM];


void ui_temp_update(unsigned char index)
{
    screen_cmd_printf("CELS(16,1,%d,'", 1 + index);
    if(temperature[index] <= -1000)
        screen_const_puts("NA");
    else
        screen_cmd_puts(int_to_float_str(temperature[index], 16));
    screen_const_puts("',15,0,1);\n");
}

void temp_update()
{
    static UINT32 last_sample_time = 0;
    static short old_temperature[TEMPERATURE_SENSOR_MAX_NUM] = {-1009, -1009, -1009};
    static char sample_err_num[TEMPERATURE_SENSOR_MAX_NUM];
    int ret;
    UINT8 i;
    if(!last_sample_time){
            /* do sample */
        for(i=0; i<TEMPERATURE_SENSOR_MAX_NUM; i++){
            ret = ds_get_temperature_sample(i);
            if(!ret)
                sample_err_num[i] = 0;
            else{
                sample_err_num[i]++;
                if(sample_err_num[i] > 64)
                    sample_err_num[i] = 64;
            }
        }
        last_sample_time = timebase_get();
    }else{
        if(time_diff_ms(last_sample_time) <= 2000) /* sample need wait 2 seconds */
            return;
        for(i=0; i<TEMPERATURE_SENSOR_MAX_NUM; i++){
            if(!sample_err_num[i]){ /* start sample return OK */
                temperature[i] = ds_get_temperature_read(i);
#if 1
                if(old_temperature[i] > -800){
                    temperature[i] = (old_temperature[i] * 4 + temperature[i])/5; /* avg temperature */
                }
#endif
            }else if(sample_err_num[i] < 5){
                    /* do nothing ,wait next sample&read */
            }else{
                    /* temperature NA now  */
                temperature[i] = -1000;
            }
            if(old_temperature[i] != temperature[i]){
                ui_temp_update(i);
                old_temperature[i] = temperature[i];
            }
        }
        last_sample_time = 0;
    }
}

char get_temperature_int(UINT8 index)
{
    return (char)(temperature[index] >> 4);
}





void flow_speed_update()
{
    UINT32 speed;
    UINT8 i, col;
    static UINT32 old_speed[FLOW_NUM] = {0xffffffff, 0xffffffff, 0xffffffff};
    
    for(i=0; i<FLOW_NUM; i++){
        speed = flow_cnt_speed(i);
        if(speed != old_speed[i]){
            switch(i){
                case FLOW_INPUT:
                    col = 3;
                    break;
                case FLOW_TANK1_OUT:
                    col = 1;
                    break;
                case FLOW_TANK2_OUT:
                    col = 2;
                    break;
            }
            screen_cmd_printf("CELS(16,3,%d,'", col);
            screen_cmd_puts(int_to_float_str(speed, 1071));
            screen_const_puts("',15,0,1);\n");
            old_speed[i] = speed;
        }
    }
}
