#include <stdio.h>

#include <stdlib.h>
#include <avr/wdt.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <compat/deprecated.h>
#include "uart.h"
#include "eeprom.h"
#include "i2c.h"
#include "rtc.h"
#include "spi.h"
#include "nrf24l01.h"
#include "timer.h"
#include "rf.h"
#include "ktype.h"		/* ktype should be included as last head file */
#include "debug.h"
#include "../rf_mng.h"





struct rf_ctrl{
    /* configuration */
    UINT8 hostId;		/* host id, 0xe0-0xfe */
    UINT8 devId;		/* device id, 0x0-0x3f, 0xff mean broadcast msg */
    UINT8 rfPlane;		/* rf channel jump plane */
    UINT8 sync;			/* sync flag */
    UINT8 rfChan;		/* current channel */
    UINT8 tick;
    UINT8 rx_err;		/* err event cnt */
    UINT8 tx_err;		/* err event cnt */
    UINT16 lastRcv;
    struct rf_cmd rxCmd;	/* buffer for rx cmd */
    UINT8 rxCmdAvail;
    struct rf_cmd txCmd;	/*  buffer for tx cmd */
    struct rf_cmd busyCmdBuf;	/* ping-pong buffer for rx cmd */
    UINT8 seqOut;
    UINT8 seqIn;
    UINT8 mode;			/* 0:listen mode   1:normal mode, IO packet */
    UINT8 rxMode;
};


static volatile struct rf_ctrl rf;


struct rf_cmd;
extern volatile UINT8 tx_deliver;

/* 16M crystal, 976.5626HZ
 * cnt to 15625 = 16s
 */
void main_timer_cb(UINT8 sec)
{
    ds_sample_check();
    if(rf.mode == HOST_MODE_LISTEN){
	static UINT16 __listenCnt = 0;
	__listenCnt++;
	if(__listenCnt > 1000){
	    __listenCnt = 0;
	    rf.rfChan = rf_channel_inc(rf.rfChan, rf.rfPlane);
	    nrf_chan_set(rf.rfChan);
	    if(!rf.rxMode){
		nrf_enter_rx_mode();
		rf.rxMode = 1;
	    }
	}
	/* listen mode dont do channel hop */
	return;
    }else{

	/* normal mode, channel hop & send cmd */
	rf.tick++;
	if(rf.tick >= RF_TICKS_PER_FRAME){
	    DDRC |= 0x2;
	    PORTC |= 0x2;
	    rf.tick = 0;
	    rf.rfChan = rf_channel_inc(rf.rfChan, rf.rfPlane);
	    /* use new channel */
	    nrf_chan_set(rf.rfChan);
	    if(!rf.rxMode){
		nrf_enter_rx_mode();
		rf.rxMode = 1;
	    }
	    PORTC &= 0xfd;
	}
	if(rf.tick == RF_HOST_FRAME_SND_TICK && RF_IS_PROCESSING_CMD()){
	    /* host发送时隙 */
	    /* send command */
	    if((rf.seqOut != rf.seqIn)){
		/* update rfChan */
		rf.txCmd.rfChan = rf.rfChan;
		/* repeat last packet */
		nrf_snd_block((UINT8*)&rf.txCmd, 32);
		rf.rxMode = 0;
		if(!rf.rxMode){
		    nrf_enter_rx_mode();
		    rf.rxMode = 1;
		}
		tx_deliver = 0x80 | rf.rfChan;
	    }else{
	    }
	    /* after sending, nrf24l01 quit rx mode */
	}else{
	    if(!rf.rxMode){
		nrf_enter_rx_mode();
		rf.rxMode = 1;
	    }
	}
    }
}



struct rf_cmd * getRxCmdBuf()
{
    if((rf.rxCmdAvail))
	return NULL;
    else
      return (struct rf_cmd * )&rf.rxCmd;
	
}
UINT8 rxBufferIsFull()
{
    if((rf.rxCmdAvail))
      return 1;
    else
      return 0;
}

static void deliverRxCmd()
{
    rf.rxCmdAvail = 1;
    rf.seqIn = rf.rxCmd.seqNum + 1;
    /* printk(">>>>deliver %d\n", rf.rxCmd.seqNum); */
}

void rf_rcv_isr()
{
    UINT8 len, pipe;
    struct rf_cmd * pCmd = (struct rf_cmd * )&rf.rxCmd;
    PCIFR = 0x1;		/* clear PCINT0 */
    if(rxBufferIsFull()){	/* 接收缓冲是满的，丢弃消息 */
	nrf_flush_rx();
	return;
    }
    
    len = nrf_int_rcv(&pipe, (UINT8*)pCmd, 32);

    if(len != 32)		/* not ktank message */
      return;

    if(rf.mode == HOST_MODE_LISTEN){
	/* dont filter any command */
    }else{
	if(pCmd->destId != rf.hostId || pCmd->seqNum != rf.seqIn){
	    /* printk("!!!rcv seq illegal\n"); */
	    /* printk("src 0x%x dest 0x%x cmd 0x%x seq 0x%x, seqOut %x in %x\n",  */
	    /* 	   pCmd->srcId, pCmd->destId, pCmd->cmd, pCmd->seqNum, rf.seqOut, rf.seqIn); */
	    return;
	}
    }
    deliverRxCmd();
}

/* 查询是否有应答命令 */
UINT8 rcvCmd(struct rf_cmd  * cmd)
{
    if((rf.rxCmdAvail)){
	memcpy((char *)cmd, (char *)&rf.rxCmd, sizeof(struct rf_cmd));
	rf.rxCmdAvail = 0;
	return sizeof(struct rf_cmd);
    }else{
	return 0;
    }
}

void flushSendCmd()
{
    rf.seqIn = rf.seqOut;
}


void rfSendCmd(struct rf_cmd * pCmd)
{
    flushSendCmd();
    _delay_us(5);
    pCmd->srcId = rf.hostId;
    pCmd->rfPlane = rf.rfPlane;
    pCmd->seqNum = rf.seqOut;
    memcpy((INT8*)&rf.txCmd, (INT8*)pCmd, sizeof(struct rf_cmd));
    rf.seqOut++;
}


/* init rf tx/rx, timer, spi, nrf2401 */
INT8 rf_init(struct rtc_time * time)
{
    rf.hostId = 0xc0;		/* 0xc0 should be a illegal id, nobody send cmd to this target */
    rf.rfPlane = 0;
    rf.rfChan = 0;
    rf.mode = HOST_MODE_LISTEN;	/* default in listen mode */
    rf.seqOut = rf.seqIn = 0;
    /* enable 976HZ timer */
    timer_init();
    spi_init();
    /* enable nrf24l01 */
    nrf_init();
    nrf_chan_set(rf.rfChan);
    if(!rf.rxMode){
	nrf_enter_rx_mode();
	rf.rxMode = 1;
    }
    timer_cb_set(main_timer_cb);
    return 0;
}

void rf_config(UINT8 hostId, UINT8 rfPlane)
{
    rf.hostId = hostId;
    rf.rfPlane = rfPlane;
}

void rf_mode_set(UINT8 mode)
{
    rf.mode = mode;
}

UINT8 rf_mode_get()
{
    return rf.mode;
}

