#include <nds.h>
#include "hw.h"
#include "wifi_state.h"
#include <nds\arm7\serial.h>

int chdata_save5 = 0;

 void hw_set_mode(int wifimode) {
	if(wifimode>3 || wifimode<0) return;
	WIFI_REGISTER(WIFI_MODE_WEP) = (WIFI_REGISTER(WIFI_MODE_WEP) & 0xfff8) | wifimode;
}

void hw_set_wep_mode(int wepmode) {
	if(wepmode<0 || wepmode>7) return;
	if(wepmode==0) {
		WIFI_REGISTER(WIFI_RXWEPCNT)=0x0000;
	} else {
		WIFI_REGISTER(WIFI_RXWEPCNT)=0x8000;
	}
	if(wepmode==0) wepmode=1;
	WIFI_REGISTER(WIFI_MODE_WEP) = (WIFI_REGISTER(WIFI_MODE_WEP) & 0xFFC7) | (wepmode<<3);
}

void hw_set_wep_key(void * wepkey) {
	int i;
	for(i=0;i<16;i++) {
		W_WEPKEY0[i]=((u16 *)wepkey)[i];
		W_WEPKEY1[i]=((u16 *)wepkey)[i];
		W_WEPKEY2[i]=((u16 *)wepkey)[i];
		W_WEPKEY3[i]=((u16 *)wepkey)[i];
	}
}

void hw_set_beacon_period(int beacon_period) {
	if(beacon_period<0x10 || beacon_period>0x3E7) return;
	WIFI_REGISTER(WIFI_BEACONPERIOD) = beacon_period;
}
	
void hw_set_preamble_type(int preamble_type) {
	if (preamble_type>1 || preamble_type<0) return;
	WIFI_REGISTER(WIFI_TXPREAMBLE) = (WIFI_REGISTER(WIFI_TXPREAMBLE) & 0xFFBF) | (preamble_type<<6);
}

void hw_disable_temp_power_save() {
	WIFI_REGISTER(WIFI_PSCNT) &= ~2;
	WIFI_REGISTER(0x8048) = 0;
}

int wifi_channel = 1;
void hw_set_channel(int channel) {
	int i,n,l;
	if(channel<1 || channel>13) return;
	wifi_channel = channel;
	channel-=1;

	switch(hw_read_flash_byte(0x40)) {
	case 2:
	case 5:
		hw_rf_write(hw_read_flash_bytes(0xf2+channel*6,3));
		hw_rf_write(hw_read_flash_bytes(0xf5+channel*6,3));

		swiDelay( 12583 ); // 1500 us delay

		if(chdata_save5 & 0x10000)
		{
			if(chdata_save5 & 0x8000) break;
			n = hw_read_flash_byte(0x154+channel);
			hw_rf_write( chdata_save5 | ((n&0x1F)<<10) );
		} else {
			hw_bb_write(0x1E, hw_read_flash_byte(0x146+channel));
		}

		break;
	case 3:
		n=hw_read_flash_byte(0x42);
		n+=0xCF;
		l=hw_read_flash_byte(n-1);
		for(i=0;i<l;i++) {
			hw_bb_write(hw_read_flash_byte(n),hw_read_flash_byte(n+channel+1));
			n+=15;
		}
		for(i=0;i<hw_read_flash_byte(0x43);i++) {
			hw_rf_write( (hw_read_flash_byte(n)<<8) | hw_read_flash_byte(n+channel+1) | 0x050000 );
			n+=15;
		}

		swiDelay( 12583 ); // 1500 us delay

		break;
	default:
		break;
	}
	//WifiData->curChannel=channel+1;
}

/*#########################################################################################
*/

void hw_stop() {
	int tIME=REG_IME;

	REG_IME=0;
	WIFI_REG(0x8004) = 0;
	WIFI_REG(0x80EA) = 0;
	WIFI_REG(0x80E8) = 0;
	WIFI_REG(0x8008) = 0;
	WIFI_REG(0x800A) = 0;

	WIFI_REG(0x80AC) = 0xFFFF;
	WIFI_REG(0x80B4) = 0xFFFF;
	//	Wifi_Shutdown();
	REG_IME=tIME;
}

void hw_shutdown() {
	int a;
	if(hw_read_flash_byte(0x40)==2) {
		hw_rf_write(0xC008);
	}
	a=hw_bb_read(0x1E);
	hw_bb_write(0x1E,a|0x3F);
	WIFI_REG(0x168)=0x800D;
	WIFI_REG(0x36)=1;
}

int RF_Reglist[] = { 0x146, 0x148, 0x14A, 0x14C, 0x120, 0x122, 0x154, 0x144, 0x130, 0x132, 0x140, 0x142, 0x38, 0x124, 0x128, 0x150 };

void hw_rf_init() {
	int i,j;
	int channel_extrabits;
	int numchannels;
	int channel_extrabytes;
	int temp;
	for(i=0;i<16;i++) {
		WIFI_REG(RF_Reglist[i])=hw_read_flash_short(0x44+i*2);
	}
	numchannels=hw_read_flash_byte(0x42);
	channel_extrabits=hw_read_flash_byte(0x41);
	channel_extrabytes=(channel_extrabits+7)/8;
	WIFI_REG(0x184)=((channel_extrabits>>7)<<8) | (channel_extrabits&0x7F);
	j=0xCE;
	if(hw_read_flash_byte(0x40)==3) {
		for(i=0;i<numchannels;i++) {
			hw_rf_write(hw_read_flash_byte(j++)|(i<<8)|0x50000);
		}
	} else if(hw_read_flash_byte(0x40)==2) {
		for(i=0;i<numchannels;i++) {
			temp = hw_read_flash_bytes(j,channel_extrabytes);
			hw_rf_write(temp);
			j+=channel_extrabytes;
			if( (temp>>18)==9 ) {
				chdata_save5 = temp&(~0x7C00);
			}
		}
	} else {
		for(i=0;i<numchannels;i++) {
			hw_rf_write(hw_read_flash_bytes(j,channel_extrabytes));
			j+=channel_extrabytes;
		}
	}
}

void hw_bb_init() {
	int i;
	WIFI_REG(0x160)=0x0100;
	for(i=0;i<0x69;i++) {
		hw_bb_write(i,hw_read_flash_byte(0x64+i));
	}
}

extern volatile int wifi_vblank_counter;

void hw_wakeup() {
	u32 i;
	WIFI_REG(0x8036)=0;

	swiDelay( 67109 ); // 8ms delay

	WIFI_REG(0x8168)=0;

	i=hw_bb_read(1);
	hw_bb_write(1,i&0x7f);
	hw_bb_write(1,i);


	swiDelay( 335544 ); // 40ms delay

	hw_rf_init();
}

// 22 entry list
int MAC_Reglist[] = { 0x04, 0x08, 0x0A, 0x12, 0x10, 0x254, 0xB4, 0x80, 0x2A, 0x28, 0xE8, 0xEA, 0xEE, 0xEC, 0x1A2, 0x1A0, 0x110, 0xBC, 0xD4, 0xD8, 0xDA, 0x76 };
int MAC_Vallist[] = { 0, 0, 0, 0, 0xffff, 0, 0xffff, 0, 0, 0, 0, 0, 1, 0x3F03, 1, 0, 0x0800, 1, 3, 4, 0x0602, 0};
void hw_mac_init() {
	int i;
	for(i=0;i<22;i++) {
		WIFI_REG(MAC_Reglist[i]) = MAC_Vallist[i];
	}
}


int crc16_slow(u8 * data, int length) {
	int i,j, crc;
	crc=0x0000;
	for(i=0;i<length;i++) {
		crc ^=data[i];
		for(j=0;j<8;j++) {
			if((crc)&1) crc = (crc>>1)^0xA001; else crc=crc>>1;
		}
	}
	crc &=0xFFFF;
	return crc;
}

void GetWfcSettings() {
	u8 data[256];
	int i,n, c;
	unsigned long s;
	c=0;
	u32 wfcBase = hw_read_flash_bytes(0x20, 2) * 8 - 0x400;
	//for(i=0;i<3;i++) WifiData->wfc_enable[i]=0;
	for(i=0;i<6;i++) {
		//readFirmware( wfcBase +(i<<8),(char *)(&wifi->dummy[i*256]),256);
		//readFirmware( wfcBase +(i<<8),(char *)data,256);
		//break;
		// check for validity (crc16)
		/*if(crc16_slow(data,256)==0x0000 && data[0xE7]==0x00) { // passed the test
			WifiData->wfc_enable[c] = 0x80 | (data[0xE6]&0x0F);
			WifiData->wfc_ap[c].channel=0;
			for(n=0;n<6;n++) WifiData->wfc_ap[c].bssid[n]=0;
			for(n=0;n<16;n++) WifiData->wfc_wepkey[c][n]=data[0x80+n];
			for(n=0;n<32;n++) WifiData->wfc_ap[c].ssid[n]=data[0x40+n];
			for(n=0;n<32;n++) if(!data[0x40+n]) break;
			WifiData->wfc_ap[c].ssid_len=n;
			WifiData->wfc_config[c][0]=((unsigned long *)(data+0xC0))[0];
			WifiData->wfc_config[c][1]=((unsigned long *)(data+0xC0))[1];
			WifiData->wfc_config[c][3]=((unsigned long *)(data+0xC0))[2];
			WifiData->wfc_config[c][4]=((unsigned long *)(data+0xC0))[3];
			s=0;
			for(n=0;n<data[0xD0];n++) {
				s |= 1<<(31-n);
			}
			s= (s<<24) | (s>>24) | ((s&0xFF00)<<8) | ((s&0xFF0000)>>8); // htonl
			WifiData->wfc_config[c][2]=s;
			c++;
		}*/
	}
}

void hw_init() {
#define REG_GPIOWTF (*(vu16*)0x4004C04)
	if (isDSiMode() & !(REG_GPIOWTF & BIT(8))) {
		REG_GPIOWTF = (REG_GPIOWTF&1) | BIT(8);
		swiDelay(0xA3A47); //5ms
	}


	POWERCNT7 |= 2; // enable power for the wifi
	WIFI_WAITCR = 0x30; // ???

	hw_init_flash_data();

	// reset/shutdown wifi:
	WIFI_REG(WIFI_MODE_RST)=0xffff;
	hw_stop();
	hw_shutdown(); // power off wifi


	int i;
	for(i=0x4000;i<0x6000;i+=2) WIFI_REG(i)=0;



	// load in the WFC data.
	GetWfcSettings();

	for(i=0;i<3;i++) {
		wifi->mac[i]=hw_read_flash_short(0x36+i*2);
	}

	W_IE=0;
	hw_wakeup();

	hw_mac_init();
	hw_rf_init();
	hw_bb_init();

	// Set Default Settings
	W_MACADDR[0]=wifi->mac[0];
	W_MACADDR[1]=wifi->mac[1];
	W_MACADDR[2]=wifi->mac[2];

	W_RETRLIMIT=7;
	hw_set_mode(1);
	hw_set_wep_mode(WEPMODE_NONE);


	hw_set_channel(1);

	hw_bb_write(0x13, 0x00);
	hw_bb_write(0x35, 0x1F);

}

void hw_tx_setup() {
	/*	switch(WIFI_REG(0x8006)&7) {
	case 0: //
	// 4170,  4028, 4000
	// TxqEndData, TxqEndManCtrl, TxqEndPsPoll
	WIFI_REG(0x4024)=0xB6B8;
	WIFI_REG(0x4026)=0x1D46;
	WIFI_REG(0x416C)=0xB6B8;
	WIFI_REG(0x416E)=0x1D46;
	WIFI_REG(0x4790)=0xB6B8;
	WIFI_REG(0x4792)=0x1D46;
	WIFI_REG(0x80AE) = 1;
	break;
	case 1: //
	// 4AA0, 4958, 4334
	// TxqEndData, TxqEndManCtrl, TxqEndBroadCast
	// 4238, 4000
	WIFI_REG(0x4234)=0xB6B8;
	WIFI_REG(0x4236)=0x1D46;
	WIFI_REG(0x4330)=0xB6B8;
	WIFI_REG(0x4332)=0x1D46;
	WIFI_REG(0x4954)=0xB6B8;
	WIFI_REG(0x4956)=0x1D46;
	WIFI_REG(0x4A9C)=0xB6B8;
	WIFI_REG(0x4A9E)=0x1D46;
	WIFI_REG(0x50C0)=0xB6B8;
	WIFI_REG(0x50C2)=0x1D46;
	//...
	break;
	case 2:
	// 45D8, 4490, 4468
	// TxqEndData, TxqEndManCtrl, TxqEndPsPoll

	WIFI_REG(0x4230)=0xB6B8;
	WIFI_REG(0x4232)=0x1D46;
	WIFI_REG(0x4464)=0xB6B8;
	WIFI_REG(0x4466)=0x1D46;
	WIFI_REG(0x448C)=0xB6B8;
	WIFI_REG(0x448E)=0x1D46;
	WIFI_REG(0x45D4)=0xB6B8;
	WIFI_REG(0x45D6)=0x1D46;
	WIFI_REG(0x4BF8)=0xB6B8;
	WIFI_REG(0x4BFA)=0x1D46;
	*/
	WIFI_REG(0x80AE)=0x000D;
	//	}
}

void hw_rx_setup() {
	WIFI_REG(0x8030) = 0x8000;
	/*	switch(WIFI_REG(0x8006)&7) {
	case 0:
	WIFI_REG(0x8050) = 0x4794;
	WIFI_REG(0x8056) = 0x03CA;
	// 17CC ?
	break;
	case 1:
	WIFI_REG(0x8050) = 0x50C4;
	WIFI_REG(0x8056) = 0x0862;
	// 0E9C ?
	break;
	case 2:
	WIFI_REG(0x8050) = 0x4BFC;
	WIFI_REG(0x8056) = 0x05FE;
	// 1364 ?
	break;
	case 3:
	WIFI_REG(0x8050) = 0x4794;
	WIFI_REG(0x8056) = 0x03CA;
	// 17CC ?
	break;
	}
	*/
	WIFI_REG(0x8050) = 0x4C00;
	WIFI_REG(0x8056) = 0x0600;

	WIFI_REG(0x8052) = 0x5F60;
	WIFI_REG(0x805A) = (WIFI_REG(0x8050)&0x3FFF)>>1;
	WIFI_REG(0x8062) = 0x5F5E;
	WIFI_REG(0x8030) = 0x8001;
}

void hw_start() {
	int i, tIME;
	tIME=REG_IME;
	REG_IME=0;
	hw_stop();

	//	Wifi_WakeUp();

	WIFI_REG(0x8032) = 0x8000;
	WIFI_REG(0x8134) = 0xFFFF;
	WIFI_REG(0x802A) = 0;
	W_AIDS           = 0;
	WIFI_REG(0x80E8) = 1;
	WIFI_REG(0x8038) = 0x0000;
	WIFI_REG(0x20) = 0x0000;
	WIFI_REG(0x22) = 0x0000;
	WIFI_REG(0x24) = 0x0000;

	hw_tx_setup();
	hw_rx_setup();

	WIFI_REG(0x8030) = 0x8000;
	/*
	switch(WIFI_REG(0x8006)&7) {
	case 0: // infrastructure mode?
	W_IF=0xFFFF;
	W_IE=0x003F;
	WIFI_REG(0x81AE)=0x1fff;
	//WIFI_REG(0x81AA)=0x0400;
	WIFI_REG(0x80D0)=0xffff;
	WIFI_REG(0x80E0)=0x0008;
	WIFI_REG(0x8008)=0;
	WIFI_REG(0x800A)=0;
	WIFI_REG(0x80E8)=0;
	WIFI_REG(0x8004)=1;
	//SetStaState(0x40);
	break;
	case 1: // ad-hoc mode? -- beacons are required to be created!
	W_IF=0xFFF;
	W_IE=0x703F;
	WIFI_REG(0x81AE)=0x1fff;
	WIFI_REG(0x81AA)=0; // 0x400
	WIFI_REG(0x80D0)=0x0301;
	WIFI_REG(0x80E0)=0x000D;
	WIFI_REG(0x8008)=0xE000;
	WIFI_REG(0x800A)=0;
	WIFI_REG(0x8004)=1;
	//??
	WIFI_REG(0x80EA)=1;
	WIFI_REG(0x80AE)=2;
	break;
	case 2: // DS comms mode?
	*/
	W_IF=0xFFFF;
	//W_IE=0xE03F;
	W_IE=0x40B3;
	WIFI_REG(0x81AE)=0x1fff;
	WIFI_REG(0x81AA)=0; //0x68
	W_BSSID[0]=wifi->mac[0];
	W_BSSID[1]=wifi->mac[1];
	W_BSSID[2]=wifi->mac[2];
	WIFI_REG(0x80D0)=0x0001;//0x0501;//0x0581; // 0x181
	WIFI_REG(0x80E0)=0x000F;//B;
	WIFI_REG(0x8008)=0;
	WIFI_REG(0x800A)=0;
#if 1
	WIFI_REG(0x8004)=1;
#endif
	WIFI_REG(0x80E8)=1;
	WIFI_REG(0x80EA)=1;
	//SetStaState(0x20);
	/*
	break;
	case 3:
	case 4:
	break;
	}
	*/

	WIFI_REG(0x8048)=0x0000;
	hw_disable_temp_power_save();
	//WIFI_REG(0x80AE)=0x0002;
	W_POWERSTATE |= 2;
	WIFI_REG(0xAC) = 0xFFFF;
	i=0xFA0;
	while(i!=0 && !(WIFI_REG(0x819C)&0x80)) i--;
	REG_IME=tIME;
}
