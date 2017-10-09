#include <stdio.h>
#include <util/delay.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/deprecated.h>

#include "ktype.h"
#include "uart.h"
#include "lib.h"


#define printf  uart_print
