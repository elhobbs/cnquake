#ifndef __DSAP_H__
#define __DSAP_H__

#include <nds\ndstypes.h>

typedef enum {
	WIFI_CLIENT_DISCONNECTED,
	WIFI_CLIENT_AUTHENTICATING,
	WIFI_CLIENT_AUTHENTICATED,
	WIFI_CLIENT_ASSOCIATING,
	WIFI_CLIENT_ASSOCIATED
} WIFI_CLIENT_STATE;


typedef struct {
	WIFI_CLIENT_STATE	state;
	u16					AID;
	u16					MAC[3];
} wifi_client_t;

extern wifi_client_t wifi_clients[8];

#endif
