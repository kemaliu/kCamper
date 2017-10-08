#ifndef __KEMLAIU_RF__
#define __KEMLAIU_RF__
#include "rtc.h"
#include "ktype.h"


#define RF_TICKS_PER_FRAME 2
#define RF_HOST_FRAME_SND_TICK 0
#define RF_SLAVE_FRAME_SND_TICK 1




#define RF_PLANE_NUM 16




typedef enum{
  /* RF command */
  KFISH_CMD_TIME_NOTIFY = 0,	/* master notify current time
				   {1B hour 1Bmin, 4Byte dev status}*/
  KFISH_CMD_DISCOVER = 1,	/* 4Byte dev status */
  KFISH_CMD_DISCOVER_ACK,	/* 1B devId, 1B ctrlNum */
  KFISH_CMD_DEVCONFIRM,		/* 4Byte dev status */
  KFISH_CMD_DEVCONFIRM_ACK,	/* 4Byte dev status, 1B controlNum */

  KFISH_CMD_HEART_BEAT = 0x10,
  KFISH_CMD_HEART_BEAT_ACK,
  /* KFISH_CMD_SET_DEVICES_INFO = 0x60, /\* 设置设备 *\/ */
  KFISH_CMD_SET_DEVICE_NAME = 0x61,  /* 设置设备名 */
  /* KFISH_CMD_SET_CTRL_INFO = 0x62,    /\* 设置控制器 *\/ */
  KFISH_CMD_SET_CTRL_NAME = 0x63,    /* 设置控制器名 */
  KFISH_CMD_SET_CTRL_CFG = 0x64,     /* 设置控制器配置值 */
  KFISH_CMD_PAUSE_DEVICE = 0x65,     /* 暂停设备 */
  KFISH_CMD_SET_TIME = 0x66,	     /* 设置时间 */

  
  KFISH_CMD_GET_DEVICES_INFO = 0x80, /* 获取设备列表 */
  KFISH_CMD_GET_DEVICES_CTRL_NUM = KFISH_CMD_GET_DEVICES_INFO, /*  */
  KFISH_CMD_GET_DEVICE_NAME = 0x81,
  KFISH_CMD_GET_CTRL_STATUS = 0x82, /* 获取控制器状态 */
  KFISH_CMD_GET_CTRL_NAME = 0x83,
  KFISH_CMD_GET_CTRL_CFG = 0x84,
  KFISH_CMD_GET_DEVICE_TIME = 0x85,/* 获取设备时间 */
}KFISH_CMD_T;

struct rf_cmd{
    UINT8 srcId;
    UINT8 destId;
    UINT8 rfPlane;
    UINT8 rfChan;		/* frameID */
    UINT8 cmd;			/* KFISH_CMD_T */
    UINT8 devId;		/* device */
    UINT8 ctrlId;		/* ctrl ID */
    UINT8 seqNum;		/* sequence number */
    UINT8 data[24];		/* cmd data */
};

UINT8 rf_mode_get();
void rf_mode_set(UINT8 mode);




UINT8 rcvCmd(struct rf_cmd  * cmd);

UINT8 rf_channel_inc(UINT8 frame, UINT8 plane);

void nrf_flush_rx();
void nrf_flush_tx();
#ifdef KTANK_HOST
INT8 rf_init(struct rtc_time * time);
#define HOST_MODE_LISTEN 0
#define HOST_MODE_DISCOVER 1
#define HOST_MODE_NORMAL 2

void rfSendCmd(struct rf_cmd * pCmd);
void rf_config(UINT8 hostId, UINT8 rfPlane);
void flushSendCmd();

/* UINT8 rf_alloc_host_id_begin(); */

/* INT8 rf_alloc_host_id_done(UINT8 * hostId, UINT8 * rfPlane); */
#else
INT8 rf_init(struct rtc_time * time, UINT8 mode);
#define SLAVE_MODE_NORMAL 0xf	/* 正常状态 */

#define SLAVE_MODE_WAIT_DISCOVER 1 /* 等待discover */
#define SLAVE_MODE_DISCOVER_GET 2 /* 收到discover */


void rfSlaveDoSync();
INT8 rfSlaveSyncOk(UINT8 * hostId, UINT8 * devId);
#endif

#endif
