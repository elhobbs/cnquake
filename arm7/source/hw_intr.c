#include <nds\interrupts.h>
#include <nds\fifocommon.h>
#include "hw.h"
#include "802.11.h"
#include "wifi_state.h"
#include <string.h>

u16 arm7q[1024];
u16 arm7qlen = 0;
char *debug_buffer = 0;

volatile int wifi_irq_counter = 0;

int wifi_process_auth_frame(u8 *frame, int length);
int wifi_process_assoc_response_frame(u8 *frame, int length);
int wifi_process_probe_response_frame(u8 *frame, int length);
int wifi_process_beacon_frame(u8 *frame, int length);
void hw_process_auth_request(u8 *frame, int length);
void hw_process_probe_request(u8 *frame, int length);
int hw_process_assoc_request(u8 *frame, int length);

extern ACCESSPOINT *wifi_resolve_ap;

u16 Wifi_MACRead(u32 MAC_Base, u32 MAC_Offset) {
	MAC_Base += MAC_Offset;
	if(MAC_Base>=(WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)) MAC_Base -= ((WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)-(WIFI_REG(WIFI_RXRANGE_BEGIN)&0x1FFE));
	return WIFI_REG(0x4000+MAC_Base);
}

void Wifi_MACCopy(u16 * dest, u32 MAC_Base, u32 MAC_Offset, u32 length) {
	int endrange,subval;
	int thislength;
	endrange = (WIFI_REG(WIFI_RXRANGE_END)&0x1FFE);
	subval=((WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)-(WIFI_REG(WIFI_RXRANGE_BEGIN)&0x1FFE));
	MAC_Base += MAC_Offset;
	if(MAC_Base>=endrange) MAC_Base -= subval;
	while(length>0) {
		thislength=length;
		if(thislength>(endrange-MAC_Base)) thislength=endrange-MAC_Base;
		length-=thislength;
		while(thislength>0) {
			*(dest++) = WIFI_REG(0x4000+MAC_Base);
			MAC_Base+=2;
			thislength-=2;
		}
		MAC_Base-=subval;
	}
}

void Wifi_MACWrite(u16 * src, u32 MAC_Base, u32 MAC_Offset, u32 length) {
	int endrange,subval;
	int thislength;
	endrange = (WIFI_REG(WIFI_RXRANGE_END)&0x1FFE);
	subval=((WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)-(WIFI_REG(WIFI_RXRANGE_BEGIN)&0x1FFE));
	MAC_Base += MAC_Offset;
	if(MAC_Base>=endrange) MAC_Base -= subval;
	while(length>0) {
		thislength=length;
		if(length>(endrange-MAC_Base)) length=endrange-MAC_Base;
		length-=thislength;
		while(thislength>0) {
			WIFI_REG(0x4000+MAC_Base) = *(src++);
			MAC_Base+=2;
			thislength-=2;
		}
		MAC_Base-=subval;
	}
}

void Wifi_TxRaw(u16 * data, int datalen) {
	//if(wifi_resolve_ap) {
	//	wifi_resolve_ap->tx_raw++;
	//}
	wifi->data_send++;
	datalen = (datalen+3)&(~3);
	Wifi_MACWrite(data, 0, 0, datalen);
	//	WIFI_REG(0xB8)=0x0001;
	WIFI_REG(0x2C)=0x0707;
	WIFI_REG(WIFI_TXLOC3)=0x8000;
	WIFI_REGISTER(WIFI_TXCNT)=0x0008;
}

int Wifi_TxCheck() {
	if(WIFI_REG(WIFI_TXBUSY)&0x0008) return 0;
	return 1;
}

int wifi_tx_queue(u16 * data, int datalen) {
	int i,j;
	if(arm7qlen) {
		if(Wifi_TxCheck()) {
			Wifi_TxRaw(arm7q,arm7qlen);
			arm7qlen=0;
			j=(datalen+1)>>1;
			if(j>1024) return 0;
			for(i=0;i<j;i++) arm7q[i]=data[i];
			arm7qlen=datalen;
			return 1;
		}
		return 0;
	}
	if(Wifi_TxCheck()) {
		Wifi_TxRaw(data,datalen);
		return 1;
	}
	arm7qlen=0;
	j=(datalen+1)>>1;
	if(j>1024) return 0;
	for(i=0;i<j;i++) arm7q[i]=data[i];
	arm7qlen=datalen;
	return 1;
}

void Wifi_Intr_StartTx() {

}

int Wifi_CheckTxBuf(s32 offset) {
	offset+=wifi_tx_buffer->bufIn;
	if(offset>=WIFI_RXBUFFER_SIZE/2) offset-=WIFI_RXBUFFER_SIZE/2;
	return wifi_tx_buffer->bufData[offset];
}

// non-wrapping function.
int Wifi_CopyFirstTxData(s32 macbase) {
	int seglen, readbase,max, packetlen,length;
	packetlen=Wifi_CheckTxBuf(5);
	readbase=wifi_tx_buffer->bufIn;
	length = (packetlen+12-4+1)/2;
	max=wifi_tx_buffer->bufOut-wifi_tx_buffer->bufIn;
	if(max<0) max+=WIFI_RXBUFFER_SIZE/2;
	if(max<length) return 0;
	while(length>0) {
		seglen=length;
		if(readbase+seglen>WIFI_RXBUFFER_SIZE/2) seglen=WIFI_RXBUFFER_SIZE/2-readbase;
		length-=seglen;
		while(seglen--) { WIFI_REG(0x4000+macbase)=wifi_tx_buffer->bufData[readbase++]; macbase+=2; }
		if(readbase>=WIFI_RXBUFFER_SIZE/2) readbase-=WIFI_RXBUFFER_SIZE/2;
	}
	wifi_tx_buffer->bufIn=readbase;
	wifi_tx_buffer->found++;
	wifi_tx_buffer->total += packetlen+12-4;

	//WifiData->stats[WSTAT_TXPACKETS]++;
	//WifiData->stats[WSTAT_TXBYTES]+=packetlen+12-4;
	//WifiData->stats[WSTAT_TXDATABYTES]+=packetlen-4;

	return packetlen;
}

static int tx_count = 0;
void Wifi_Intr_TxEnd() { // assume that we can now tx something new.
	wifi->stats_debug=((WIFI_REG(0xA8)&0x8000)|(WIFI_REG(0xB6)&0x7FFF));
	if(!Wifi_TxCheck()) {
		return;
	}
	if(arm7qlen) {
		Wifi_TxRaw(arm7q, arm7qlen);
		//keepalive_time=0;
		arm7qlen=0;
		return;
	}

	if((wifi_tx_buffer->bufOut!=wifi_tx_buffer->bufIn)) {// && (!(WifiData->curReqFlags&WFLAG_REQ_APCONNECT) || WifiData->authlevel==WIFI_AUTHLEVEL_ASSOCIATED)) {
		if(Wifi_CopyFirstTxData(0)) {
			/*keepalive_time=0;
			if(WIFI_REG(0x4008)==0) { WIFI_REG(0x4008)=WifiData->maxrate7; } // if rate dne, fill it in.
			if(WIFI_REG(0x400C)&0x4000) { // wep is enabled, fill in the IV.
				WIFI_REG(0x4024) = (W_RANDOM ^ (W_RANDOM<<7) ^ (W_RANDOM<<15))&0xFFFF;
				WIFI_REG(0x4026) = ((W_RANDOM ^ (W_RANDOM>>7))&0xFF) | (WifiData->wepkeyid7<<14);
			}*/
			//	WIFI_REG(0xB8)=0x0001;
			WIFI_REG(0x2C)=0x0707;
			WIFI_REG(WIFI_TXLOC3)=0x8000;
			WIFI_REGISTER(WIFI_TXCNT)=0x0008;
		} else {
			wifi_tx_buffer->lost++;
		}
	}
}

void Wifi_Intr_TBTT() {
	//if(WIFI_REG(WIFI_TXLOC3)&0x8000) {
	//	WIFI_REG(WIFI_TXCNT)=0x0008;
	//}
}

void Wifi_Intr_DoNothing() {
}

int Wifi_QueueRxMacData(u32 base, u32 len) {
	int buflen, temp,macofs, tempout;
	macofs=0;
	buflen=(wifi_rx_buffer->bufIn-wifi_rx_buffer->bufOut-1)*2;
	if(buflen<0) buflen += WIFI_RXBUFFER_SIZE;
	if(buflen<len) { wifi_rx_buffer->lost++; return 0; }
	wifi_rx_buffer->found++;
	wifi_rx_buffer->total += len;
	temp=WIFI_RXBUFFER_SIZE-(wifi_rx_buffer->bufOut*2);
	tempout=wifi_rx_buffer->bufOut;
	if(len>temp) {
		Wifi_MACCopy((u16*)wifi_rx_buffer->bufData+tempout,base,macofs,temp);
		macofs+=temp;
		len-=temp;
		tempout=0;
	}
	Wifi_MACCopy((u16*)wifi_rx_buffer->bufData+tempout,base,macofs,len);
	tempout+=len/2;
	if(tempout>=(WIFI_RXBUFFER_SIZE/2)) tempout-=(WIFI_RXBUFFER_SIZE/2);
	wifi_rx_buffer->bufOut=tempout;
	fifoSendValue32(FIFO_DSWIFI2, 0);
	//if(synchandler) synchandler();
	return 1;
}

int Wifi_ProcessReceivedFrame(int macbase, int framelen) {
	u8 data[512];
	int datalen = framelen;
	FRAME_HEADER *header = (FRAME_HEADER *)data;
	int headlen = (sizeof(*header)+1)&(~1);
	AUTHFRAME_HEADER *auth_head;// = (AUTHFRAME_HEADER *)frame;
	//u32 *iv = (u32 *)(auth_head+1);;
	AUTHFRAME_DATA *auth_data;// = (AUTHFRAME_DATA *)(iv+1);

	Wifi_MACCopy((u16 *)data,macbase,0,headlen);

	switch(header->fc.type) {
	case 0x0: //management
		if(datalen > 512) datalen = 512; 
		Wifi_MACCopy((u16 *)(data+headlen),macbase,headlen,framelen-headlen);
		wifi->stats_recv[header->fc.subType%16]++;
		switch(header->fc.subType) {
		case 0x0:
			if(wifi->host_state == WIFI_HOST_STATE_STARTED) {
				hw_process_assoc_request(data,datalen);
			}
			break;
		case 0x1: //assoc response
			wifi_process_assoc_response_frame(data,datalen);
			break;
		case 0x4: //probe request
			if(wifi->host_state == WIFI_HOST_STATE_STARTED) {
				hw_process_probe_request(data,datalen);
			}
			break;
		case 0x5: //probe response
			wifi_process_probe_response_frame(data,datalen);
			break;
		case 0x8: //beacon
			wifi_process_beacon_frame(data,datalen);
			break;
		case 0xB: //authentication
			auth_head = (AUTHFRAME_HEADER *)data;
			auth_data = (AUTHFRAME_DATA *)(auth_head+1);
			if(auth_data->transactionSequence&1) {
				if(wifi->host_state == WIFI_HOST_STATE_STARTED) {
					hw_process_auth_request(data,datalen);
				}
			} else {
				wifi_process_auth_frame(data,datalen);
			}
			break;
		}
		break;
	case 2: //data
		Wifi_QueueRxMacData(macbase,framelen);
		wifi->data_recv++;
		break;
	}

	return 0;
}

void Wifi_Intr_RxEnd() {
	static volatile int in_process = 0;
	u32 base;
	int packetlen;
	int full_packetlen;
	int cut, temp;
	int cs=enterCriticalSection();

	if(in_process) {
		//siprintf(debug_buffer,"re-entry               ");
		leaveCriticalSection(cs);
		return;
	}
	in_process = 1;
	cut=0;

	while(WIFI_REG(WIFI_RXWRITECSR)!=WIFI_REG(WIFI_RXREADCSR)) {
		base = WIFI_REG(WIFI_RXREADCSR)<<1;
		packetlen=Wifi_MACRead((u32)base,(u32)8);
		full_packetlen=12+((packetlen+3)&(~3));

		temp=Wifi_ProcessReceivedFrame(base,full_packetlen); // returns packet type

		base += full_packetlen;
		if(base>=(WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)) base -= ((WIFI_REG(WIFI_RXRANGE_END)&0x1FFE)-(WIFI_REG(WIFI_RXRANGE_BEGIN)&0x1FFE));
		WIFI_REG(WIFI_RXREADCSR)=base>>1;

		if(cut++>5) break;
	}
	leaveCriticalSection(cs);
	in_process = 0;
}

void hw_interrupt1() {
	int wIF;

	wifi_irq_counter++;

	do {
		REG_IF=0x01000000; // now that we've cleared the wireless IF, kill the bit in regular IF.
		wIF= W_IE & W_IF;
		if(wIF& 0x0001) { W_IF=0x0001;  Wifi_Intr_DoNothing();  } // 0) Rx End
		if(wIF& 0x0002) { W_IF=0x0002;  Wifi_Intr_DoNothing();  } // 1) Tx End
		if(wIF& 0x0004) { W_IF=0x0004;  Wifi_Intr_DoNothing();  } // 2) Rx Cntup
		if(wIF& 0x0008) { W_IF=0x0008;  Wifi_Intr_DoNothing();  } // 3) Tx Err
		if(wIF& 0x0010) { W_IF=0x0010;  Wifi_Intr_DoNothing();  } // 4) Count Overflow
		if(wIF& 0x0020) { W_IF=0x0020;  Wifi_Intr_DoNothing();  } // 5) AckCount Overflow
		if(wIF& 0x0040) { W_IF=0x0040;  Wifi_Intr_DoNothing();  } // 6) Start Rx
		if(wIF& 0x0080) { W_IF=0x0080;  Wifi_Intr_DoNothing();  } // 7) Start Tx
		if(wIF& 0x0100) { W_IF=0x0100;  Wifi_Intr_DoNothing();  } // 8) 
		if(wIF& 0x0200) { W_IF=0x0200;  Wifi_Intr_DoNothing();  } // 9)
		if(wIF& 0x0400) { W_IF=0x0400;  Wifi_Intr_DoNothing();  } //10)
		if(wIF& 0x0800) { W_IF=0x0800;  Wifi_Intr_DoNothing();  } //11) RF Wakeup
		if(wIF& 0x1000) { W_IF=0x1000;  Wifi_Intr_DoNothing();  } //12) MP End
		if(wIF& 0x2000) { W_IF=0x2000;  Wifi_Intr_DoNothing();  } //13) ACT End
		if(wIF& 0x4000) { W_IF=0x4000;  Wifi_Intr_DoNothing();  } //14) TBTT
		if(wIF& 0x8000) { W_IF=0x8000;  Wifi_Intr_DoNothing();  } //15) PreTBTT
		wIF= W_IE & W_IF;
	} while(wIF);

}
void hw_interrupt2() {
	int wIF;

	wifi_irq_counter++;

	do {
		REG_IF=0x01000000; // now that we've cleared the wireless IF, kill the bit in regular IF.
		wIF= W_IE & W_IF;
		if(wIF& 0x0001) { W_IF=0x0001;  Wifi_Intr_RxEnd();  } // 0) Rx End
		if(wIF& 0x0002) { W_IF=0x0002;  Wifi_Intr_TxEnd();  } // 1) Tx End
		if(wIF& 0x0004) { W_IF=0x0004;  Wifi_Intr_DoNothing();  } // 2) Rx Cntup
		if(wIF& 0x0008) { W_IF=0x0008;  Wifi_Intr_DoNothing();  } // 3) Tx Err
		if(wIF& 0x0010) { W_IF=0x0010;  Wifi_Intr_CntOverflow();  } // 4) Count Overflow
		if(wIF& 0x0020) { W_IF=0x0020;  Wifi_Intr_CntOverflow();  } // 5) AckCount Overflow
		if(wIF& 0x0040) { W_IF=0x0040;  Wifi_Intr_DoNothing();  } // 6) Start Rx
		if(wIF& 0x0080) { W_IF=0x0080;  Wifi_Intr_StartTx();  } // 7) Start Tx
		if(wIF& 0x0100) { W_IF=0x0100;  Wifi_Intr_DoNothing();  } // 8) 
		if(wIF& 0x0200) { W_IF=0x0200;  Wifi_Intr_DoNothing();  } // 9)
		if(wIF& 0x0400) { W_IF=0x0400;  Wifi_Intr_DoNothing();  } //10)
		if(wIF& 0x0800) { W_IF=0x0800;  Wifi_Intr_DoNothing();  } //11) RF Wakeup
		if(wIF& 0x1000) { W_IF=0x1000;  Wifi_Intr_DoNothing();  } //12) MP End
		if(wIF& 0x2000) { W_IF=0x2000;  Wifi_Intr_DoNothing();  } //13) ACT End
		if(wIF& 0x4000) { W_IF=0x4000;  Wifi_Intr_TBTT();  } //14) TBTT
		if(wIF& 0x8000) { W_IF=0x8000;  Wifi_Intr_DoNothing();  } //15) PreTBTT
		wIF= W_IE & W_IF;
	} while(wIF);

}

enum WIFI_STATS {
	// software stats
	WSTAT_RXQUEUEDPACKETS, // number of packets queued into the rx fifo
	WSTAT_TXQUEUEDPACKETS, // number of packets queued into the tx fifo
	WSTAT_RXQUEUEDBYTES, // number of bytes queued into the rx fifo
	WSTAT_TXQUEUEDBYTES, // number of bytes queued into the tx fifo
	WSTAT_RXQUEUEDLOST, // number of packets lost due to space limitations in queuing
	WSTAT_TXQUEUEDREJECTED, // number of packets rejected due to space limitations in queuing
	WSTAT_RXPACKETS,
	WSTAT_RXBYTES,
	WSTAT_RXDATABYTES,
	WSTAT_TXPACKETS,
	WSTAT_TXBYTES,
	WSTAT_TXDATABYTES,
	WSTAT_ARM7_UPDATES,
	WSTAT_DEBUG,
	// harware stats (function mostly unknown.)
	WSTAT_HW_1B0,WSTAT_HW_1B1,WSTAT_HW_1B2,WSTAT_HW_1B3,WSTAT_HW_1B4,WSTAT_HW_1B5,WSTAT_HW_1B6,WSTAT_HW_1B7,	
	WSTAT_HW_1B8,WSTAT_HW_1B9,WSTAT_HW_1BA,WSTAT_HW_1BB,WSTAT_HW_1BC,WSTAT_HW_1BD,WSTAT_HW_1BE,WSTAT_HW_1BF,	
	WSTAT_HW_1C0,WSTAT_HW_1C1,WSTAT_HW_1C4,WSTAT_HW_1C5,
	WSTAT_HW_1D0,WSTAT_HW_1D1,WSTAT_HW_1D2,WSTAT_HW_1D3,WSTAT_HW_1D4,WSTAT_HW_1D5,WSTAT_HW_1D6,WSTAT_HW_1D7,	
	WSTAT_HW_1D8,WSTAT_HW_1D9,WSTAT_HW_1DA,WSTAT_HW_1DB,WSTAT_HW_1DC,WSTAT_HW_1DD,WSTAT_HW_1DE,WSTAT_HW_1DF,	

	NUM_WIFI_STATS
};

#define CNT_STAT_START WSTAT_HW_1B0
#define CNT_STAT_NUM 18
u16 count_ofs_list[CNT_STAT_NUM] = {
	0x1B0, 0x1B2, 0x1B4, 0x1B6, 0x1B8, 0x1BA, 0x1BC, 0x1BE, 0x1C0, 0x1C4, 0x1D0, 0x1D2, 0x1D4, 0x1D6, 0x1D8, 0x1DA, 0x1DC, 0x1DE
};
u32 stats[NUM_WIFI_STATS] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void Wifi_Intr_CntOverflow() {
	int i;
	int s,d;
	s=CNT_STAT_START;
	for(i=0;i<CNT_STAT_NUM;i++) {
		d=WIFI_REG(count_ofs_list[i]);
		stats[s++]+=(d&0xFF);
		stats[s++]+=((d>>8)&0xFF);
	}
}

void hw_interrupt() {
	int wIF;

	wifi_irq_counter++;

	do {
		REG_IF=0x01000000; // now that we've cleared the wireless IF, kill the bit in regular IF.
		wIF= W_IE & W_IF;
		if(wIF& 0x0001) { W_IF=0x0001;  Wifi_Intr_RxEnd();  } // 0) Rx End
		if(wIF& 0x0002) { W_IF=0x0002;  Wifi_Intr_TxEnd();  } // 1) Tx End
		if(wIF& 0x0004) { W_IF=0x0004;  Wifi_Intr_DoNothing();  } // 2) Rx Cntup
		if(wIF& 0x0008) { W_IF=0x0008;  Wifi_Intr_DoNothing();  } // 3) Tx Err
		if(wIF& 0x0010) { W_IF=0x0010;  Wifi_Intr_CntOverflow();  } // 4) Count Overflow
		if(wIF& 0x0020) { W_IF=0x0020;  Wifi_Intr_CntOverflow();  } // 5) AckCount Overflow
		if(wIF& 0x0040) { W_IF=0x0040;  Wifi_Intr_DoNothing();  } // 6) Start Rx
		if(wIF& 0x0080) { W_IF=0x0080;  Wifi_Intr_DoNothing();  } // 7) Start Tx
		if(wIF& 0x0100) { W_IF=0x0100;  Wifi_Intr_DoNothing();  } // 8) 
		if(wIF& 0x0200) { W_IF=0x0200;  Wifi_Intr_DoNothing();  } // 9)
		if(wIF& 0x0400) { W_IF=0x0400;  Wifi_Intr_DoNothing();  } //10)
		if(wIF& 0x0800) { W_IF=0x0800;  Wifi_Intr_DoNothing();  } //11) RF Wakeup
		if(wIF& 0x1000) { W_IF=0x1000;  Wifi_Intr_DoNothing();  } //12) MP End
		if(wIF& 0x2000) { W_IF=0x2000;  Wifi_Intr_DoNothing();  } //13) ACT End
		if(wIF& 0x4000) { W_IF=0x4000;  Wifi_Intr_DoNothing();  } //14) TBTT
		if(wIF& 0x8000) { W_IF=0x8000;  Wifi_Intr_DoNothing();  } //15) PreTBTT
		wIF= W_IE & W_IF;
	} while(wIF);

}

