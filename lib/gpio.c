#include <avr/io.h>
#include "gpio.h"
#include "kconfig.h"
unsigned char  gpio_input(unsigned char group, unsigned char index, unsigned char pull_up)
{
    unsigned char val = 1 << index;
    unsigned char pull_up_val = 1 << index;
    val = ~val;
    switch(group){
        case GPIO_GROUP_B:
            DDRB &= val;
            if((pull_up)){
                PORTB |= pull_up_val;
            }else{
                PORTB &= ~pull_up_val;
            }
            return (PINB >> index) & 1;
        case GPIO_GROUP_C:
            DDRC &= val;
            if((pull_up)){
                PORTC |= pull_up_val;
            }else{
                PORTC &= ~pull_up_val;
            }
            return (PINC >> index) & 1;
        case GPIO_GROUP_D:
            DDRD &= val;
            if((pull_up)){
                PORTD |= pull_up_val;
            }else{
                PORTD &= ~pull_up_val;
            }
            return (PIND >> index) & 1;
        default:
            return 0xff;
    }
}


void gpio_output(unsigned char group, unsigned char index, unsigned char val)
{
    unsigned char direct_val = 1 << index;
    switch(group){
        case GPIO_GROUP_B:
            DDRB |= direct_val;
            if((val)){
                PORTB |= direct_val;
            }else{
                PORTB &= ~direct_val;
            }
            break;
        case GPIO_GROUP_C:
            DDRC |= direct_val;
            if((val)){
                PORTC |= direct_val;
            }else{
                PORTC &= ~direct_val;
            }
            break;
        case GPIO_GROUP_D:
            DDRD |= direct_val;
            if((val)){
                PORTD |= direct_val;
            }else{
                PORTD &= ~direct_val;
            }
            break;
    }
}
