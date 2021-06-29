#ifndef __801_11_H__
#define __801_11_H__

#include <nds\ndstypes.h>

typedef struct WIFI_TXHEADER {
	u16 enable_flags;
	u16 unknown;
	u16 countup;
	u16 beaconfreq;
	u16 tx_rate;
	u16 tx_length;
} Wifi_TxHeader;

typedef struct WIFI_RXHEADER {
	u16 a;
	u16 b;
	u16 c;
	u16 d;
	u16 byteLength;
	u16 rssi_;
} Wifi_RxHeader;

typedef enum {
	TYPE_MANAGEMENT=0,
	TYPE_CONTROL=1,
	TYPE_DATA=2
} WIFI_CONTROL_TYPE;

typedef enum {
	SUBTYPE_ASSOC_REQ=0,
	SUBTYPE_ASSOC_RESP=1,
	SUBTYPE_REASSOC_REQ=2,
	SUBTYPE_REASSOC_RESP=3,
	SUBTYPE_PROBE_REQ=4,
	SUBTYPE_PROBE_RESP=5,
	SUBTYPE_BEACON=8,
	SUBTYPE_ATIM=9,
	SUBTYPE_DISASSOC=10,
	SUBTYPE_AUTH=11,
	SUBTYPE_DEAUTH=12
} WIFI_CONTROL_SUBTYPE;

typedef struct FRAMECONTROL
{
	unsigned short		version:2 ;
	unsigned short		type:2 ;
	unsigned short		subType:4 ;
	unsigned short		toDS:1 ;
	unsigned short		fromDS:1 ;
	unsigned short		moreFrag:1 ;
	unsigned short		retry:1 ;
	unsigned short		powerMgm:1 ;
	unsigned short		moreData:1 ;
	unsigned short		wep:1 ;
	unsigned short		ordered:1 ;
} FRAMECONTROL, *LPFRAMECONTROL ;

typedef struct CAPABILITY
{
	unsigned short		ess:1 ;
	unsigned short		ibss:1 ;
	unsigned short		cf_pollable:1 ;
	unsigned short		cf_poll_request:1 ;
	unsigned short		privacy:1 ;
	unsigned short		short_preamble:1 ;
	unsigned short		pbcc:1 ;
	unsigned short		channel_agility:1 ;
	unsigned short		spectrum_management:1 ;
	unsigned short		QoS:1 ;
	unsigned short		short_slot_time:1 ;
	unsigned short		apsd:1 ;
	unsigned short		reserved:1 ;
	unsigned short		dsss_ofdm:1 ;
	unsigned short		delayed_block_ack:1 ;
	unsigned short		immediate_block_ack:1 ;
} CAPABILITY, *LPCAPABILITY;


typedef struct WIFI_ACCESSPOINT {
	char ssid[33]; // 0-32byte data, zero
	char ssid_len;
	u8 bssid[6];
	u8 macaddr[6];
	u16 maxrate; // max rate is measured in steps of 1/2Mbit - 5.5Mbit will be represented as 11, or 0x0B
	u32 timectr;
	u16 rssi;
	u16 flags;
	u32 spinlock;
	u8 channel;
	u8 rssi_past[8];
	u8 base_rates[16]; // terminated by a 0 entry
} Wifi_AccessPoint;

typedef struct MGMT_FRAME_HEADER
{
	FRAMECONTROL fc;
	u16 duration;
	u16 DA[3];
	u16 SA[3];
	u16 BSSID[3];
	u16 sequence;
} MGMT_FRAME_HEADER, *LPMGMT_FRAME_HEADER;

typedef struct BEACON
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	MGMT_FRAME_HEADER header;
	u16 timestamp[4];
	u16 interval;
	CAPABILITY cap;
} BEACON, *LPBEACON, PROBERESPONSE, *LPPROBERESPONSE;

typedef struct PROBEREQUEST
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	MGMT_FRAME_HEADER header;
} PROBEREQUEST, *LPPROBEREQUEST;

typedef struct AUTHFRAME_HEADER
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	MGMT_FRAME_HEADER header;
} AUTHFRAME_HEADER, *LPAUTHFRAME_HEADER;

typedef struct AUTHFRAME_DATA
{
	u16 algorithmNumber;
	u16 transactionSequence;
	u16 status;
} AUTHFRAME_DATA, *LPAUTHFRAME_DATA;

typedef struct ASSOCREQUESTFRAME
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	MGMT_FRAME_HEADER header;
	CAPABILITY cap;
	u16 listenInterval;
} ASSOCREQUESTFRAME, *LPASSOCREQUESTFRAME;

typedef struct ASSOCRESPONSEFRAME
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	MGMT_FRAME_HEADER header;
	CAPABILITY cap;
	u16 status;
	u16 AID;
} ASSOCRESPONSEFRAME, *LPASSOCRESPONSEFRAME;

typedef struct DATA_FRAME_HEADER
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	FRAMECONTROL fc;
	u16 duration;
	u16 A1[3];
	u16 A2[3];
	u16 A3[3];
	u16 sequence;
} DATA_FRAME_HEADER, *LPDATA_FRAME_HEADER;

typedef struct FRAME_HEADER
{
	union {
		Wifi_TxHeader tx;
		Wifi_RxHeader rx;
	};
	FRAMECONTROL fc;
} FRAME_HEADER;

#endif //__801_11_H__
