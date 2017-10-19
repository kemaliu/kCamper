#ifndef __KCONFIG_H__
#define __KCONFIG_H__
#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>

#include "ktype.h"
#include "uart.h"
#include "lib.h"
#include <avr/pgmspace.h>

#if 0
#define printf(...)  do{}while(0)
#else
#define printf uart_print
#endif

#define screen_cmd_puts  put_s
#define screen_cmd_printf  uart_print


extern char realTimeFmtBuf[32];


#define screen_const_puts(fmt) do{                        \
        static const char fmtBuf[] PROGMEM = fmt;                             \
        strcpy_P(realTimeFmtBuf, fmtBuf);                               \
        put_s(realTimeFmtBuf);                                          \
    }while(0)



#endif  /* #ifndef __KCONFIG_H__ */
