#ifndef __DEBUG_H__
#define __DEBUG_H__
#include "uart.h"

#define ENABLE_DBG_PRINT

#ifdef ENABLE_DBG_PRINT
#define DBG_PRINT(fmt, arg...) do{		\
      printk(fmt, ##arg);			\
  }while(0)
#else
#define DBG_PRINT(fmt, arg...) do{		\
  }while(0)
#endif


#ifdef INCLUDE_KTANK_UART



extern char realTimeFmtBuf[64];
#if 1
char realTimeFmtBuf[64];
#include <avr/pgmspace.h>

#define printk(fmt, arg...) do{static const char fmtBuf[] PROGMEM = fmt;	\
      strcpy_P(realTimeFmtBuf, fmtBuf);					\
      uart_print(realTimeFmtBuf, ##arg);				\
  }while(0)
#else
#define printk uart_print
#endif
#else  /* #ifdef INCLUDE_KTANK_UART */
#define printk(fmt, arg...) do{ }while(0)
#endif /* #ifdef INCLUDE_KTANK_UART */


#endif

