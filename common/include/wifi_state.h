#ifndef __WIFI_STATE_H__
#define __WIFI_STATE_H__

#include <nds/ndstypes.h>

typedef enum {
	WIFI_STATE_DISCONNECTED,
	WIFI_STATE_AUTHENTICATING,
	WIFI_STATE_AUTHENTICATED,
	WIFI_STATE_ASSOCIATING,
	WIFI_STATE_ASSOCIATED,
	WIFI_STATE_CONNECTED,
	WIFI_STATE_CANNOT_CONNECT
} WIFI_STATE;

typedef enum {
	WIFI_SCAN_STATE_NONE,
	WIFI_SCAN_STATE_ACTIVE,
	WIFI_SCAN_STATE_RESOLVE
} WIFI_SCAN_STATE;

typedef enum {
	WIFI_HOST_STATE_NONE,
	WIFI_HOST_STATE_STARTING,
	WIFI_HOST_STATE_STARTED
} WIFI_HOST_STATE;


typedef struct ACCESSPOINT {
	u8			ssid[34];
	u16			bssid[3];
	u16			mac[3];
	u16			adhoc;
	char		wepmode;
	char		wepkeyid;
	u8			wepkey[20];
	u8			channel;
	u16			pollperiod;
	
	//debugging
	/*u16		tx_raw;
	u16		probe_response;
	u16		probe_response2;
	u16		probe_response3;
	u16		frames[16];
	u8		dummy[2];
	u8		buf[6*256];//512];*/
} ACCESSPOINT;

typedef struct ACCESSPOINTSCANLIST {
	u8				ssid[34];
	int				count, maxcount;
	ACCESSPOINT		ap[1];
} ACCESSPOINTSCANLIST;

typedef enum {
	WIFI_MSG_INIT,
	WIFI_MSG_RESOLVE,
	WIFI_MSG_SCAN,
	WIFI_MSG_CONNECT,
	WIFI_MSG_DEBUG_BUFFER,
	WIFI_MSG_HOST_START,
	WIFI_MSG_SEND,
	WIFI_MSG_RX,
	WIFI_MSG_RX2,
	WIFI_MSG_TX
} WIFI_MSG_TYPE;

typedef enum {
	WIFI_MSG32_OFF=0,
	WIFI_MSG32_ON=1,
	WIFI_MSG32_STATE=3,
	WIFI_MSG32_SET_RX=4,
	WIFI_MSG32_SET_RX2=5,
	WIFI_MSG32_HOST_START=6
} WIFI_MSG32_TYPE;

typedef struct WIFI_MSG {
	WIFI_MSG_TYPE	type;
	union {
		ACCESSPOINTSCANLIST *scanlist;
		ACCESSPOINT	*ap;
		char *debug_buffer;
		void		*p;
		u16			reg_value;
	};
} WIFI_MSG;

#define WIFI_RXBUFFER_SIZE	(1024*12)

typedef struct WIFI_BUFFER {
	// wifi data
	u32 bufIn, bufOut; // bufIn/bufOut have 2-byte granularity.
	u32 lost, found, total;
	u16 bufData[WIFI_RXBUFFER_SIZE/2]; // send raw 802.11 data through! rxbuffer is for rx'd data, arm7->arm9 transfer
} WIFI_BUFFER;

typedef struct WIFI_MAINSTRUCT {
	int					initialized;
	WIFI_STATE			state;
	u16					mac[3];
	u16					counter;
	u16					aid;

	WIFI_SCAN_STATE		scan_state;

	WIFI_HOST_STATE		host_state;

	int					stats_recv[16];
	int					stats_send[16];
	int					data_recv;
	int					data_send;
	u16					probe_da[3];

	WIFI_BUFFER			rx_buffer;
	WIFI_BUFFER			tx_buffer;

	int					tx_intr;
	unsigned short		stats_debug;

//u8					dummy[6*256];

	ACCESSPOINT			ap;
} WIFI_MAINSTRUCT;

extern WIFI_BUFFER *wifi_rx_buffer;
extern WIFI_BUFFER *wifi_tx_buffer;

extern volatile WIFI_MAINSTRUCT *wifi;
#define FIFO_DSWIFI2 FIFO_USER_02

#endif //end __WIFI_STATE_H__