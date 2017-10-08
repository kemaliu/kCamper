#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "ktype.h"
#include "uart.h"
#include <avr/interrupt.h>
#include "spi.h"
/* SIP module */
/* connection with nRF24L01
   device 0
   M8                                   nRF24L01
   15   PC0(ADC0)    ------------------------------->3   CE
   16   PD7          ------------------------------->4   CSN
   17   PB3 (MOSI/OC2)------------------------------>6   MOSI
   18   PB4 (MISO)   <-------------------------------7   MISO
   19   PB5 (SCK)    ------------------------------->5   SCK
   20   PB0(PCINT0)  <-------------------------------8   INT
 */

#define SPI_CS_HIGH() PORTD |= 0x80
#define SPI_CS_LOW() PORTD &= 0x7f





void spi_init()
{
    /* att: SS must be configure as output when SPI master enabled
     * otherwise SPI will enter slave mode automaticly
     */
    DDRB |= 0x2c;                /* PB2(SS), 3, 5 output */
    DDRB &= ~0x11;                /* INT(PB0), MISO(PB4) input */
    
    DDRD |= 0x80;		/* CSN(PD7) output */


    SPI_CS_HIGH();
#if 1
    SPCR = 0x50;
#else
    SPCR = (0 << SPIE) |		/* spi interrupt enable */
	   (1 << SPE) |			/* spi enable */
	   (0 << DORD) |		/* DORD 1:lst first 0:msb first, nrf2401 should be msb first */
	   (1 << MSTR) |		/* master mode */
	   0;				/* 16M/4xdouble = 8Mbps */
#endif
    SPSR = 0x01;			/* double spi speed */
    sei();
}

UINT8 spi_rw(UINT8 data)
{
    UINT8 val;
    SPDR = data;		/* 写出数据 */
    while(0 == (SPSR & 0x80));  /* wait the tx/rx finish */
    val = SPDR;
    return val;
}



void spi_reg_write(UINT8 reg, UINT8 * pData, UINT8 len)
{
    UINT8 i;
    SPI_CS_LOW();
    spi_rw(reg | 0x20);
    for(i=0; i<len; i++){
	spi_rw(pData[(int)i]);
    }
    SPI_CS_HIGH();
}



UINT8 spi_reg_read(UINT8 reg, UINT8 * pData, UINT8 len)
{
    INT8 i;
    SPI_CS_LOW();
    spi_rw(reg);
    while(len-- > 0){
	*pData++ = spi_rw(0xff);
    }
    SPI_CS_HIGH();
    return len;
}

