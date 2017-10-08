/* SPI性能记录
 * 读写1Byte 7us
 * 读写32byte 66us
 * unblock 发送32B, 1M/2Mbps速度  112us
 * block 发送32B, 1M 556us 2Mbps 396us
 * 如果先写入数据，再nrf_tx_start(TX_BLOCK) 2Mbps下320us
 * 可以认为发送结束时，接收已经完成
 * !!!注意:NRF在中断和外部循环都有对芯片的SPI操作，外部访问需要关中断保护
 */


#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "ktype.h"
#include "uart.h"
#include  "spi.h"
#include "nrf24l01.h"
#include "timer.h"
#include "debug.h"
/* interrupt use PCINT0(PB0) */


#define RF_SPEED RF_2M

enum{
    RF_18dbm = 0,                   /* -18dbm */
    RF_12dbm = 2,                   /* -12dbm */
    RF_6dbm  = 4,                   /* -6dbm */
    RF_0dbm  = 6,                    /* 0dbm */
    RF_1M    = 0x0,
    RF_2M    = 0x8,
    RF_250K  = 0x20,
    RF_PLLLOCK= 0x10,
    RF_CONT_WAVE= 0x80,
};


/* CONFIG register */
static UINT8  config_regVal = 0;
#define REG_CONFIG 0

#define _CONFIG_RX_MODE 1
#define _CONFIG_PWR_UP 2
#define _CONFIG_CRC_1B 0
#define _CONFIG_CRC_2B 0x4
#define _CONFIG_CRC_ENABLE 0x8
#define _CONFIG_MASK_INT_RETRY_MAX 0x10
#define _CONFIG_MASK_INT_TX_DONE 0x20
#define _CONFIG_MASK_INT_RX_AVAIL 0x40

#define REG_EN_AA 1
#define REG_EN_RXADDR 2
#define REG_SETUP_AW 3
#define REG_SETUP_RETR 4
#define REG_RF_CH 5
#define REG_RF_SETUP 6
/* 状态寄存器 */
#define REG_STATUS 7
#define _STATUS_RX_AVAIL 0x40
#define _STATUS_TX_DONE 0x20
#define _STATUS_MAX_RETRY 0x10




#define REG_OBSERVE_TX 8
#define REG_RPD 9
#define REG_RX_ADDR_P0 0xa
#define REG_RX_ADDR_P1 0xb
#define REG_RX_ADDR_P2 0xc
#define REG_RX_ADDR_P3 0xd
#define REG_RX_ADDR_P4 0xe
#define REG_RX_ADDR_P5 0xf
#define REG_TX_ADDR 0x10
#define REG_RX_PW_P0 0x11
#define REG_RX_PW_P1 0x12
#define REG_RX_PW_P2 0x13
#define REG_RX_PW_P3 0x14
#define REG_RX_PW_P4 0x15
#define REG_RX_PW_P5 0x16
/* FIFO STATUS */
#define REG_FIFO_STATUS 0x17

#define _TX_FIFO_REUSE 0x40
#define _TX_FIFO_FULL 0x20
#define _TX_FIFO_EMPTY 0x10
#define _RX_FIFO_FULL 0x2
#define _RX_FIFO_EMPTY 0x1

#define REG_DYNPD 0x1c
#define REG_FEATURE 0x1d
#define REG_FLUSH_TX 0xe1
#define REG_FLUSH_RX 0xe2
#define REG_REUSE_TX_PL 0xe3
#define REG_TX_FIFO 0xa0
#define REG_RX_FIFO_LEN 0x60
#define REG_RX_FIFO 0x61
#define REG_W_ACK_PAYLOAD_P0 0xa8
#define REG_W_ACK_PAYLOAD_P1 0xa9
#define REG_W_ACK_PAYLOAD_P2 0xaa
#define REG_W_ACK_PAYLOAD_P3 0xab
#define REG_W_ACK_PAYLOAD_P4 0xac
#define REG_W_ACK_PAYLOAD_P5 0xad
#define REG_W_TX_PAYLOAD_NO_ACK 0xb0
#define REG_NOP 0xff


#define NRF_CE_HIGH() PORTC |= 1
#define NRF_CE_LOW() PORTC &= 0xfe

/* PIPE0使用的物理地址 */
UINT8 addr_pipe0[5] = "kemah";
/* PIPE1使用的物理地址 */
UINT8 addr_pipe1[5] = "kemas";


UINT8 nrf_int_rcv(UINT8 * pipe, UINT8 *buf, UINT8 size);


SIGNAL(PCINT0_vect)
{
    void rf_rcv_isr();
    if((PINB & 1))		/* high, ignore */
      return;
    rf_rcv_isr();
}






static void nrf_reg_byte_write(UINT8 regAddr, UINT8 value)
{
    spi_reg_write(regAddr, &value, 1);
}


UINT8 nrf_reg_byte_read(UINT8 regAddr)
{
    UINT8 value;
    spi_reg_read(regAddr, &value, 1);
    return value;
}

void nrf_enter_rx_mode()
{
    /* before change work mode, need go back to standby mode */
    NRF_CE_LOW();
    /* powerup chip, rx mode, enable 1byte CRC */
    config_regVal |= _CONFIG_RX_MODE;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);
    /* printk("cfg %x\n", nrf_reg_byte_read(REG_CONFIG)); */
    NRF_CE_HIGH();
}


void nrf_flush_rx()
{
    if(!(nrf_reg_byte_read(REG_FIFO_STATUS) & _RX_FIFO_EMPTY)){
	nrf_reg_byte_write(REG_FLUSH_RX, 0xff);
    }
    nrf_reg_byte_write(REG_STATUS, _STATUS_RX_AVAIL);
}

void nrf_flush_tx()
{
    if(!(nrf_reg_byte_read(REG_FIFO_STATUS) & _TX_FIFO_EMPTY)){
	nrf_reg_byte_write(REG_FLUSH_TX, 0xff);
	nrf_reg_byte_write(REG_STATUS, _STATUS_TX_DONE | _STATUS_MAX_RETRY);
    }
}

void nrf_flush()
{
    nrf_flush_rx();
    nrf_flush_tx();
}


void nrf_powerdown()
{
    config_regVal &= ~_CONFIG_PWR_UP;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);
}

void nrf_powerup()
{
    config_regVal |= _CONFIG_PWR_UP;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);
}

/**
 *初始化NRF芯片以及SPI管脚
 */
void nrf_init()
{
    UINT8 vp[5];
    
    spi_init();
    DDRC |= 0x1;		/* CE(PC0) output */
    
    _delay_ms(1);
    /* set PIN CE to low level */
    NRF_CE_LOW();
    _delay_ms(1);
    /* flush tx/rx fifo */
    nrf_flush();

    /* power down */
    config_regVal = 0;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);

    _delay_ms(1);
    /* power up */
    config_regVal = _CONFIG_MASK_INT_RETRY_MAX | 
		    _CONFIG_MASK_INT_TX_DONE|
		    _CONFIG_CRC_ENABLE |
		    _CONFIG_CRC_2B |
		    _CONFIG_PWR_UP;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);
    _delay_ms(2);
    /* disable all pipes */
    nrf_reg_byte_write(REG_EN_RXADDR, 0);
    /* disable auto retransmit */
    nrf_reg_byte_write(REG_SETUP_RETR, 0);
    
    /* disable auto ACK for all pipes*/
    nrf_reg_byte_write(REG_EN_AA, 0);

    /* set address width to 5bytes */
    nrf_reg_byte_write(REG_SETUP_AW, 3);

    /* set up speed & power */
    nrf_reg_byte_write(REG_RF_SETUP, RF_18dbm | RF_SPEED);
    
    /* set pipe0 address to 0a-0a-0a-0a-0a */
    spi_reg_write(REG_RX_ADDR_P0, addr_pipe0, 5);

    /* set pipe1 address */
    spi_reg_write(REG_RX_ADDR_P1, addr_pipe1, 5);


    spi_reg_read(REG_RX_ADDR_P0, vp, 5);
    DBG_PRINT("pipe0 %.2x-%.2x-%.2x-%.2x-%.2x\n", 
	   vp[0], vp[1], vp[2], vp[3], vp[4]);
    spi_reg_read(REG_RX_ADDR_P1, vp, 5);
    DBG_PRINT("pipe1 %.2x-%.2x-%.2x-%.2x-%.2x\n", 
	   vp[0], vp[1], vp[2], vp[3], vp[4]);

    spi_reg_write(REG_TX_ADDR, addr_pipe1, 5);
    spi_reg_read(REG_TX_ADDR, vp, 5);
    DBG_PRINT("tx addr %.2x-%.2x-%.2x-%.2x-%.2x\n", 
	   vp[0], vp[1], vp[2], vp[3], vp[4]);
    
    /* set rx data size to 32 bytes */
    nrf_reg_byte_write(REG_RX_PW_P0, 32);
    nrf_reg_byte_write(REG_RX_PW_P1, 32);
    nrf_reg_byte_write(REG_RX_PW_P2, 32);
    nrf_reg_byte_write(REG_RX_PW_P3, 32);
    nrf_reg_byte_write(REG_RX_PW_P4, 32);
    nrf_reg_byte_write(REG_RX_PW_P5, 32);
    
    /* enable pipe0&pipe1 */
    nrf_reg_byte_write(REG_EN_RXADDR, 3);

    /* /\* powerup chip, rx mode, enable 1byte CRC *\/ */
    nrf_enter_rx_mode();
    /* enable PCINT0 */
#if 1
    DDRB &= 0xfe;		/* set PB0(PCINT0) to input */
    PORTB |= 1;
    PCMSK0 |= 1;
    PCIFR |= 1;			/* clear PCINT0 flag*/
    PCICR |= 1;			/* enable PCINT0 */
#endif
}

/* 进入tx模式 */
static void nrf_enter_tx_mode()
{
    /* before change work mode, need go back to standby mode */
    NRF_CE_LOW();
    /* powerup chip, rx mode, enable 1byte CRC */
    config_regVal &= ~_CONFIG_RX_MODE;
    nrf_reg_byte_write(REG_CONFIG, config_regVal);
}
/* 开始发送数据 */

void nrf_tx_start(UINT8 block)
{
    nrf_enter_tx_mode();
    NRF_CE_HIGH();		/* 首先开始发送 */
#if 0
    /* 使用FIFO status,tx 中断也没用，不需要情中断状态位 */
    if(block){
	nrf_reg_byte_write(REG_STATUS, _STATUS_TX_DONE);
    }
#endif
    /* tx事，CE保持10us就可以了 */
    _delay_us(10);
    NRF_CE_LOW();
    /* 等待发送结束 */
    if(block){
	UINT8 c, cnt = 0;
	do{
	    c = nrf_reg_byte_read(REG_FIFO_STATUS);
	    cnt++;
	}while((!(c & _TX_FIFO_EMPTY)) && cnt < 255);
	if(cnt == 255){
	    printk("tx timeout, fifo status 0x%x 0x%x, status 0x%x\n",
		   c, nrf_reg_byte_read(REG_FIFO_STATUS), nrf_reg_byte_read(REG_STATUS));
	    /* do flush */
	    nrf_reg_byte_write(REG_FLUSH_TX, 0xff);
	}
    }	_delay_us(10);
}


void nrf_chan_set(UINT8 chan)
{
    NRF_CE_LOW();
    nrf_reg_byte_write(REG_RF_CH, chan);
    NRF_CE_HIGH();
}


void nrf_write_tx_buffer(UINT8 *buf, UINT8 size)
{
    UINT8 c;
    UINT32 now;
    now = timebase_get();
    /* send 600us time out */
    do{
    	c = nrf_reg_byte_read(REG_FIFO_STATUS);
    }while((0 == (c & _TX_FIFO_EMPTY))
    	   && (time_diff_us(now) < 600));
    if(!(c & _TX_FIFO_EMPTY)){
	nrf_flush_tx();
    	/* printk("force do tx flush, %dus\n", (UINT16)time_diff_us(now)); */
	printk("f");
    }
    /* 写入数据 */
    spi_reg_write(REG_TX_FIFO, buf, size);
}

/* 发送数据 */
void nrf_snd(UINT8 *buf, UINT8 size)
{
    /* 写入数据 */
    nrf_write_tx_buffer(buf, size);
    /* 开始发送 */
    nrf_tx_start(TX_UNBLOCK);
}


void nrf_snd_block(UINT8 *buf, UINT8 size)
{
    /* 写入数据 */
    nrf_write_tx_buffer(buf, size);
    /* 开始发送 */
    nrf_tx_start(TX_BLOCK);
}



UINT8 nrf_rcv(UINT8 pipe, UINT8 *buf, UINT8 size)
{
    UINT8 c;
    /* printk("reg value 0x%x\n", nrf_reg_byte_read(0)); */
    c = nrf_reg_byte_read(REG_STATUS);
    /* 无数据 */
    if(!(c & 0x40)){
	return 0;
    }
    c = nrf_reg_byte_read(REG_STATUS);
    c = (c >> 1) & 0x7;
    if(c > 5){
    	return 0;
    }
    if(c != pipe){
      return 0;
    }
    spi_reg_read(REG_RX_FIFO_LEN, &c, 1);
    if(c == 0){
      return 0;
    }
    if(c > size){
	printk("buf not enough %d:%d\n", c, size);
	c  = size;
    }
    spi_reg_read(REG_RX_FIFO, buf, c);
    nrf_reg_byte_write(REG_STATUS, 0x40);
    return c;
}

UINT8 nrf_get_status()
{
    return nrf_reg_byte_read(REG_STATUS);
}

UINT8 nrf_int_rcv(UINT8 * pipe, UINT8 *buf, UINT8 size)
{
    UINT8 c;
    
    c = nrf_reg_byte_read(REG_STATUS);
    spi_reg_read(REG_RX_FIFO, buf, 32);
    /* clear pending */
    nrf_reg_byte_write(REG_STATUS, _STATUS_RX_AVAIL);
    c = (c >> 1) & 0x7;
    if(c > 5){
    	return 0;
    }
    *pipe = c;
    nrf_flush_rx();
    return 32;
}






/* 测试代码 */
#if 0
#include "nrf24l01.h"

void rf_tx_timecost_test()
{
    UINT8 val, i, buf[128];
    UINT32 now, cnt = 0;
    now = timebase_get();
    val = nrf_reg_byte_read(0);
    now = time_diff_us(now);
    printk("read return 0x%x, cost %dus\n", val, now);
    now = timebase_get();
    for(i=0; i<100; i++)
      val = nrf_reg_byte_read(0);
    now = time_diff_us(now);
    printk("read 1byte 100times return 0x%x, cost %dus\n", val, now);
    
    now = timebase_get();
    for(i=0; i<10; i++)
      spi_reg_read(0, buf, 32);
    now = time_diff_us(now);
    printk("spi read 32yte 10times return 0x%x, cost %dus\n", val, now);
    
    now = timebase_get();
    for(i=0; i<100; i++)
      spi_reg_write(0xa0, buf, 32);
    now = time_diff_us(now);
    printk("write 32B  100times cost %dus\n", now);
    
    nrf_snd(buf, 32);
    _delay_ms(1000);
    now = timebase_get();
    nrf_snd(buf, 32);
    now = time_diff_us(now);
    printk("send unblock cost %dus\n", now);
    _delay_ms(1000);
    now = timebase_get();
    nrf_snd_block(buf, 32);
    now = time_diff_us(now);
    printk("send block cost %dus\n", now);
}
void rf_tx_test()
{
    UINT8 val, i, buf[128];
    UINT32 now, cnt = 0;
    wdt_enable(WDTO_1S);
    init_uart(57600);
    DDRD |= 8;
    PORTD |= 8;			/* high */
    printk("host..............\n");
    printk("MCUSR 0x%x WDTCSR 0x%x\n", MCUSR, WDTCSR);
    nrf_init();
    timer_init();
    
    /* rf_timecost_test(); */
    while(1){
	wdt_reset();

	
	nrf_enter_rx_mode();
	nrf_write_tx_buffer(buf, 32);
	
	now = timebase_get();	
	PORTD &= ~8;			/* high */	
	nrf_tx_start(TX_BLOCK);
	PORTD |= 8;			/* high */
	now = time_diff_us(now);
	cnt++;
	_delay_us(1000);
	if(cnt % 100 == 0)
	  printk("%lusend block cost %luus\n", cnt++, now);
    }
    
}


/* rx侧终端服务程序 */
void rf_rcv_isr()
{
    extern UINT32 _rcv_cnt;
    UINT8 len, pipe;
    struct rf_cmd cmd;
    PORTD &=~8;
    len = nrf_int_rcv(&pipe, (UINT8*)&cmd, 32);
    if(len != 32){
	printk("len %d illegal\n", len);
    }
    PORTD |= 8;
    _rcv_cnt++;
}

volatile UINT32 _rcv_cnt=0;
void rf_rx_test()
{
    UINT8 i;
    UINT8 nrf_reg_byte_read(UINT8 regAddr);
    UINT32 last = 0;
    /* PC3 output */
    DDRD |= 8;
    PORTD |= 8;			/* high */
    init_uart(57600);
    printk("\n\n\n!!!!!!!!!!ktank start!!!!!!!!!!!!!!\n\n\n");
        /* init timer */
    timer_init();
    nrf_init();
    nrf_enter_rx_mode();
    for(i=0; i<32; i++){
	printk("0x%x 0x%x\n", i, nrf_reg_byte_read(i));
    }
    sei();
    nrf_flush_rx();
    while(1){
#if 1
	if(_rcv_cnt % 100 == 0)
	{
	    last = _rcv_cnt;
	    printk("%lu\n", _rcv_cnt);
	}
#else
	for(i=0; i<32; i++){
	printk("0x%x 0x%x\n", i, nrf_reg_byte_read(i));
    }
#endif
	
	/* printk("chip status 0x%x fifo status 0x%x\n", nrf_reg_byte_read(7), nrf_reg_byte_read(0x17)); */
	/* printk("config 0x%x\n", nrf_reg_byte_read(0)); */
    }
    
}
#endif
