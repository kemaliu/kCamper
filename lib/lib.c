#include <stdarg.h>
#include <stdio.h>
void uart_print( const char * fmt, ... )
{
    va_list args;
    int n;
    char printbuffer[64];
    va_start ( args, fmt );
    n = vsprintf ( printbuffer, fmt, args );
    va_end ( args );
    put_s(printbuffer);
}
