#ifndef _FNET_ARM_CONFIG_H_

#define _FNET_ARM_CONFIG_H_

#define FNET_ARM								(1)

#define FNET_CFG_CPU_LITTLE_ENDIAN              (1)

/* System frequency in Hz. */
#ifndef FNET_CFG_CPU_CLOCK_HZ
#define FNET_CFG_CPU_CLOCK_HZ               (33513982)
#endif

/* Ethernet RX IRQ number */
#undef FNET_CFG_CPU_TIMER_VECTOR_NUMBER
#define FNET_CFG_CPU_TIMER_VECTOR_NUMBER    24  

/* Ethernet RX IRQ number */ 
#undef FNET_CFG_CPU_ETH_VECTOR_NUMBER 
#define FNET_CFG_CPU_ETH_VECTOR_NUMBER      24

/* No cache. */
#define FNET_CFG_CPU_CACHE                  (0) 

#endif