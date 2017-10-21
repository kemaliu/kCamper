#include <util/delay.h>
#include "lib/kconfig.h"
#include "ui.h"
#define NO_OFFSET


void pump_mode_show(char mode)
{
#ifdef NO_OFFSET
    screen_const_puts("LABL(16,175,99,239,'泵:");
    if(mode == 0)
        screen_const_puts("全速");
    else
        screen_const_puts("低速");
    screen_const_puts("',15,0);\n");
#else
    
#endif
}


void draw_main_page()
{
    screen_const_puts("DR3;");
    screen_const_puts("TPN(2);");
    screen_const_puts("CLS(0);");
    screen_const_puts("TABL(0,0,79,30,3,3,19);");
    screen_const_puts("CELS(24,0,0,'',15,0,1);");
    screen_const_puts("CELS(24,0,1,'温度',2,0,1);");
    screen_const_puts("CELS(24,0,2,'流速',2,0,1);");
    screen_const_puts("CELS(24,1,0,'主水箱',2,0,1);");
    screen_const_puts("CELS(24,1,1,'',2,0,1);");
    screen_const_puts("CELS(24,1,2,'',2,0,1);");
    screen_const_puts("CELS(24,2,0,'副水箱',2,0,1);");
    screen_const_puts("CELS(24,2,1,'',2,0,1);");
    screen_const_puts("CELS(24,2,2,'',2,0,1);");
    screen_const_puts("SBC(0);");
    screen_const_puts(AREA_1_START);
    screen_const_puts("BOXF(0,0,329,3,2);DS16(1,17,'模式:',2,0);");
    screen_const_puts("LABL(24,40,9,179,'正常用水',1,0);");
    screen_const_puts("BTN(1,0,0,239,120,0);");
    screen_const_puts("SXY(0,0);");
    
    screen_const_puts(AREA_2_START);
    screen_const_puts("BOXF(0,0,319,3,2);");
    screen_const_puts("BTN(2,0,10,119,50,4);LABL(24,2,17,118,'"BUTTON2_CONTENT"',15,1);");
    screen_const_puts("BTN(3,120,10,239,50,4);LABL(24,122,17,238,'"BUTTON3_CONTENT"',15,1);");
    screen_const_puts("BTN(4,0,60,119,100,4);LABL(24,2,67,118,'"BUTTON4_CONTENT"',15,1);");
    screen_const_puts("BTN(5,120,60,239,100,4);LABL(24,122,67,238,'"BUTTON5_CONTENT"',15,1);");
    screen_const_puts("SXY(0,0);\r\n");
    _delay_ms(2000);               /* wait 100ms for main page drawing */
}

