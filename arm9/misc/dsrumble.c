#include		"quakedef.h"
#include		"dsrumble.h"

volatile unsigned ds_rumble_time[3] = {0,0,0};
volatile int ds_rumble_freq[3] = {0,0,0};

volatile unsigned ds_rumble_count;

volatile int rumble_freq = 0;

static int ds_rumble_initialized = 0;

void ds_rumble(rumble_t index,int freq,float duration)
{
	ds_rumble_time[index] = ds_rumble_count + (duration*4096.0);
	ds_rumble_freq[index] = freq;
	ds_rumble_update();
}

void ds_rumble_state()
{
	int f;

	if(cls.state == ca_connected && 
		cl.viewentity < cl.num_entities)
	{
		if(cl_entities[cl.viewentity]->effects & EF_MUZZLEFLASH)
		{
			cl.rumble_time = cl.time + 0.001;
			ds_rumble(rum_muzzle,ds_rmuzzle.value,0.07);
		}
		f = cl.stats[STAT_HEALTH]/20;
		if(f < 4 && cl.stats[STAT_HEALTH] > 0)
		{
			float t[] = {0.07,0.03,0.01,0.001};
			ds_rumble(rum_health,ds_rhealth.value,t[f]);
		}
	}
}

void ds_rumble_update()
{
	int freq = 0;

	if(ds_rumble_time[0] > ds_rumble_count)
	{
		freq |= (1<<ds_rumble_freq[0]);
	}
	if(ds_rumble_time[1] > ds_rumble_count)
	{
		freq |= (1<<ds_rumble_freq[1]);
	}
	if(ds_rumble_time[2] > ds_rumble_count)
	{
		freq |= (1<<ds_rumble_freq[2]);
	}
	rumble_freq = freq;
}

void ds_rumble_timer(void)
{
	int bit=1<<16;
	int mask=(1<<16)-1;
	do
	{
		if(rumble_freq & bit &&
			(mask & ds_rumble_count) == mask)
		{
#ifdef NDS
            *(vu16 *)0x8000000 = 0x00;//witre any value for vibration.
            *(vu16 *)0x8000000 = 0x02;
#endif
			break;
		}
		bit >>= 1;
		mask >>= 1;
	} while(bit);

	ds_rumble_update();

	ds_rumble_count++;
}

#ifdef NDS
void ds_rumble_3in1_mode(u16 data)
{
	*(vuint16 *)0x9fe0000 = 0xd200;
	*(vuint16 *)0x8000000 = 0x1500;
	*(vuint16 *)0x8020000 = 0xd200;
	*(vuint16 *)0x8040000 = 0x1500;
	*(vuint16 *)0x9E20000 = data;
	*(vuint16 *)0x9fc0000 = 0x1500;
}
#endif

void ds_rumble_on()
{
	if(ds_rumble_initialized == 0)
		return;
#ifdef NDS
	sysSetCartOwner(true);
	irqSet(IRQ_TIMER2, ds_rumble_timer);
	irqEnable(IRQ_TIMER2);
	TIMER2_DATA = TIMER_FREQ_256(4096);
	TIMER2_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_256;
#endif
}

void ds_rumble_off()
{
	if(ds_rumble_initialized == 0)
		return;
#ifdef NDS
	irqDisable(IRQ_TIMER2);
#endif
}

void ds_rumble_3in1_init(void)
{
	ds_rumble_initialized = 1;
#ifdef NDS
	sysSetCartOwner(true);
	irqSet(IRQ_TIMER2, ds_rumble_timer);
	irqEnable(IRQ_TIMER2);
	TIMER2_DATA = TIMER_FREQ_256(4096);
	TIMER2_CR = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_256;
	ds_rumble_3in1_mode(0xF0);
#endif
}