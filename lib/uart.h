#ifndef __UART_H__
#define __UART_H__
#include "ktype.h"
unsigned char get_c( void );

void init_uart(uint16 baud);

unsigned char get_hex();
void put_s(char * ptr);
void put_c(char c);
void put(unsigned char c);
INT8 uart_poll_c(UINT8 * pC);
int uart_print( const char * fmt, ... );
#endif
