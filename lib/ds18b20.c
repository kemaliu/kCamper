#include "ktype.h"
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "debug.h"
#define MATCH_ROM 0x55
#define SKIP_ROM 0xcc
#define CONVERT_START 0x44
#define READ_ALL 0xbe


#if 0
#define PERROR(fmt, arg...) printk(fmt, ##arg)
#else
#define PERROR(fmt, arg...) do{}while(0)
#endif




UINT8 CRC8Table[] = {
  0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
  157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
  35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
  190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
  70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
  219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
  101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
  248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
  140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
  17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
  175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
  50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
  202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
  87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
  233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
  116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};


unsigned char crc_check(unsigned char *p, char counter)
{
    unsigned char crc8 = 0;

    for( ; counter > 0; counter--){
        crc8 = CRC8Table[crc8^*p];
        p++;
    }
    return(crc8);

}



#define MAX_DEVICE_NUM 2
static UINT8 __devices_id[MAX_DEVICE_NUM][8];
static UINT8 __devices_num = 0;
static UINT8 __conflict_num = 0;
static UINT8 __conflict_pos[MAX_DEVICE_NUM];
static UINT8 __conflict_state[MAX_DEVICE_NUM];








static inline void ds_pin_input()
{
    DDRD &= ~0x8;		/* set input */
    PORTD |= 0x8;		/* pull up */
}


static inline void ds_pin_output()
{
    DDRD |= 0x8;		/* set input */
    PORTD |= 0x8;		/* output high */
}

static inline void ds_pin_high()
{
    PORTD |= 0x8;		/* output high */
}

static inline void ds_pin_low()
{
    PORTD &= ~0x8;		/* output high */
}

static inline UINT8 ds_pin_get()
{
    return (PIND >> 3) & 1;
}



/* use PD3 */
void ds_init()
{
    ds_pin_output();
}

INT8 ds_reset()
{
    int cnt = 0, ok_cnt = 0;
    ds_pin_output();
    ds_pin_low();		/* low for 600us */
    _delay_us(600);
    ds_pin_input();
    _delay_us(15);
    while(cnt++ < 500){
	if(ds_pin_get() == 0){
	    ok_cnt++;
	}
	_delay_us(1);
    }
    if(ok_cnt > 60)
      return 0;
    else
      return -1;
}


void ds_write_bit(UINT8 val)
{
    ds_pin_low();		/* low for 2us */
    _delay_us(2);
    cli();			/* 关中断 */
    if(val > 0){
	ds_pin_high();		/* low for 2us */
    }else{
	ds_pin_low();		/* low for 2us */
    }
    _delay_us(60);		/* hold for 60 seconds */
    /* pull to high */
    ds_pin_high();
    sei();			/* 开中断 */
    _delay_us(2);
}


void ds_write_byte(UINT8 val)
{
    int i;
    ds_pin_output();
    for(i=0; i<8; i++){
	ds_write_bit((val >> i) & 1);
    }
    ds_pin_output();
}


UINT8 ds_read_bit()
{
    UINT8 ret;
    ds_pin_output();
    cli();			/* 关中断 */
    ds_pin_low();		/* low for 2us */
    _delay_us(2);
    ds_pin_input();
    _delay_us(5);
    ret = ds_pin_get();
    _delay_us(60);
    ds_pin_output();
    ds_pin_high();
    sei();			/* 开中断 */
    _delay_us(1);
    
    return ret;
}

UINT8 ds_read_byte()
{
    int i;
    UINT8 val = 0;
    ds_pin_output();
    for(i=0; i<8; i++){
	val |= (ds_read_bit() << i);
    }
    ds_pin_output();
    return val;
}



INT16 ds_get_temperature()
{
    UINT32 wait_cnt = 0;
    UINT8 high, low;
    UINT16 temperature;
    ds_reset();
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(0xcc);
    //开始转换
    ds_write_byte(0x44);
    ds_pin_input();
    /*温度采集时，读回的是0, 1表示采集结束*/
    while(wait_cnt++ < 20000){	/* 超时20Kx60us = 1200ms = 1.2s */
	if(ds_read_bit() == 1)	/* about 60us every read */
	  break;
    }
    if(wait_cnt >= 20000)
      return -1000;		/* -1000表示读取失败 */
    /* start read */
    ds_reset();
    //跳过ROM，不读地址，直接通讯
    ds_write_byte(0xcc);
    //开始转换
    ds_write_byte(0xbe);
    low = ds_read_byte();
    high = ds_read_byte();
    temperature = (low>>4)+((UINT16)high<<4);
    return temperature;
}

/* 直接阻塞式读取温度 */
INT16 ds_get_id_temperaturex16(UINT8 index)
{
    UINT32 retry_cnt = 0, wait_cnt = 0;
    UINT8 *high, *low;
    INT16 temperature;
    UINT8 buf[9];
    UINT8  i;
    UINT8 * id;
    if(index >= __devices_num){
	return -1000;
    }
 again:
    id = &__devices_id[index][0];
    ds_reset();
    /* 选择ROM */
    ds_write_byte(MATCH_ROM);
    for(i=0; i<8; i++){
	ds_write_byte(id[i]);
    }
    //开始转换
    ds_write_byte(CONVERT_START);
    ds_pin_input();
    /*温度采集时，读回的是0, 1表示采集结束*/
    while(wait_cnt++ < 20000){	/* 超时20Kx60us = 1200ms = 1.2s */
	if(ds_read_bit() == 1)	/* about 60us every read */
	  break;
    }
    if(wait_cnt >= 20000)
      return -1000;		/* -1000表示读取失败 */
    /* start read */
    ds_reset();
    //跳过ROM，不读地址，直接通讯
    /* 选择ROM */
    ds_write_byte(MATCH_ROM);
    for(i=0; i<8; i++){
	ds_write_byte(id[i]);
    }
    ds_write_byte(READ_ALL);
    for(i=0; i<9; i++){
	buf[i] = ds_read_byte();
    }
    
    if(crc_check(buf, 9) != 0){
	if(retry_cnt++ < 5){
	    PERROR("crc failed, retry\n");
	    goto again;    
	}else{
	    return -1000;
	}
	
    }
    low = &buf[0];
    high = &buf[1];
    temperature = (UINT16)(*low)+((UINT16)*high<<8);
    return temperature;
}





static UINT8 temperature_runtime_state = 0;
static UINT16 temperature_runtime_wait_cnt;
void ds_sample_check()		/* 每1ms调用一次该函数 */
{
    if(temperature_runtime_state != 2){
	temperature_runtime_wait_cnt = 0;
	return;
    }
    if(ds_read_bit() == 1){
	temperature_runtime_wait_cnt = 0x8000;
    }else{
	if(temperature_runtime_wait_cnt < 1200)
	  temperature_runtime_wait_cnt++;
	else{
	    temperature_runtime_wait_cnt = 0x8001;
	}
    }
    
}
/** 非阻塞式阻塞式读取温度 
 * @param index
 * @return -2000:io 错误, -1001:操作尚未结束
 */
INT16 ds_get_id_temperaturex16_unblock(UINT8 * pIndex)
{
    UINT8  i;
    if(temperature_runtime_state == 0){
	/* 复位 */
	if(0 != ds_reset()){
	    /* 复位失败,严重错误*/
	    temperature_runtime_state = 0;
	    return -2000;
	}else{
	    temperature_runtime_state = 1;
	    return -1001;
	}
    }else if(temperature_runtime_state == 1){
	/* 发送SKIP_ROM + 温度采集指令, 需要写入16bit, 
		 耗时大约64usx16 = 1.024ms */
	ds_reset();
	/* 向所有温传发送采集温度指令 */
	ds_write_byte(SKIP_ROM);
	//开始转换
	ds_write_byte(CONVERT_START);
	ds_pin_input();
	cli();			/* 关中断 */
	temperature_runtime_wait_cnt = 0;
	sei();			/* 开中断 */
	temperature_runtime_state = 2;
	return -1001;
    }else if(temperature_runtime_state == 2){
	/* 等待温度采集结束 */
	if(temperature_runtime_wait_cnt < 0x8000){
	    /* 采样尚未就绪 */
	    return -1001;
	}else if(temperature_runtime_wait_cnt == 0x8000){
	    /* 采样结束,可以读取温度了 */
	    temperature_runtime_state = 0x10;
	}else{			/* 采样超时,重新开始采样 */
	    PERROR("timeout\n");
	    temperature_runtime_state = 0;
	}
	return -1001;
    }else if(temperature_runtime_state < 0x10){
	PERROR("bug ds18b20.c %d\n", __LINE__);
	temperature_runtime_state = 0;
	return -1001;
    }else if(temperature_runtime_state < 0x20){
	/* 读取 */
	UINT8 index = (temperature_runtime_state - 0x10) >> 2;
	UINT8 stage = (temperature_runtime_state - 0x10) & 3;
	UINT8 * id;
	static UINT8 buf[9];
	INT16 temperature;
	id = &__devices_id[index][0];
	if(index >= __devices_num){ 
	    /* 没有该温传， 重新采样 */
	    temperature_runtime_state = 0;
	    return -1001;
	}
	switch(stage){
	  case 0:
	    /* 复位耗时约1ms */
	    if(0 != ds_reset()){
		/* 复位失败,严重错误*/
		temperature_runtime_state = 0;
		PERROR("reset failed...\n");
		return -2000;
	    }
	    /* 写入读取温度命令, 并写入3byte的rom, 64us x 24bit = 1.5ms */
	    ds_write_byte(MATCH_ROM);
	    for(i=0; i<3; i++){
		ds_write_byte(id[i]);
	    }
	    break;
	  case 1:		/* 写入剩余的5byte rom, 耗时 2.5ms */
	    for(i=3; i<8; i++){
		ds_write_byte(id[i]);
	    }
	    break;
	  case 2:		/* 写入读取命令，读取4Byte, 耗时 2.5ms */
	    ds_write_byte(READ_ALL);
	    for(i=0; i<4; i++){
		buf[i] = ds_read_byte();
	    }
	    break;
	  case 3:		/* 读取剩余5byte 耗时2.5ms */
	    for(i=4; i<9; i++){
		buf[i] = ds_read_byte();
	    }
	    /* 计算CRC */
	    if(crc_check(buf, 9) != 0){
		PERROR("%d crc failed\n", index);
		temperature_runtime_state++;
		return -2000;
	    }
	    temperature = (UINT16)buf[0]+((UINT16)buf[1]<<8);
	    *pIndex = index;
	    temperature_runtime_state++;
	    return temperature;
	  default:
	    PERROR("bug ds18b20.c %d\n", __LINE__);
	}
	temperature_runtime_state++;
	return -1001;
    }else{
	temperature_runtime_state = 0;
	PERROR("bug ds18b20.c %d\n", __LINE__);
	return -1001;
    }
}




void update_device_id(UINT8 pos, UINT8 bit)
{
    UINT8 * p;
    p = &__devices_id[__devices_num][0];
    p[pos>>3] |= bit << (pos & 7);
}

void set_conflict_bit_state(INT8 pos, UINT8 val)
{
    UINT8 i;
    for(i=0; i<__conflict_num; i++){
	if(pos == __conflict_pos[i]){
	    __conflict_state[i] = val;
	    return;
	}
    }
    printk("bug1 !!!!!!!!!!!!!!\n");
}

UINT8 get_conflict_bit_state(INT8 pos)
{
    UINT8 i;
    for(i=0; i<__conflict_num; i++){
	if(pos == __conflict_pos[i]){
	    return __conflict_state[i];
	}
    }
    printk("bug2 !!!!!!!!!!!!!!\n");
    return 255;
}

/* 识别一个ds18b20的ID */
INT8 ds_identify()
{
    UINT8 val, bit;
    UINT8 i;
    __devices_num = 0;
    __conflict_num = 0;
    memset(__devices_id, 0, sizeof(__devices_id));
    do{
	ds_reset();
	/*begin search rom */
	ds_write_byte(0xf0);
	for(i=0; i<64; i++){
	    val = (ds_read_bit() << 1) | ds_read_bit();
	    switch(val){
	      case 0:
		bit = 1;		/* conflict, use bit 1 */
		if(__conflict_num == 0 ||
		   i > __conflict_pos[__conflict_num - 1]){ /* 新发现的conflict */
		    bit = 0;			       /* 写0 */
		    __conflict_pos[__conflict_num] = i;
		    __conflict_num++;
		    set_conflict_bit_state(i, 0);
		}else if(i < __conflict_pos[__conflict_num - 1]){/* 扫描到最后一个冲突位 */
		    bit = get_conflict_bit_state(i); /* 获取之前使用的bit值 */
		}else{				  /* 最后一个冲突位 */
		    bit = 1;
		    set_conflict_bit_state(i, 1); /* 标记当前扫描到1 */
		}
		break;
	      case 1:
		bit = 0;
		break;
	      case 2:
		bit = 1;
		break;
	      case 3:
	      default:		/* no device */
		break;
	    }
	    ds_write_bit(bit);
	    update_device_id(i, bit);
	}
	if(0 != crc_check(&__devices_id[__devices_num][0], 8)){
	    printk("crc check failed %.2x-%.2x-%.2x-%.2x\n", 
		   __devices_id[__devices_num][0], 
		   __devices_id[__devices_num][1],
		   __devices_id[__devices_num][2],
		   __devices_id[__devices_num][3]
		  );
	    return 0;
	}else{
	    /* printk("found device %d: %x-%x-%x-%x-%x-%x-%x-%x\n", */
	    /* 	   __devices_num, */
	    /* 	   __devices_id[__devices_num][0], */
	    /* 	   __devices_id[__devices_num][1], */
	    /* 	   __devices_id[__devices_num][2], */
	    /* 	   __devices_id[__devices_num][3], */
	    /* 	   __devices_id[__devices_num][4], */
	    /* 	   __devices_id[__devices_num][5], */
	    /* 	   __devices_id[__devices_num][6], */
	    /* 	   __devices_id[__devices_num][7]); */
	    __devices_num++;
	}
	
	/* 检查冲突位搞定没有 */
	if(__conflict_num > 0){
	    for(i=__conflict_num-1; i>=0; i--){
		if(__conflict_state[i] == 1){
		    __conflict_num--;
		}else{
		    break;
		}
	    }
	}
	if(__conflict_num  ==  0)
	  break;
	if(__devices_num >= MAX_DEVICE_NUM){
	    printk("reach max device num %d\n", 8);
	    break;
	}
    }while(1);
    return __devices_num;
}

