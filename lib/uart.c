#include <avr/io.h>
#include <util/delay.h>
#include <stdarg.h>

#include <stdio.h>

#include <avr/interrupt.h>
#include "uart.h"
#include "debug.h"
#include "pwm.h"
//常量定义
#include "ktype.h"		/* ktype should be included as last head file */
#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)

#ifndef UBRRL
#define UBRRL UBRR0L
#endif

#ifndef UBRRH
#define UBRRH UBRR0H
#endif

#ifndef UDR
#define UDR UDR0
#endif
#ifndef UCSRA
#define UCSRA UCSR0A
#endif


#ifndef RXC
#define RXC RXC0
#endif

#ifndef UDRE
#define UDRE UDRE0
#endif

#ifndef UCSRA
#define UCSRA UCSR0A
#endif


#ifndef UCSRB
#define UCSRB UCSR0B
#endif

#ifndef UCSRC
#define UCSRC UCSR0C
#endif


#ifndef RXEN
#define RXEN RXEN0
#endif


#ifndef TXEN
#define TXEN TXEN0
#endif

#ifndef RXCIE
#define RXCIE RXCIE0
#endif

#endif	/* if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__) */

void uartRcv(uint8 c);





/* uart interrupt */
#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
SIGNAL(USART_RX_vect)
#else
SIGNAL(SIG_UART_RECV)
#endif
{
#if 1
    uartRcv(UDR);
#else
    put(UDR);
#endif
}



//向串口写数

void put(unsigned char c)
{

    while( !(UCSRA & (1<<UDRE)));

    UDR=c;

}







void put_c(char c)
{
    if (c == 0x0d || c == '\n'){
        put(0xd);
        put(0xa);
    }else{
        put(c);
    }

}



//从串口读数

unsigned char get_c( void )
{
    //等待接受标志
    while ( !(UCSRA & (1<<RXC)) )
      ;
    //读接收数据
    return UDR;

}


INT8 uart_poll_c(UINT8 * pC)
{
    if ( !(UCSRA & (1<<RXC)) )
      return 0;
    *pC = UDR;
    return 1;
}



unsigned char get_hex()
{
    unsigned char value = 0;
    while(1){
        unsigned char v;
        v = get_c();
        if (v >= '0' && v <= '9'){
            put_c(v);
            v -= 0x30;
            value = (value << 4) | v;
        }else if (v >= 'a' && v <= 'f'){
            put_c(v);
            v = v - 'a' + 10;
            value = (value << 4) | v;
        }else if (v == 0xd){
            put_c(v);
            return value;
        }
    }
}



//向串口写字符串

void put_s(char * ptr)

{
    while ((*ptr))
    {
        put_c(*ptr++);
    }
}





//USART 初始化

void init_uart(uint16 baud)
{
    unsigned short div;
#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
    UCSRC = /* (0<<UMSEL01) | (1<<UMSEL00) |  */0x06;
#else
    //USART 9600 8, n,1  PC上位机软件(超级终端等)也要设成同样的设置才能通讯
    UCSRC = (1<<URSEL) | 0x06;
#endif
   
    //异步，8位数据，无奇偶校验，一个停止位，无倍速  
    //U2X=0时的公式计算
    div = F_CPU/baud/16-1;
    UBRRL= div%256;
    UBRRH= div/256;
    UCSRA = 0x00;
    //使能接收中断，使能接收，使能发送
    UCSRB = (1<<RXEN)|		/* enable rx */
	    (1<<TXEN)|		/* enable tx */
	    (1<<RXCIE);		/* enable rx interrupt */
    sei();
}



static char printbuffer[64];
int uart_print( const char * fmt, ... )
{
    va_list args;
    int n;
    va_start ( args, fmt );
    n = vsprintf ( printbuffer, fmt, args );
    va_end ( args );
    put_s(printbuffer);
    return n;
}



