#ifndef __K_TYPE_H__
#define __K_TYPE_H__

#ifndef F_CPU
#warning F_CPU not defined, use default value 7372800
#define F_CPU   7372800
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;

typedef unsigned short UINT16;
typedef short INT16;


typedef unsigned char uint8;
typedef unsigned char UINT8;
typedef char int8;
typedef char INT8;

typedef unsigned short uint16;
typedef short int16;

typedef unsigned long uint32;
typedef long int32;

typedef unsigned long UINT32;
typedef long INT32;

typedef unsigned long long uint64;
typedef long long int64;

typedef unsigned long long UINT64;
typedef long long INT64;


#define ERROR -1
#define OK 0
#endif
