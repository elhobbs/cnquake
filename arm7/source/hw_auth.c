#include <nds\ndstypes.h>
#include "hw.h"
#include "802.11.h"
#include "wifi_state.h"
#include <string.h>

volatile WIFI_MAINSTRUCT *wifi;
extern ACCESSPOINT *wifi_resolve_ap;

//WIFI_STATE wifi_state = WIFI_STATE_DISCONNECTED;

int wifi_send_null_frame() {
	u8 frame[128];
	u8 *data;
	DATA_FRAME_HEADER *head = (DATA_FRAME_HEADER *)frame;
	memset(head,0,sizeof(*head));
	head->fc.type = 0x02;
	head->fc.subType = 0x04;
	head->fc.toDS = 1;
	wifi_copy_mac(head->A1,wifi->ap.mac);
	wifi_copy_mac(head->A2,wifi->mac);
	wifi_copy_mac(head->A3,wifi->ap.bssid);

	data = (u8 *)(head+1);
	head->tx.tx_length = data-frame-12+4;

	return wifi_tx_queue((u16 *)frame,data-frame);
}

int wifi_send_auth_frame() {
	u8 frame[128];
	u8 *data;
	AUTHFRAME_HEADER *auth_head = (AUTHFRAME_HEADER *)frame;
	AUTHFRAME_DATA *auth_data = (AUTHFRAME_DATA *)(auth_head+1);
	memset(auth_head,0,sizeof(*auth_head));
	memset(auth_data,0,sizeof(*auth_data));
	auth_head->header.fc.type = 0;
	auth_head->header.fc.subType = 0xB;
	wifi_copy_mac(auth_head->header.DA,wifi->ap.mac);
	wifi_copy_mac(auth_head->header.SA,wifi->mac);
	wifi_copy_mac(auth_head->header.BSSID,wifi->ap.bssid);
	auth_head->header.duration = 0;
	auth_data->algorithmNumber = wifi->ap.wepmode == 0 ? 0 : 1;
	auth_data->transactionSequence = 1;
	auth_data->status = 0;

	data = (u8 *)(auth_data+1);
	auth_head->tx.tx_length = (data-frame)-12+4;
	auth_head->tx.tx_rate = 0x000A;

	wifi->stats_send[0xB]++;

	return wifi_tx_queue((u16 *)frame,data-frame);
}

int wifi_send_auth_resp_frame(int challenge_length,u8 *challenge_text) {
	u8 frame[320];
	u8 *data;
	int i;
	AUTHFRAME_HEADER *auth_head = (AUTHFRAME_HEADER *)frame;
	u32 *iv = (u32 *)(auth_head+1);;
	AUTHFRAME_DATA *auth_data = (AUTHFRAME_DATA *)(iv+1);
	memset(auth_head,0,sizeof(*auth_head));
	memset(auth_data,0,sizeof(*auth_data));
	auth_head->header.fc.type = 0;
	auth_head->header.fc.subType = 0xB;
	auth_head->header.fc.wep = 1;
	wifi_copy_mac(auth_head->header.DA,wifi->ap.mac);
	wifi_copy_mac(auth_head->header.SA,wifi->mac);
	wifi_copy_mac(auth_head->header.BSSID,wifi->ap.bssid);
	auth_head->header.duration = 0;

	auth_data->algorithmNumber = 1;
	auth_data->transactionSequence = 3;
	auth_data->status = 0;

	//set the iv
	*iv=((W_RANDOM ^ (W_RANDOM<<7) ^ (W_RANDOM<<15))&0x0FFF) | (wifi->ap.wepkeyid<<30); // I'm lazy and certainly haven't done this to spec.
	
	//add the challenge text
	data = (u8 *)(auth_data+1);
	*data++ = 0x10;
	*data++ = challenge_length;
	for(i=0;i<challenge_length;i++) {
		*data++ = challenge_text[i];
	}
	auth_head->tx.tx_length = (data-frame)-12+4+4;
	auth_head->tx.tx_rate = 0x000A;

	//memcpy(wifi_resolve_ap->buf,frame,320);
	wifi->stats_send[0xB]++;

	return wifi_tx_queue((u16 *)frame,data-frame);
}

int wifi_process_auth_frame(u8 *frame, int length) {
	AUTHFRAME_HEADER *auth_head = (AUTHFRAME_HEADER *)frame;
	AUTHFRAME_DATA *auth_data = (AUTHFRAME_DATA *)(auth_head+1);
	u8 *data = (u8 *)(auth_data+1);
	u8 segtype, seglen;
	
	//not for us - ignore
	if(wifi->state != WIFI_STATE_AUTHENTICATING ||
		wifi_compare_mac(wifi->mac,auth_head->header.DA) == 0 ||
		wifi_compare_mac(wifi->ap.bssid,auth_head->header.BSSID) == 0) {
			return 1;
	}

	if(auth_data->status == 0) {
		if(auth_data->transactionSequence == 2) {
			if(auth_data->algorithmNumber == 0) { //open system
				wifi->state = WIFI_STATE_AUTHENTICATED;
				return 0;
			} else if(auth_data->algorithmNumber == 1) { //shared key
				segtype = *data++;
				seglen = *data++;
				if(segtype == 0x10) {
					return wifi_send_auth_resp_frame(seglen,data);
				}
			}
		} else if(auth_data->transactionSequence == 4) {
			wifi->state = WIFI_STATE_AUTHENTICATED;
			return 0;
		}
	}

	wifi->state = WIFI_STATE_CANNOT_CONNECT;
	return 1;
}

int wifi_send_assoc_frame() {
	u8 frame[320];
	u8 *data ;
	int i;
	ASSOCREQUESTFRAME *assoc = (ASSOCREQUESTFRAME *)frame;
	memset(assoc,0,sizeof(*assoc));
	assoc->header.fc.type = 0;
	assoc->header.fc.subType = 0;
	wifi_copy_mac(assoc->header.DA,wifi->ap.mac);
	wifi_copy_mac(assoc->header.SA,wifi->mac);
	wifi_copy_mac(assoc->header.BSSID,wifi->ap.bssid);
	assoc->header.duration = 0;

	//capability
	assoc->cap.ess = 1;
	assoc->cap.short_preamble = 1;
	assoc->cap.privacy = (wifi->ap.wepmode==0 ? 0 : 1);

	//listen interval
	assoc->listenInterval = 0;//WIFI_REGISTER(WIFI_LISTENINT);

	data = (u8 *)(assoc+1);

	//SSID
	*data++ = 0;
	*data++ = wifi->ap.ssid[0];
	for(i=0;i<wifi->ap.ssid[0];i++) {
		*data++ = wifi->ap.ssid[1+i];
	}

	//supported rates
	*data++ = 1; //type 1 supported rate
	*data++ = 4;//we will try just these 4 rates and see what happens
	*data++ = 0x02;//1mbit
	*data++ = 0x84;//2mbit
	*data++ = 0x0B;//5.5mbit
	*data++ = 0x16;//11mbit

	assoc->tx.tx_length = (data-frame)-12+4;
	assoc->tx.tx_rate = 0x000A;

	wifi->stats_send[0]++;

	return wifi_tx_queue((u16 *)frame,data-frame);
}

int wifi_process_assoc_response_frame(u8 *frame, int length) {
	ASSOCRESPONSEFRAME *assoc = (ASSOCRESPONSEFRAME *)frame;

	//not for us - ignore
	if(wifi->state != WIFI_STATE_ASSOCIATING ||
		wifi_compare_mac(wifi->mac,assoc->header.DA) == 0 ||
		wifi_compare_mac(wifi->ap.bssid,assoc->header.BSSID) == 0) {
			return 1;
	}

	if(assoc->status == 0) {
		WIFI_REGISTER(WIFI_AIDS) = assoc->AID;
		WIFI_REGISTER(WIFI_AIDS_FULL) = assoc->AID;
		wifi->state = WIFI_STATE_ASSOCIATED;
		wifi->aid = assoc->AID;
		/*WIFI_REGISTER(WIFI_RXFILTER) |= 0x0400;
		WIFI_REGISTER(WIFI_RXFILTER) &= ~0x0800;  // disallow toDS
		WIFI_REGISTER(WIFI_RXFILTER2) |= 0x0002;*/
		return 1;
	}
	wifi->state = WIFI_STATE_CANNOT_CONNECT;
	return 1;
}


