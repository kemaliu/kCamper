#include "rf.h"
#include "uart.h"




UINT8 __rf_plane__[RF_PLANE_NUM] = {11, 12, 13, 14,
			16, 17, 18, 19, 
			21, 23, 24, 26, 
			27, 28, 29, 31};


UINT8 rf_channel_inc(UINT8 frame, UINT8 plane)
{
    if(plane >= 16)
      plane = 0;
    frame = frame + __rf_plane__[plane];
    while(frame >= 125){
	frame = frame - 125;
    }
    return frame;
}
