#include "kconfig.h"

void env_test()
{
    unsigned long val = 0x12345678;
    float fval = 1.12345678;
    printf("short size %d\n", sizeof(short));
    printf("int size %d\n", sizeof(int));
    printf("long size %d\n", sizeof(long));
    printf("long long size %d\n", sizeof(long long));
    printf("float size %d\n", sizeof(float));
    printf("printf %%x:%x\n", val);
    printf("printf %%lx:%lx\n", val);
    printf("printf %%f:%f\n", fval);
    
}
