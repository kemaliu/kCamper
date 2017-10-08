#ifndef __RF_PROTOCL_H__
#define __RF_PROTOCL_H__


typedef enum{
  KFISH_LED_LIGHT = 0, 
  KFISH_SWITCH,
}KFISH_DEV_T;
struct kfish_led_info{
    char name[16];
    unsigned char lowest_pwm;	/* lowest pwm value, this value match 0% */
    unsigned char watt;		/* power, watt */
    unsigned int char pwm[24];	/* pwm value for every hour */
    char temperature;		/* led temperature */
};

struct kfish_switch_info{
    char name[16];
    unsigned int char pwm[24];	/* pwm value for every hour */
};


typedef enum{
  KFISH_CMD_TIME_GET = 0,	/* slave query current time */
  KFISH_CMD_TIME_NOTIRY,	/* master notify current time */
  KFISH_CMD_CTRL_SET,		/* master setup controller */
  KFISH_CMD_CTRL_ACK,		/* master setup controller */
  KFISH_CMD_CTRL_GET,		/* master get controller info */
  KFISH_CMD_CTRL_RESP,		/* slave response for get  */
}KFISH_CMD_T;

struct kfish_ctrl{
    KFISH_DEV_T devType;
    union dev{
	struct kfish_led_info led;
	struct kfish_switch_info swi;
    }
};


/* every kfish device has a RF module */
struct kfish_dev{
    union{
	void * rf;
	INT8 ctrlNum;
    };
    struct kfish_ctrl ctrl[0];
};


#endif
