#ifndef __NRF24L01_H__
#define __NRF24L01_H__

#include <ktype.h>



void nrf_init();
/* 设置当前使用的通道, 0-124 */
void nrf_chan_set(UINT8 chan);


/* 发送数据，不等待结束 */
void nrf_snd(UINT8 *buf, UINT8 size);


/* 接收数据， 
 * return 数据长度 
 */
UINT8 nrf_rcv(UINT8 pipe, UINT8 * buf, UINT8 size);


/* 进入rx模式 */
void nrf_enter_rx_mode();

/* 中断中接收数据的函数，每次固定接收32B */
UINT8 nrf_int_rcv(UINT8 * pipe, UINT8 *buf, UINT8 size);


/* 进入powerdown 模式 */
void nrf_powerdown();

void nrf_snd(UINT8 *buf, UINT8 size);


void nrf_snd_block(UINT8 *buf, UINT8 size);

void nrf_write_tx_buffer(UINT8 *buf, UINT8 size);

#define TX_UNBLOCK 0
#define TX_BLOCK 1

void nrf_tx_start(UINT8 block);
#endif
