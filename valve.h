#ifndef __VALVE_H__
#define __VALVE_H__
enum{
    SCENE_NORMAL = 0,
    SCENE_WATER_TANK1_TO_TANK2 = 1,
    SCENE_WATER_TANK1_LOOP = 2,
    SCENE_WATER_TANK2_LOOP = 3,
    SCENE_WATER_TANK2_TO_TANK1 = 4,
    SCENE_TOTAL_NUM,
};


void valve_setup(UINT8 scene);
void valve_check();
void valve_power_down();
void valve_init();

/* time(seconds) for valve switching */
#define VALVE_OPR_TIME 10


#endif  /* #ifndef __VALVE_H__ */
