#include <avr/io.h>

#include <stdarg.h>

#include <stdio.h>

#include <avr/interrupt.h>
#include <avr/signal.h>
//发送函数
void putchar0(unsigned char c) 
{ 

    while (!(UCSR0A&(1<<UDRE0)));//表明发送器已经准备就绪 
    UDR0=c; //将要发送的数据装入UDR0寄存器
} 
//初始化函数
//函数功能:uart0初始化程序 入口参数: 出口参数: *******************/ 
void uart0_init(void) 
{
    unsigned short div;
    unsigned short baud = 9600;
    div = F_CPU/baud/16-1;
    UCSR0B = 0x00;   //关闭UART00 
    UCSR0A = 0X00;  //不使用倍速发送（异步） 
    UCSR0C = 0X06;  //(1<<UCSZ01)|(1<<UCSZ00) //数据位为8位  
#if 0
    UBRR0L = 0x51;   //(fosc/16/baud-1)%256; //异步正常情况下的计算公式 U2XN=1则是8等于0则是16
    UBRR0H = 0x00;  //(fosc/16/baud-1))/256; 
#else
    UBRR0L= div%256;
    UBRR0H= div/256;
#endif
    UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);   //接收使能和发送使能 若中断使能则需要置位
}
//主函数不停发送0x71
void main()
{
    uart0_init();
    while(1) 
    {
	putchar0('a');

    }
}
