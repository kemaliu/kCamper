#ifndef __UI_H__
#define __UI_H__

#define AREA_0_START "SXY(0,0);"

#define AREA_1_START "SXY(0,90);"

#define AREA_2_START "SXY(0,210);"

#define BUTTON2_CONTENT "�Զ�����"
#define BUTTON3_CONTENT "ˮ�����"
#define BUTTON4_CONTENT "�����ˮ"
#define BUTTON5_CONTENT "�����ˮ"

void setup_button_enable(char enable);
void draw_main_page();
void pump_status_show(char status);

#endif  /* #ifndef __UI_H__ */
