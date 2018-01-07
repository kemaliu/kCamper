#ifndef __FLOW_CTRL_H__
#define __FLOW_CTRL_H__

typedef enum{
    FLOW_INPUT = 0,
    FLOW_TANK1_OUT = 1,
    FLOW_TANK2_OUT = 2,

    FLOW_NUM
}FLOW_CTRL_T;

void flow_init();

void flow_reset(UINT8 index);

UINT32 flow_cnt(UINT8 index);

UINT32 flow_cnt_speed(UINT8 index);
#endif  /* #ifndef __FLOW_CTRL_H__ */
