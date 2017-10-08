#include <stdio.h>
#include "i2c.h"
#include "rtc.h"

/* set time */
void rtc_set(struct rtc_time * time)
{
    unsigned char buf[7];
    buf[0] = 0;
    I2C_Write_(0xd0, 0, buf, 1); /* clear second first */
    buf[0] = ((time->second/10) << 4) | (time->second%10);
    buf[1] = ((time->min/10) << 4) | (time->min%10);
    buf[2] = ((time->hour/10) << 4) | (time->hour%10);
    buf[3] = time->week;
    buf[4] = ((time->day/10) << 4) | (time->day%10);
    buf[5] = ((time->mon/10) << 4) | (time->mon%10);
    buf[6] = ((time->year/10) << 4) | (time->year%10);
    I2C_Write_(0xd0, 0, buf, 7);
}


void rtc_get(struct rtc_time * time)
{
    unsigned char buf[7];
    I2C_Read_(0xd0, 0, 0xd1, buf, 7);

    time->second = (buf[0] >> 4) * 10 + (buf[0] & 0xf);
    time->min = (buf[1] >> 4) * 10 + (buf[1] & 0xf);
    time->hour = (buf[2] >> 4) * 10 + (buf[2] & 0xf);
    time->week = buf[3];
    time->day = (buf[4] >> 4) * 10 + (buf[4] & 0xf);
    time->mon = (buf[5] >> 4) * 10 + (buf[5] & 0xf);
    time->year = (buf[6] >> 4) * 10 + (buf[6] & 0xf);
}



/* #include "uart.h" */
/* void rtc_test() */
/* { */
/*     unsigned char buf[8] = {1, 1, 1, 4, 0x27, 0x8, 0x13, 1}; */
/*     unsigned long cnt=0; */
    
/*     init_uart(); */
/*     I2C_init(); */
/*     uart_print("gogogog\n"); */
/*     /\* I2C_Write_(0xd0, 0, buf, 7); *\/ */
/*     uart_print("write done\n"); */
/*     while(1){ */
/* 	while(cnt++ < 0x10000); */
/* 	cnt = 0; */
/*     	I2C_Read_(0xd0, 0, 0xd1, buf, 3); */
/*     	uart_print("%x %x %x ", buf[0], buf[1], buf[2]); */
/*     	uart_print("%x %x %x ", buf[3], buf[4], buf[5]); */
/*     	uart_print("%x %x\n", buf[6], buf[7], buf[5]); */
/*     } */
/* } */

