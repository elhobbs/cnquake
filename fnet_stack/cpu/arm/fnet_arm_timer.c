#include "fnet_config.h"

#if FNET_ARM
#include "fnet.h"
#include "fnet_timer_prv.h"
#include "fnet_isr.h"
#include "fnet_arm.h"
#ifdef NDS
#include <nds\timers.h>
#endif

void fnet_cpu_timer_release( void )
{
#ifdef NDS
	timerStop(3);
#endif
}

#define ARM_TIMER 0x12

void fnet_arm_timer() {
    /* Call FNET isr handler.*/
    fnet_isr_handler( ARM_TIMER );
}

void fnet_cpu_timer_handler_top() {
    /* Update RTC counter. 
     */
    fnet_timer_ticks_inc(); 
}

int fnet_cpu_timer_init( unsigned int period_ms )
{
    int result = FNET_OK;
    
    result = fnet_isr_vector_init(ARM_TIMER, fnet_cpu_timer_handler_top,
                                              fnet_timer_handler_bottom, 0);
    
    if(result == FNET_OK)
    {
#ifdef NDS
		timerStart(3, ClockDivider_1024, TIMER_FREQ_1024(10), fnet_arm_timer);
#endif
    }

	return result;
}

#endif