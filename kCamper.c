/*
 *
 *    ��ˮ��<----------<---switch_tank2<--pumb_tank2<--������A<--temp_tank2<--��ˮ�����
 *                    ^
 *                    |
 *                    |                            
 *                  switch_tank1_loop
 *                    ^
 *                    |
 *                    |
 *    ��ˮ---------------switchB---------------------------->��ˮ���ˮ
 *      ^
 *      +---------------------------------------------
 *                                                   ^
 *    ��ˮ��ѹ---------temp_cold--->pump_Cold---flow_cold---->������
 *              |                          |
 *              |                          |
 *              +-----ԭ��ˮ��-------------->
 *
 *
 *    ����Һ----------temp_hot
 *
 * - ���ܣ�
 *  - ��ˮ�䱣��
 *  - ��ˮ�䱣��
 *  - ��ˮ����ȵ���ˮ�¶�
 *  - ��ˮ�����ʱ���븱ˮ��ѭ�����ϵ��¶�
 *  - �쳣ʱ����������
 *   - ��ˮ����ˮ  ����
 *   - ��ˮ����ˮ  ����
 *   - ����Һ����  ����

 * use internal OSC:
 * PB2/3/4/5 reserved for SPI
 * PB6/7 reserved for external OSC
 * PD0/1 reserved for UART1
 * PC6 reserved for reset
 * 
 * 
 * pinmux desc:
 *              DATA  GND
 *   temp_cold: PC0  PC1
 *   temp_hot:  PC2  PC3
 *   temp_tank2:PC4  PC5
 * 
 *   switch_tank2&pumb_tank2:  PD2
 *   switchB:PD3
 *   switch_tank1_loop:PD4
 *   flow_cold:PD5
 *   flow_bakTank:PD6
 */



#include "lib/uart.h"
#include "lib/ds18b20.h"
#include "lib/kconfig.h"
#include <util/delay.h>
#include <avr/interrupt.h>
int main()
{
    int cnt = 0;
    short temp;
    int i;
        /* init uart */
    init_uart(4800);
        /* init temperature */
    ds_init();
    while(1){
        printf("--kCamper %d\n", cnt++);
        temp = ds_get_temperature_x16(0);
        if(temp == -1000)
            printf("read failed\n");
        else
            printf("temperature %d\n", temp/16);
        for(i=0; i<10000; i++)
            _delay_us(100);
    }
    
    
}
