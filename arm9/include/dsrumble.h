#ifndef __DSRUMBLE_H__
#define __DSRUMBLE_H__

typedef enum {rum_pain,rum_muzzle,rum_health,rum_max} rumble_t;

void ds_rumble(rumble_t index,int freq,float duration);

void ds_rumble_state(void);

void ds_rumble_update(void);

void ds_rumble_timer(void);

void ds_rumble_on();

void ds_rumble_off();

void ds_rumble_3in1_init(void);

extern volatile unsigned ds_rumble_count;
extern volatile int rumble_freq;

#endif