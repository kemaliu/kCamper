#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>

#include "ktype.h"
#include "uart.h"
#include "lib.h"

#if 0
#define printf(...)  do{}while(0)
#else
#define printf uart_print
#endif

#define screen_cmd_puts  put_s
#define screen_cmd_printf  uart_print

