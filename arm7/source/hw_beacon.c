#include <nds\ndstypes.h>
#include <nds\system.h>
#include "hw.h"
#include "802.11.h"
#include "wifi_state.h"
#include "dsap.h"
#include <string.h>

extern char *debug_buffer;

char WIFI_HOST_NAME[64] =  "dsnifi";
//static u8 buffer[512];

wifi_client_t wifi_clients[8] = {
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}},
	{WIFI_CLIENT_DISCONNECTED,0,{0,0,0}}
};

wifi_client_t* wifi_get_client(u16 *mac) {
	int i;
	for(i=0;i<8;i++) {
		if(wifi_compare_mac(mac,wifi_clients[i].MAC)) {
			return &wifi_clients[i];
		}
	}
	return 0;
}

wifi_client_t* wifi_new_client(u16 *mac,WIFI_CLIENT_STATE state) {
	int i;
	wifi_client_t *client = wifi_get_client(mac);
	if(client) {
		client->state = state;
		return client;
	}
	for(i=0;i<8;i++) {
		if(wifi_clients[i].state == WIFI_CLIENT_DISCONNECTED) {
			wifi_clients[i].state = state;
			wifi_clients[i].AID = i + 1;
			wifi_copy_mac(&wifi_clients[i].MAC,mac);
			return &wifi_clients[i];
		}
	}

	return 0;
}

int hw_process_assoc_request(u8 *frame, int length) {
	u8 resp_buffer[512];
	ASSOCREQUESTFRAME *req = (ASSOCREQUESTFRAME *)frame;
	u8 *data = (u8 *)(req+1);
	ASSOCRESPONSEFRAME *resp = (ASSOCRESPONSEFRAME *)resp_buffer;
	u8 *respdata = (u8 *)(resp+1);
	wifi_client_t *client;
	u8 segtype, seglen;
	
	do {
		segtype = *data++;
		seglen = *data++;
		switch(segtype) {
		case 0: //SSID
			if(memcmp(WIFI_HOST_NAME,data,seglen)) { //not for us
				return 0;
			}
			*respdata++ = 0;
			*respdata++ = strlen(WIFI_HOST_NAME);
			memcpy(respdata,WIFI_HOST_NAME,strlen(WIFI_HOST_NAME));
			respdata += seglen;
			break;
		case 1: //set rate
			*respdata++ = 1; //type 1 supported rate
			*respdata++ = 2;//we will try just these two rates and see what happens
			*respdata++ = 0x82;//1mbit
			*respdata++ = 0x84;//2mbit
			break;
		}
		data += seglen;
	} while ( ((int)(data-frame)+2) < length);

	client = wifi_new_client(req->header.SA,WIFI_CLIENT_ASSOCIATING);
	if(client == 0) {
		return 0;
	}

	memset(resp,0,sizeof(*resp));
	resp->header.fc.type = 0;
	resp->header.fc.subType = 0x1;
	//auth_head->header.fc.wep = 1;
	resp->AID = client->AID;
	resp->status = 0;
	
	wifi_copy_mac(resp->header.DA,req->header.SA);
	wifi_copy_mac(resp->header.SA,wifi->mac);
	wifi_copy_mac(resp->header.BSSID,wifi->mac);

	resp->tx.tx_length = (respdata-resp_buffer)-12+4;//+4;
	resp->tx.tx_rate = 0x000A;

	//memcpy(wifi_resolve_ap->buf,frame,320);
	wifi->stats_send[1]++;

	return wifi_tx_queue((u16 *)resp,respdata-resp_buffer);
}

int hw_send_auth_resp_frame(AUTHFRAME_HEADER *auth_head_client,AUTHFRAME_DATA *auth_data_client) {
	u8 frame[320];
	u8 *data;
	int i;
	AUTHFRAME_HEADER *auth_head = (AUTHFRAME_HEADER *)frame;
	//u32 *iv = (u32 *)(auth_head+1);;
	AUTHFRAME_DATA *auth_data = (AUTHFRAME_DATA *)(auth_head+1);


	memset(auth_head,0,sizeof(*auth_head));
	memset(auth_data,0,sizeof(*auth_data));

	auth_head->header.fc.type = 0;
	auth_head->header.fc.subType = 0xB;
	//auth_head->header.fc.wep = 1;
	
	wifi_copy_mac(auth_head->header.DA,auth_head_client->header.SA);
	wifi_copy_mac(auth_head->header.SA,wifi->mac);
	wifi_copy_mac(auth_head->header.BSSID,wifi->mac);
	auth_head->header.duration = 0;

	auth_data->algorithmNumber = 0; //open system
	auth_data->transactionSequence = 2;
	auth_data->status = 0;

	//set the iv
	//*iv=((W_RANDOM ^ (W_RANDOM<<7) ^ (W_RANDOM<<15))&0x0FFF) | (wifi->ap.wepkeyid<<30); // I'm lazy and certainly haven't done this to spec.
	
	//add the challenge text
	data = (u8 *)(auth_data+1);
	/*
	*data++ = 0x10;
	*data++ = challenge_length;
	for(i=0;i<challenge_length;i++) {
		*data++ = challenge_text[i];
	}*/
	auth_head->tx.tx_length = (data-frame)-12+4;//+4;
	auth_head->tx.tx_rate = 0x000A;

	//memcpy(wifi_resolve_ap->buf,frame,320);

	//sprintf(debug_buffer,"hello world");
	return wifi_tx_queue((u16 *)frame,data-frame);
}

void hw_process_auth_request(u8 *frame, int length) {
	AUTHFRAME_HEADER *auth_head = (AUTHFRAME_HEADER *)frame;
	AUTHFRAME_DATA *auth_data = (AUTHFRAME_DATA *)(auth_head+1);
	u8 *data = (u8 *)(auth_data+1);
	
	if(auth_data->transactionSequence == 1) {
		hw_send_auth_resp_frame(auth_head,auth_data);
	}
}

int hw_process_probe_request(u8 *frame, int length) {
	u8 resp_buffer[512];
	PROBEREQUEST *probe = (PROBEREQUEST *)frame;
	u8 *data = (u8 *)(probe+1);
	u8 segtype, seglen, ssid_len, channel=0;
	PROBERESPONSE *resp = (PROBERESPONSE *)resp_buffer;
	u8 *respdata = (u8 *)(resp+1);


	do {
		segtype = *data++;
		seglen = *data++;
		switch(segtype) {
		case 0: //SSID
			if(seglen != 0 && memcmp(WIFI_HOST_NAME,data,seglen)) { //not for us
				return 0;
			}
			*respdata++ = 0;
			*respdata++ = strlen(WIFI_HOST_NAME);
			memcpy(respdata,WIFI_HOST_NAME,strlen(WIFI_HOST_NAME));
			respdata += strlen(WIFI_HOST_NAME);
			break;
		case 1: //set rate
			*respdata++ = 1; //type 1 supported rate
			*respdata++ = 2;//we will try just these two rates and see what happens
			*respdata++ = 0x82;//1mbit
			*respdata++ = 0x84;//2mbit
			break;
		case 3: //channel
			*respdata++ = 3; //type 3 current channel
			*respdata++ = 1;
			*respdata++ = 1; //this is hardcoded as channel 1 for now
			break;
		}
		data += seglen;
	} while ( ((int)(data-frame)+2) < length);

	memset(resp,0,sizeof(*resp));
	resp->header.fc.type = 0;
	resp->header.fc.subType = 0x5;
	//auth_head->header.fc.wep = 1;
	
	//set ibss
	resp->cap.ibss = 1;

	wifi_copy_mac(wifi->probe_da,probe->header.SA);
	wifi_copy_mac(resp->header.DA,probe->header.SA);
	wifi_copy_mac(resp->header.SA,wifi->mac);
	wifi_copy_mac(resp->header.BSSID,wifi->mac);
	resp->interval = 200;

	resp->timestamp[0] = WIFI_REGISTER(WIFI_USCOUNTER_VAL0);
	resp->timestamp[1] = WIFI_REGISTER(WIFI_USCOUNTER_VAL1);
	resp->timestamp[2] = WIFI_REGISTER(WIFI_USCOUNTER_VAL2);
	resp->timestamp[3] = WIFI_REGISTER(WIFI_USCOUNTER_VAL3);

	resp->tx.tx_length = (respdata-resp_buffer)-12+4;//+4;
	resp->tx.tx_rate = 0x000A;

	//memcpy(wifi_resolve_ap->buf,frame,320);

	wifi->stats_send[5]++;

	return wifi_tx_queue((u16 *)resp,respdata-resp_buffer);
}


void hw_start_beacons() {
	u8 buffer[512];
	LPBEACON beacon = (LPBEACON)buffer;
	unsigned char *cinfo,*info = (unsigned char *)(beacon+1);
	int i, total_length;
	u16 broadcast_mac[3] = {0xffff,0xffff,0xffff};
	char name[64];
	s16 *name16 = &(PersonalData->name[0]);
	int nameLen = PersonalData->nameLen;
	int valid = 1;


	//use the system name as the ssid
	if(nameLen > 0 && nameLen <= 10) {
		for(i=0;i<nameLen;i++) {
			name[i] = name16[i];
		}
		name[i] = 0;
		if(valid) {
			strcpy(WIFI_HOST_NAME,name);
		}
	}

	memset(beacon,0,sizeof(*beacon));

	//WIFI_REGISTER(WIFI_RXFILTER) |= (WIFI_RXFILTER_MANAGEMENT|WIFI_RXFILTERBEACON|WIFI_RXFILTER_PROBE);
	/*WIFI_REGISTER(WIFI_RXFILTER) =0x0301;//|= (WIFI_RXFILTER_MANAGEMENT|WIFI_RXFILTERBEACON|WIFI_RXFILTER_PROBE);
	WIFI_REG(WIFI_RXFILTER2) = 0x0D;*/

	hw_set_channel(1);

	WIFI_REGISTER(WIFI_BSSID + 0)		= wifi->mac[0] ;
	WIFI_REGISTER(WIFI_BSSID + 2)		= wifi->mac[1] ;
	WIFI_REGISTER(WIFI_BSSID + 4)		= wifi->mac[2] ;

	wifi_copy_mac(wifi->ap.bssid,wifi->mac);
	wifi->ap.adhoc = 1;

	cinfo = info;

	beacon->tx.tx_rate = 0x0A;
	beacon->header.fc.subType = 8;

	//set ibss
	beacon->cap.ibss = 1;

	//broadcast mac
	wifi_copy_mac(beacon->header.DA,broadcast_mac);

	//the stations mac
	wifi_copy_mac(beacon->header.SA,wifi->mac);

	//the bssid for the ibss
	wifi_copy_mac(beacon->header.BSSID,wifi->mac);

	//set the beacon rate to every 100 ms
	beacon->interval = 200;

	//set the capabilities for an ibss
	//beacon->cap.ibss = 1;
	//beacon->cap.cf_poll_request = 1;
	//beacon->cap.short_preamble = 1;

	/* NOT NEEDED
	
	//set the timestamp
	beacon->timestamp[0] = WIFI_REGISTER(WIFI_USCOUNTER_VAL0);
	beacon->timestamp[1] = WIFI_REGISTER(WIFI_USCOUNTER_VAL1);
	beacon->timestamp[2] = WIFI_REGISTER(WIFI_USCOUNTER_VAL2);
	beacon->timestamp[3] = WIFI_REGISTER(WIFI_USCOUNTER_VAL3);*/

	//info 0 - ssid
	*cinfo++ = 0; //type 0 ssid
	*cinfo++ = strlen(WIFI_HOST_NAME);
	memcpy(cinfo,WIFI_HOST_NAME,strlen(WIFI_HOST_NAME));
	cinfo += strlen(WIFI_HOST_NAME);

	//info 1 - supported rates
	*cinfo++ = 1; //type 1 supported rate
	*cinfo++ = 2;//we will try just these two rates and see what happens
	*cinfo++ = 0x82;//1mbit
	*cinfo++ = 0x84;//2mbit

	//info 2 - channel
	*cinfo++ = 3; //type 3 current channel
	*cinfo++ = 1;
	*cinfo++ = 1; //this is hardcoded as channel 1 for now

	//info 3 - ibss parameter ste
	//*cinfo++ = 6; //type 3 current channel
	//*cinfo++ = 2;
	//*cinfo++ = 0;
	//*cinfo++ = 0;
	
	//info 3 TIM
	*cinfo++ = 5;
	*cinfo++ = 5;
	*cinfo++ = 0;
	*cinfo++ = 1;
	*cinfo++ = 0;
	*cinfo++ = 0;
	*cinfo++ = 0;

	//info 4 - ERP
	//*cinfo++ = 42; //type 42 ERP
	//*cinfo++ = 1;
	//*cinfo++ = 0; //set it to 0 - maybe look into this later

	total_length = sizeof(BEACON) + (cinfo-info);
	beacon->tx.tx_length = total_length - 12 + 4;
	for(i=0;i<total_length;i+=2) {
		WIFI_REGISTER(0x4A00 + i) = ((unsigned short *)beacon)[i/2] ;
	}
#if 1
#if 1
	WIFI_REGISTER(WIFI_BEACONPERIOD)	= 200; // 200 ms
	WIFI_REGISTER(W_BEACONCOUNT1)		= 200; // 200 ms
	WIFI_REGISTER(WIFI_PRE_BEACON)		= 0x1000; //let me know 4ms before it sends... in theory?
	WIFI_REGISTER(WIFI_TXMODIFY)		= WIFI_USESW_DURATION;
	WIFI_REGISTER(WIFI_BEACONTRANS)		= 0x8000 | (0x4A00>>1);
	WIFI_REGISTER(WIFI_USCOUNTER_CR)	= 1 ;
	
	WIFI_REGISTER(0x0118)		= 0x0400;
	WIFI_REGISTER(0x0090)		= 0x8000;
	WIFI_REGISTER(0x00ee)		= 1;
	WIFI_REGISTER(WIFI_TXCNT)		= BIT(1);
	
	WIFI_REGISTER(0x0194)	= 0x0000 ;
#else
	WIFI_REGISTER(WIFI_BEACONPERIOD)	= 200; // 200 ms
	WIFI_REGISTER(W_BEACONCOUNT1)		= 200; // 200 ms
	WIFI_REGISTER(WIFI_PRE_BEACON)		= 0x1000; //let me know 4ms before it sends... in theory?
	WIFI_REGISTER(WIFI_TXMODIFY)		= WIFI_USESW_DURATION;
	WIFI_REGISTER(WIFI_BEACONTRANS)		= 0x8000;
	WIFI_REGISTER(WIFI_USCOUNTER_CR)	= 1 ;
	WIFI_REGISTER(0x0194)	= 0x0000 ;
#endif

#endif
}
