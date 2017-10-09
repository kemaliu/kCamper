#ifndef __GPIO_H__
#define __GPIO_H__
typedef enum {
    GPIO_GROUP_A,
    GPIO_GROUP_B,
    GPIO_GROUP_C,
    GPIO_GROUP_D,
}GPIO_GROUP_T;


unsigned char  gpio_input(unsigned char group, unsigned char index, unsigned char pull_up);

void gpio_output(unsigned char group, unsigned char index, unsigned char val);

#endif  /* #ifndef __GPIO_H__ */
