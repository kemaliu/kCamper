#ifndef __UI_H__
#define __UI_H__

#define AREA_0_START "SXY(0,0);"

#define AREA_1_START "SXY(0,90);"

#define AREA_2_START "SXY(0,210);"

#define BUTTON2_CONTENT "自动保温"
#define BUTTON3_CONTENT "水箱加热"
#define BUTTON4_CONTENT "主箱加水"
#define BUTTON5_CONTENT "副箱加水"

void setup_button_enable(char enable);
void draw_main_page();
void pump_status_show(char status);

#endif  /* #ifndef __UI_H__ */
