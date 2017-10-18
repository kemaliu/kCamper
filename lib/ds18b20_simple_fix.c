/* one wire only connect 1 ds18b20
 */
#include "ktype.h"
#include "gpio.h"
#include "kconfig.h"
#include "ds18b20_simple_fix.h"
#include <util/delay.h>
#include <avr/interrupt.h>

unsigned char  gpio_input(unsigned char group, unsigned char index, unsigned char pull_up);
static inline unsigned char ds_pin_input(char index)
{
    return gpio_input(GPIO_GROUP_C, index, 1); /* pull up */
}

static inline void ds_pin_output(char index)
{
    gpio_output(GPIO_GROUP_C, index, 1);
}


#define ds_pin_output_high(index) gpio_output(GPIO_GROUP_C, index, 1)

#define  ds_pin_output_low(index) gpio_output(GPIO_GROUP_C, index, 0)


void ds_init()
{
    ds_pin_output_high(TEMPERATURE_SENSOR_TANK1);
    ds_pin_output_high(TEMPERATURE_SENSOR_TANK2);
    ds_pin_output_high(TEMPERATURE_SENSOR_HEATER);
}



INT8 ds_reset(char index)
{
    int cnt = 0, ok_cnt = 0;
    ds_pin_output_high(index);
    _delay_us(10);
    ds_pin_output_low(index);		/* low for 600us */
    _delay_us(600);
    ds_pin_input(index);
    _delay_us(15);
        /* wait for response */
    while(cnt++ < 500){
        if(ds_pin_input(index) == 0){
            ok_cnt++;
        }
        
        _delay_us(1);
    }
    ds_pin_output_high(index);
    if(ok_cnt > 10){
        return 0;
    }else{
        return -1;
    }
}



void ds_write_bit(char index, UINT8 val)
{
    ds_pin_output_low(index);		/* low for 2us */
    _delay_us(2);
    cli();			/* 关中断 */
    if(val > 0){
        ds_pin_output_high(index);		/* high for 2us */
    }else{
        ds_pin_output_low(index);		/* low for 2us */
    }
    _delay_us(60);		/* hold for 60 us */
    /* pull to high */
    ds_pin_output_high(index);
    sei();			/* 开中断 */
    _delay_us(2);
}


void ds_write_byte(char index, UINT8 val)
{
    int i;
    ds_pin_output_high(index);
    for(i=0; i<8; i++){
        ds_write_bit(index, (val >> i) & 1);
    }
    ds_pin_output_high(index);
}


UINT8 ds_read_bit(char index)
{
    UINT8 ret;
    ds_pin_output_high(index);
    cli();			/* 关中断 */
    ds_pin_output_low(index);		/* low for 2us */
    _delay_us(2);
    ds_pin_input(index);        /* set pull up input mode */
    _delay_us(5);              /* wait 10us */
        /* do sample */
    ret = ds_pin_input(index);
    _delay_us(60);
    ds_pin_output_high(index);
    sei();			/* 开中断 */
    _delay_us(1);
    
    return ret;
}

UINT8 ds_read_byte(char index)
{
    int i;
    UINT8 val = 0;
    ds_pin_output_high(index);
    for(i=0; i<8; i++){
	val |= (ds_read_bit(index) << i);
    }
    ds_pin_output_high(index);
    return val;
}



INT16 ds_get_temperature_x16(char index)
{
    UINT8 high, low;
    UINT16 temperature;
    INT8 ret;
    ret = ds_reset(index);
    if(ret < 0){
        return -1001;
    }
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(index, 0xcc);
    //开始转换
    ds_write_byte(index, 0x44);
    
    ds_pin_input(index);
#if 1
        /* parasite power mode: output high instead of check QD for converting status*/
    ds_pin_output_high(index);
    _delay_ms(2000);
#else
        /* VCC power supply, can check QD for converting status */
    /*温度采集时，读回的是0, 1表示采集结束*/
    while(wait_cnt++ < 1500){	/* 超时1.5s */
        _delay_ms(1);
        if(ds_read_bit(index) == 1)
            break;
    }
    if(wait_cnt >= 1500){
        return -1000;		/* -1000表示读取失败 */
    }
#endif
        /* start read */
    ds_reset(index);
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(index, 0xcc);
    //开始转换
    ds_write_byte(index, 0xbe);
    low = ds_read_byte(index);
    high = ds_read_byte(index);

        /* now get temperature, powerdown chip */
    ds_pin_output_low(index);
    temperature = (INT16)((UINT16)low|((UINT16)high << 8));
    return temperature;
}




INT16 ds_get_temperature_sample(char index)
{
    INT8 ret;
    ret = ds_reset(index);
    if(ret < 0){
        return -1001;
    }
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(index, 0xcc);
    //开始转换
    ds_write_byte(index, 0x44);
    
    ds_pin_input(index);
        /* parasite power mode: output high instead of check QD for converting status*/
    ds_pin_output_high(index);
    return 0;
}



INT16 ds_get_temperature_read(char index)
{
    UINT8 high, low;
    UINT16 temperature;
        /* start read */
    ds_reset(index);
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(index, 0xcc);
    //开始转换
    ds_write_byte(index, 0xbe);
    low = ds_read_byte(index);
    high = ds_read_byte(index);

        /* now get temperature, powerdown chip */
    ds_pin_output_low(index);
    temperature = (INT16)((UINT16)low|((UINT16)high << 8));
    return temperature;
}
