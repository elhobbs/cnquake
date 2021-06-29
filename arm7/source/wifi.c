#include "802.11.h"
#include "802.2.h"
#include "wifi_state.h"
#include <string.h>
#include "hw.h"
#include <nds\interrupts.h>
#include <nds\fifocommon.h>
#include <math.h>

void hw_interrupt();
int wifi_send_probe_request_frame(u8 *ssid);
void hw_init();
void hw_start();
void hw_start_beacons();

ACCESSPOINTSCANLIST *wifi_scan_list = 0;
ACCESSPOINT *wifi_resolve_ap = 0;
int wifi_resolve_ap_count = 0;
int wifi_scan_channel = 0;
u8	*wifi_scan_ssid;
ACCESSPOINT *wifi_host_ap = 0;
WIFI_BUFFER *wifi_rx_buffer = 0;
WIFI_BUFFER *wifi_tx_buffer = 0;

extern char *debug_buffer;

void wifi_scan();
int wifi_send_auth_frame();
int wifi_send_assoc_frame();
void Wifi_Intr_RxEnd();
void Wifi_Intr_TxEnd();

#define LED_LONGBLINK	1
#define LED_SHORTBLINK	3
#define LED_ON			0
int led_state=0;

int PowerChip_ReadWrite(int cmd, int data) {
	if(cmd&0x80) data=0;
	while(SPI_CR&0x80);
	SPI_CR=0x8802;
	SPI_DATA=cmd;
	while(SPI_CR&0x80);
	SPI_CR=0x8002;
	SPI_DATA=data;
	while(SPI_CR&0x80);
	data=SPI_DATA;
	SPI_CR=0;
	return data;
}

void SetLedState(int state) {
	int i;
	if(state>3 || state<0) return;
	if(state!=led_state) {
		led_state=state;
		i=PowerChip_ReadWrite(0x80,0);
		i=i&0xCF;
		i |= state<<4;
		PowerChip_ReadWrite(0,i);
	}
}


void wifi_connect() {
	hw_set_wep_key((void *)wifi->ap.wepkey);
	hw_set_wep_mode(wifi->ap.wepmode);
	W_BSSID[0] = wifi->ap.bssid[0];
	W_BSSID[1] = wifi->ap.bssid[1];
	W_BSSID[2] = wifi->ap.bssid[2];
	hw_set_channel(wifi->ap.channel);
	wifi->state = WIFI_STATE_AUTHENTICATING;
	wifi_send_auth_frame();
	wifi->counter = WIFI_REGISTER(WIFI_USCOUNTER_VAL1);
}

int wifi_send_probe_request_frame(u8 *ssid) {
	u8 frame[320];
	u16 broadcast_mac[3] = {0xffff,0xffff,0xffff};
	u8 *data ;
	int i;
	PROBEREQUEST *probe = (PROBEREQUEST *)frame;
	memset(probe,0,sizeof(*probe));
	probe->header.fc.type = 0;
	probe->header.fc.subType = 4;
	wifi_copy_mac(probe->header.DA,broadcast_mac);
	wifi_copy_mac(probe->header.SA,wifi->mac);
	wifi_copy_mac(probe->header.BSSID,broadcast_mac);
	probe->header.duration = 0;

	data = (u8 *)(probe+1);

	//SSID
	*data++ = 0;
	*data++ = ssid[0];
	for(i=0;i<ssid[0];i++) {
		*data++ = ssid[1+i];
	}

	//supported rates
	*data++ = 1; //type 1 supported rate
	*data++ = 2;//we will try just these two rates and see what happens
	*data++ = 0x82;//1mbit
	*data++ = 0x84;//2mbit

	//request information element
	*data++ = 10;
	*data++ = 4;

	*data++ = 0; //SSID
	*data++ = 1; //rates
	*data++ = 3; //channel
	*data++ = 48; //RSN

	probe->tx.tx_length = (data-frame)-12+4;
	probe->tx.tx_rate = 0x000A;

	wifi->stats_send[4]++;
	return wifi_tx_queue((u16 *)frame,data-frame);
}

int wifi_scan_ap_unique(u16* BSSID) {
	int i;
	if(wifi_scan_list == 0) {
		return 0;
	}
	if(BSSID[0] == 0 &&BSSID[1] == 0 && BSSID[2] == 0) {
		return 0;
	}
	for(i=0;i<wifi_scan_list->count;i++) {
		if(wifi_compare_mac(BSSID,wifi_scan_list->ap[i].bssid)) {
			return 0;
		}
	}
	return 1;
}
extern int wifi_channel;
int wifi_process_ap_data(MGMT_FRAME_HEADER *header,CAPABILITY cap, u8 *frame, u8 *data, int length) {
	u8 *ssid = 0;
	u8 segtype, seglen, ssid_len, channel=wifi_channel;

	do {
		segtype = *data++;
		seglen = *data++;
		switch(segtype) {
		case 0: //SSID
			ssid = data;
			ssid_len = seglen;
			break;
		case 1: //set rate
			break;
		case 3: //channel
			channel = *data;
			break;
		}
		data += seglen;
		//if(segtype >= 3) {
		//	break;
		//}
	} while ( ((int)(data-frame)+2) < length);

	//if we are resolving than update the ap
	if(wifi_resolve_ap && wifi_scan_ap_unique(header->BSSID)) {
		if(wifi->scan_state == WIFI_SCAN_STATE_RESOLVE && (wifi_scan_list->ssid[0] != ssid_len || memcmp(&wifi_scan_list->ssid[1],ssid,ssid_len))) {
			return 0;
		}
		if(ssid_len > 0) {
			memcpy(&wifi_resolve_ap->ssid[1],ssid,ssid_len);
		}
		wifi_resolve_ap->ssid[0] = ssid_len;
		wifi_resolve_ap->ssid[ssid_len+1] = 0;
		wifi_copy_mac(wifi_resolve_ap->bssid,header->BSSID);
		wifi_copy_mac(wifi_resolve_ap->mac,header->SA);
		wifi_resolve_ap->channel = channel;
		wifi_resolve_ap_count++;
		wifi_resolve_ap->adhoc = cap.ibss ? 1 : 0;

		if(wifi_scan_list && wifi_scan_list->count < wifi_scan_list->maxcount) {
			wifi_resolve_ap++;
			wifi_scan_list->count++;
		}
	}

	return 1;
}

int wifi_process_probe_response_frame(u8 *frame, int length) {
	PROBERESPONSE *resp = (PROBERESPONSE *)frame;
	u8 *data = (u8 *)(resp+1);

	//not for us - ignore
	if(wifi->scan_state == WIFI_SCAN_STATE_NONE ||
		wifi_compare_mac(wifi->mac,resp->header.DA) == 0) {
			return 1;
	}

	//if(wifi_resolve_ap) {
	//	wifi_resolve_ap->probe_response++;
		//memcpy(wifi_resolve_ap->buf,wifi->dummy,sizeof(wifi->dummy));
	//}

	return wifi_process_ap_data(&resp->header,resp->cap,frame,data,length);
}

int wifi_process_beacon_frame(u8 *frame, int length) {
	BEACON *beacon = (BEACON *)frame;
	u8 *data = (u8 *)(beacon+1);

	//not for us - ignore
	if(wifi->scan_state == WIFI_SCAN_STATE_NONE) {
			return 1;
	}

	return wifi_process_ap_data(&beacon->header,beacon->cap,frame,data,length);
}

static u8 channel_order[13] = {11,1,6,12,8,10,5,2,3,13,9,4,7};
int fin_counter = 100;
void wifi_scan() {
	if(wifi->scan_state == WIFI_SCAN_STATE_NONE || wifi_resolve_ap == 0) { return; }
	if(wifi_scan_channel >= 13) {
		wifi->scan_state = WIFI_SCAN_STATE_NONE;
		wifi_scan_list = 0;
		wifi_resolve_ap = 0;
		SetLedState(LED_LONGBLINK);
		fifoSendValue32(FIFO_DSWIFI, fin_counter++);
		return;
	}
			//WIFI_REG(0xD0) |= 0x0400;
			//WIFI_REG(0xD0) &= ~0x0800;  // disallow toDS
	/*WIFI_REGISTER(WIFI_RXFILTER) =0x0181;//|= (WIFI_RXFILTER_MANAGEMENT|WIFI_RXFILTERBEACON|WIFI_RXFILTER_PROBE);
	WIFI_REGISTER(WIFI_RXFILTER) =0x0581;//|= (WIFI_RXFILTER_MANAGEMENT|WIFI_RXFILTERBEACON|WIFI_RXFILTER_PROBE);
	WIFI_REG(WIFI_RXFILTER2) = 0x0B;*/
	hw_set_channel(channel_order[wifi_scan_channel]);
	wifi_send_probe_request_frame(wifi_scan_ssid);
	wifi->counter = WIFI_REGISTER(WIFI_USCOUNTER_VAL1);
	wifi_scan_channel++;
}

extern volatile int wifi_irq_counter;
volatile int wifi_vblank_counter = 0;
void wifi_update() {
	static WIFI_STATE last_wifi_state;
	wifi_vblank_counter++;
	//static int i = 0;
	//if(debug_buffer) {
	//	sprintf(debug_buffer,"wi: %d              ",wifi_irq_counter);
		//i++;
	//}

	if(!wifi) return;

	switch(wifi->state) {
	case WIFI_STATE_DISCONNECTED:
		break;
	case WIFI_STATE_AUTHENTICATING:
		 // ~1 second timeout
		if(WIFI_REGISTER(WIFI_USCOUNTER_VAL1)-wifi->counter>20) {
			wifi->state = WIFI_STATE_CANNOT_CONNECT;
		}
		break;
	case WIFI_STATE_AUTHENTICATED:
		wifi->state = WIFI_STATE_ASSOCIATING;
		wifi_send_assoc_frame();
		break;
	case WIFI_STATE_ASSOCIATING:
		 // ~1 second timeout
		if(WIFI_REGISTER(WIFI_USCOUNTER_VAL1)-wifi->counter>20) {
			wifi->state = WIFI_STATE_CANNOT_CONNECT;
		}
		break;
	case WIFI_STATE_ASSOCIATED:
		wifi->state = WIFI_STATE_CONNECTED;
		fifoSendValue32(FIFO_DSWIFI, WIFI_STATE_CONNECTED);
		break;
	case WIFI_STATE_CONNECTED:
		break;
	case WIFI_STATE_CANNOT_CONNECT:
		wifi->state = WIFI_STATE_DISCONNECTED;
		fifoSendValue32(FIFO_DSWIFI, WIFI_STATE_CANNOT_CONNECT);
		break;
	}
	last_wifi_state = wifi->state;
	
	Wifi_Intr_RxEnd();
	
	Wifi_Intr_TxEnd();
	//wifi_frame();


}

extern u16 arm7qlen;
extern int wifi_channel;

int Wifi_TxCheck();
int sending = 0;

u16 rxfilter = 0x0001;
u16 rxfilter2 = 0x000F;

void wifi_frame() {

	if(!wifi_rx_buffer) return;
	
	WIFI_REGISTER(WIFI_RXFILTER) = rxfilter;//0x0581;//|= (WIFI_RXFILTER_MANAGEMENT|WIFI_RXFILTERBEACON|WIFI_RXFILTER_PROBE);
	WIFI_REG(WIFI_RXFILTER2) = rxfilter2;//0x0B;

	switch(wifi->host_state) {
	case WIFI_HOST_STATE_STARTING:
		wifi->host_state = WIFI_HOST_STATE_STARTED;
		hw_start_beacons();
		break;
	case WIFI_HOST_STATE_STARTED:
	case WIFI_HOST_STATE_NONE:
		break;
	}

	if(wifi->scan_state != WIFI_SCAN_STATE_NONE && WIFI_REGISTER(WIFI_USCOUNTER_VAL1)-wifi->counter>10) {
		wifi_scan();
	}


	//if(debug_buffer) siprintf(debug_buffer,"rx_regs %04x %04x\n",WIFI_REG(WIFI_RXWRITECSR),WIFI_REG(WIFI_RXREADCSR));
	//siprintf(debug_buffer,"tx_buffer\n%08x\n%08x\n",wifi_tx_buffer->bufIn,wifi_tx_buffer->bufOut);
	/*siprintf(debug_buffer,
		"arm7qlen: %04d %04d\n"
		"host_state: %d state: %d\n"
		"data send: %04d\n"
		"data recv: %04d\n"
		"channel: %02d\n"
		"rxfilter: %04x %04x\n"
		"lost: %04d\nfound: %04d\ntotal: %04d\n",
		arm7qlen,Wifi_TxCheck(),
		wifi->host_state,wifi->state,
		wifi->data_send,
		wifi->data_recv,
		wifi_channel,
		rxfilter,
		rxfilter2,
		wifi_rx_buffer->lost,wifi_rx_buffer->found,wifi_rx_buffer->total);*/
}

static int tx_count = 0;
void wifi_address_handler(void * address, void * userdata) {
	WIFI_MSG *msg = (WIFI_MSG *)address;

	if(address == 0) {
		return;
	}
	//siprintf(buf,"addr2: %d\n",msg->type);
	//nocashMessage(buf);

	switch(msg->type) {
	case WIFI_MSG_INIT:
		SetLedState(LED_LONGBLINK);
		wifi = (WIFI_MAINSTRUCT *)msg->p;
		wifi_rx_buffer = &wifi->rx_buffer;
		wifi_tx_buffer = &wifi->tx_buffer;
		hw_init();
		hw_start();
		irqEnable(IRQ_WIFI);
		wifi->initialized = 1;
		//fifoSendValue32(FIFO_DSWIFI, 0);
		break;
	case WIFI_MSG_TX:
		Wifi_Intr_TxEnd();
		break;
	case WIFI_MSG_DEBUG_BUFFER:
		debug_buffer = msg->debug_buffer;
		*debug_buffer = 0;
		break;
	case WIFI_MSG_RESOLVE:
		rxfilter |= WIFI_RXFILTER_MANAGEMENT;
		rxfilter2 |=  0x0001;
		WIFI_REG(0x000D) = rxfilter;
		WIFI_REG(0x000E) = rxfilter2;
		SetLedState(LED_SHORTBLINK);
		wifi_scan_list = msg->scanlist;
		wifi_resolve_ap = &wifi_scan_list->ap[0];
		wifi_scan_channel = 0;
		wifi_scan_ssid = wifi_scan_list->ssid;
		wifi->scan_state = WIFI_SCAN_STATE_RESOLVE;
		wifi_scan();
		break;
	case WIFI_MSG_SCAN:
		rxfilter |= WIFI_RXFILTER_MANAGEMENT;
		rxfilter2 |=  0x0001;
		WIFI_REG(0x000D) = rxfilter;
		WIFI_REG(0x000E) = rxfilter2;
		SetLedState(LED_SHORTBLINK);
		wifi_scan_list = msg->scanlist;
		wifi_resolve_ap = &wifi_scan_list->ap[0];
		wifi_scan_channel = 0;
		wifi_scan_ssid = wifi_scan_list->ssid;
		wifi->scan_state = WIFI_SCAN_STATE_ACTIVE;
		wifi_scan();
		break;
	case WIFI_MSG_CONNECT:
		memcpy((void *)&wifi->ap,msg->ap,sizeof(wifi->ap));
		rxfilter &= ~(WIFI_RXFILTER_MANAGEMENT|1);
		rxfilter2 = 0x0008| (wifi->ap.adhoc ? 0x6 : 0);//&=  ~0x0001;
		WIFI_REG(0x000D) = rxfilter;
		WIFI_REG(0x000E) = rxfilter2;
		wifi_connect();
		break;
	case WIFI_MSG_HOST_START:
		rxfilter = 0x401;//|= WIFI_RXFILTER_MANAGEMENT;
		rxfilter2 = 0x0E;//&=  ~0x0001;
		WIFI_REG(0x000D) = rxfilter;
		WIFI_REG(0x000E) = rxfilter2;
		wifi->host_state = WIFI_HOST_STATE_STARTING;
		wifi->ap.adhoc = 1;
		break;
	case WIFI_MSG_RX:
			WIFI_REG(0x00D0) = msg->reg_value;
			rxfilter = msg->reg_value;
		break;
	case WIFI_MSG_RX2:
			WIFI_REG(0x00E0) = msg->reg_value;
			rxfilter2 = msg->reg_value;
		break;
	}

}

void wifi_install_fifo() {
	//memset(&wifi,0,sizeof(wifi));
	//wifi->state = WIFI_STATE_DISCONNECTED;

	irqSet(IRQ_WIFI, hw_interrupt); // set up wifi interrupt
	wifi_irq_counter++;

	fifoSetAddressHandler(FIFO_DSWIFI, wifi_address_handler, 0);

}

