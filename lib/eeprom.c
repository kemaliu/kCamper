#include <avr/io.h>
#include "ktype.h"

UINT8 EEPROM_get(UINT16 uiAddress);
UINT8 EEPROM_put(UINT16 uiAddress, UINT8 ucData)
{
    if(EEPROM_get(uiAddress) == ucData) /* data not changed */
      return 0;
    
    /* 设置地址和数据寄存器*/
    EEAR = uiAddress;
    EEDR = ucData;
    /* 置位EEMPE */
    EECR = (1<<EEMPE);		/* 主机写使能 */
    /* 置位EEPE 以启动写操作*/
    EECR |= (1<<EEPE);		/* 主机使能 */
    /* 等待上一次写操作结束 */
    while(EECR & (1<<EEPE))
      ;
    return 1;
}


UINT8 EEPROM_get(UINT16 uiAddress)
{
    /* 等待上一次写操作结束 */
    while(EECR & (1<<EEPE))
      ;
    
    /* 设置地址寄存器*/
    EEAR = uiAddress;
    /* 设置EERE 以启动读操作*/
    EECR |= (1<<EERE);
    /* 自数据寄存器返回数据 */
    return EEDR;
}


void eeprom_read(UINT16 addr, UINT8 * buf, UINT8 size)
{
    int i;
    for(i=0; i<size; i++){
	buf[i] = EEPROM_get(addr+i);
    }
}


UINT8 eeprom_write(UINT16 addr, UINT8 * buf, UINT8 size)
{
    int i;
    UINT8 cnt = 0;
    for(i=0; i<size; i++){
	cnt += EEPROM_put(addr+i, buf[i]);
    }
    return cnt;
}
