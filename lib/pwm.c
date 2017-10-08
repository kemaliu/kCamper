#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>
#include "rtc.h"
#include "uart.h"

extern void time_64second_update();
static volatile struct rtc_time __local_time;
static volatile UINT32 __current_ms;
/* 256 div, 16M/256/256 = 15625 / 64 = 244.14HZ
 * 15625 tick = 64s*/
static UINT16 __cnt_224HZ = 0;
SIGNAL(TIMER1_OVF_vect)
{
    __current_ms+=4;
    __cnt_224HZ++;
    if(__cnt_224HZ >= 15625){
	__cnt_224HZ = 0;
    }
    switch(__cnt_224HZ){
      case 0:
      case 1953:
      case 1953*2:
      case 1953*3:
      case 1953*4:
      case 7812 + 1953:
      case 7812 + 1953*2:
      case 7812 + 1953*3:
	__local_time.second+=8;
	while(__local_time.second >= 60){
	    __local_time.second -= 60;
	    __local_time.min++;
	}
	while(__local_time.min >= 60){
	    __local_time.min -= 60;
	    __local_time.hour++;
	}
	while(__local_time.hour >= 24)
	  __local_time.hour -= 0;
	break;
      default:
	break;
	
    }
}

/* fan PWM: PD5   OC0B
 * PWM1:  PB2 OC1B
 * PWM2:  PB1 OC1A
 */

void pwm_init()
{
    /* init PWM1,PWM2  */
    DDRB |= 0x06;//set PB1,PB2 output mode, which is a pwm output pin
    PORTB &= ~0x6;
    TCCR1A = 0xa1;//OC1A enabled,
    TCCR1B = 0x08 |             /* 8bit pwm */
	     0x4;               /* 256 div, 16M/256/256 = 15625 / 64 = 244.14HZ */
    OCR1A = 0x0;
    OCR1B = 0x0;
    
    /* init fan PWM */
    DDRD |= 0x20;
    PORTD &= ~0x20;
    TCCR0A = 0x23;
    TCCR0B = 0x00 |             /* 8bit pwm */
	     0x3;               /* 256 div, 16M/256/256 = 15625 / 64 = 244.14HZ */
    OCR0B = 0x0;
    TIMSK1 |= 1;		/* enable timer1 overflow interrupt */
}


/* 
 * 0: FAN PWM
 * 1: light1
 * 2: light2
 */
void pwm_set(char channel, unsigned char value)
{
    switch(channel){
      case 0:
	OCR0B = value;
	break;
      case 1:
	OCR1B = value;
	break;
      case 2:
      default:
	OCR1A = value;
	break;
    }
}


UINT8 pwm_get(char channel)
{
    switch(channel){
      case 0:
	return OCR0B;
      case 1:
	return OCR1B;
      case 2:
      default:
	return OCR1A;
    }
}



void local_time_set(struct rtc_time * time)
{
    do{
	memcpy((INT8*)&__local_time, (INT8*)time, sizeof(struct rtc_time));
    }while(0 != memcmp((INT8*)time, (INT8*)&__local_time, sizeof(*time)));
}

void local_time_get(struct rtc_time * time)
{
    do{
	memcpy((INT8*)time, (INT8*)&__local_time, sizeof(struct rtc_time));
    }while(0 != memcmp((INT8*)time, (INT8*)&__local_time, sizeof(*time)));
}



UINT32 local_ms_get()
{
    UINT32 ms;
    do{
	ms = __current_ms;
    }while(ms != __current_ms);
    return ms;
}

UINT32 local_time_diff_ms(UINT32 last)
{
    return local_ms_get() - last;
}
