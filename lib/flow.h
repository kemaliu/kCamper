#ifndef __FLOW_CTRL_H__
#define __FLOW_CTRL_H__

typedef enum{
    FLOW_TANK1_OUT = 0,
    FLOW_TANK2_OUT = 1,
    FLOW_NUM
}FLOW_CTRL_T;

UINT32 flow_num(FLOW_CTRL_T index);
void flow_init();

void flow_reset(FLOW_CTRL_T index);

UINT32 flow_cnt(FLOW_CTRL_T index);

UINT32 flow_cnt_speed(FLOW_CTRL_T index);
#endif  /* #ifndef __FLOW_CTRL_H__ */
