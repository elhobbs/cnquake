#ifndef __802_2_H__
#define __802_2_H__

#include <nds\ndstypes.h>

typedef struct WIFI_LLC {
	u8 DSAP;
	u8 SSAP;
	u8 command;
} WIFI_LLC;

typedef struct WIFI_SNAP {
	u8 org_code[3];
	u8 ether_type[2];
} WIFI_SNAP;

typedef struct WIFI_802_2 {
	u8	DSAP;
	u8	SSAP;
	u8	command;
	u8	org_code[3];
	u8	ether_type[2];
} WIFI_802_2;


#endif

