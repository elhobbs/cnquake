#ifdef ARM9
#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include "wifi_state.h"
#include "802.11.h"
#include "802.2.h"
#endif

#include "fnet.h" /* FNET API */ 
#include "fnet_cpu.h"
#include "fnet_eth_prv.h"
#include "fnet_error.h"
#include "fnet_debug.h"
#include "fnet_isr.h"
#include "fnet_prot.h"
#include "fnet_arp.h"
#include "fnet_timer_prv.h"
#include "fnet_loop.h"
#include "fnet_poll.h"
#include "fnet_stack.h"

#include "fnet_stdlib.h"
#include <stdlib.h>

int fnet_printf( const char *format, ... );

static unsigned char *fnet_heap_global = 0;//[FNET_CFG_HEAP_SIZE];

#ifdef ARM9
volatile WIFI_MAINSTRUCT *wifi = 0;
WIFI_BUFFER *wifi_rx_buffer = 0;
WIFI_BUFFER *wifi_tx_buffer = 0;

ACCESSPOINTSCANLIST *scanlist_cached = 0;
ACCESSPOINTSCANLIST *scanlist = 0;
volatile WIFI_MSG	wifi_msg;

char debug_buffer_cached[512] = {0,0,0,0};
char *debug_buffer;


ACCESSPOINTSCANLIST *alloc_APSCANLIST(int count) {
	ACCESSPOINTSCANLIST *list = (ACCESSPOINTSCANLIST *)malloc(sizeof(*scanlist) + (count-1) * sizeof(ACCESSPOINT));
	if(list) {
		list->count = 0;
		list->maxcount = 25;
		memset(list->ap,0,sizeof(ACCESSPOINT)*count);
	}
	return list;
}

#define DIR_LIST_COUNT 20
void dsnifi_choose_draw(ACCESSPOINTSCANLIST *list,int pos)
{
	int i;
	int count = list->count;
	int start = (pos/DIR_LIST_COUNT)*DIR_LIST_COUNT;
	int end = start + DIR_LIST_COUNT;

	if(end > count)
		end = count;

	// Clear the screen
	iprintf ("\x1b[2J");

	iprintf("Choose an Access Point\nA to select or B to cancel");
	// Move to 2nd row
	iprintf ("\x1b[2;0H");
	// Print line of dashes
	iprintf ("--------------------------------");
	for(i = start;i < end;i++)
	{
		// Set row
		iprintf ("\x1b[%d;0H", i-start + 3);
		iprintf (" [%s]", &list->ap[i].ssid[1]);
	}
	if(end < count)
	{
		// Set row
		iprintf ("\x1b[%d;0H", i-start + 3);
		iprintf (" more...");
	}
}

int dsnifi_choose_ap(ACCESSPOINTSCANLIST *list)
{
	int count = 0;
	int i;
	int pos = 0;
	int pressed = 0;
	int selected = -1;

	ds_choose_draw(list,pos);

	while (true) {
		// Clear old cursors
		for (i = 0; i < DIR_LIST_COUNT; i++) {
			iprintf ("\x1b[%d;0H ", i+3);
		}
		// Show cursor
		iprintf ("\x1b[%d;0H*", (pos%DIR_LIST_COUNT) + 3);
		
		// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing else to do
		do {
			scanKeys();
			pressed = keysDown();
			swiWaitForVBlank();
			if(count != list->count) {
				count = list->count;
				dsnifi_choose_draw(list,pos);
			}
		} while (!pressed);
		
	
		if (pressed & KEY_UP)
		{
			pos -= 1;
			if (pos < 0)
			{
				pos = count - 1;		// Wrap around to bottom of list
				dsnifi_choose_draw(list,pos);
			}
			else if(pos % DIR_LIST_COUNT == (DIR_LIST_COUNT-1))
			{
				dsnifi_choose_draw(list,pos);
			}
		}
		if (pressed & KEY_DOWN)
		{
			pos += 1;
			if (pos >= count)
			{
				pos = 0;		// Wrap around to top of list
				dsnifi_choose_draw(list,pos);
			}
			else if(pos % DIR_LIST_COUNT == 0)
			{
				dsnifi_choose_draw(list,pos);
			}
		}
		
		
		if (pressed & KEY_A) {
			// Clear the screen
			iprintf ("\x1b[2J");
			iprintf("selected %s\n",&list->ap[pos].ssid[1]);
			selected = pos;
			break;
		}
		
		if (pressed & KEY_B) {
			// Clear the screen
			iprintf ("\x1b[2J");
			iprintf("canceled AP selection\n");
			break;
		}
	}
	/*do {
		scanKeys();
		pressed = keysDown();
		swiWaitForVBlank();
	} while (!pressed);*/

	return selected;
}

void dsnifi_start_host() {
	wifi_msg.type = WIFI_MSG_HOST_START;
	DC_FlushAll();
	fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);
}

void dsnifi_connect_to_ap() {
	int selected_ap, result;

	wifi_msg.type = WIFI_MSG_SCAN;
	strcpy((char *)&scanlist->ssid[1],"");
	scanlist->ssid[0] = 0;
	scanlist->count = 0;
	wifi_msg.scanlist = scanlist_cached;
	DC_FlushAll();
	fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);
	while(!fifoCheckValue32(FIFO_DSWIFI));
	result = fifoGetValue32(FIFO_DSWIFI);
	printf("scan finished %d\n",result);

	selected_ap = dsnifi_choose_ap(scanlist);
			
	if(selected_ap != -1) {
		ACCESSPOINT *ap = &scanlist->ap[selected_ap];
		consoleClear();
		ap->wepkeyid = 0;
		ap->wepmode = 0;
		wifi_msg.type = WIFI_MSG_CONNECT;
		wifi_msg.ap = &scanlist_cached->ap[selected_ap];
		iprintf("connecting to: %s on %d\n",&ap->ssid[1],ap->channel);
		DC_FlushAll();
		fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);
		while(!fifoCheckValue32(FIFO_DSWIFI)) {
			//iprintf("\x1b[2;0H%s\n",debug_buffer);
		}
		result = fifoGetValue32(FIFO_DSWIFI);
		iprintf("wifi.state == %d\n",result);
	}
}

u16 Wifi_RxReadOffset(s32 base, s32 offset) {
	base+=offset;
	if(base>=(WIFI_RXBUFFER_SIZE/2)) base -= (WIFI_RXBUFFER_SIZE/2);
	return wifi_rx_buffer->bufData[base];
}

int Wifi_RxRawReadPacket(s32 base, s32 readlength, u16 * data) {
	int readlen,read_data;
	readlength= (readlength+1)/2;
	read_data=0;
	if(base>=(WIFI_RXBUFFER_SIZE/2)) base -= (WIFI_RXBUFFER_SIZE/2);
	while(readlength>0) {
		readlen=readlength;
		if(readlen>(WIFI_RXBUFFER_SIZE/2)-base) readlen=(WIFI_RXBUFFER_SIZE/2)-base;
		readlength-=readlen;
		read_data+=readlen;
		while(readlen>0) {
			*(data++) = wifi_rx_buffer->bufData[base++];
			readlen--;
		}
		base=0;
	}
	return read_data;
}

void wifi_rx_data() {
	int cnt;
	int base, base2, len, fulllen;
    fnet_netbuf_t * nb=0;
	struct {
		DATA_FRAME_HEADER data;
		WIFI_LLC llc;
		WIFI_SNAP snap;
	} header;

	if(!wifi_rx_buffer) return;

	while(wifi_rx_buffer->bufIn!=wifi_rx_buffer->bufOut) {
		base = wifi_rx_buffer->bufIn;
		len=Wifi_RxReadOffset(base,4);
		fulllen=((len+3)&(~3))+12;

		Wifi_RxRawReadPacket(base,sizeof(header),(u16 *)&header);
		//Con_Printf("R1 %d %x %x %x\n",len,header.llc.DSAP,header.llc.SSAP,header.llc.command);
		if(header.llc.DSAP == 0xAA &&
			header.llc.SSAP == 0xAA &&
			header.llc.command == 0x03 &&
			//header.snap.ether_type[0] == 0x08 &&
			//header.snap.ether_type[1] == 0x00 &&
			header.snap.org_code[0] == 0 &&
			header.snap.org_code[1] == 0 &&
			header.snap.org_code[2] == 0)
		{

			//do something
			nb = fnet_netbuf_new(len-sizeof(header)+12, FNET_TRUE);

			if(nb) {
				Wifi_RxRawReadPacket(base+(sizeof(header)/2),len-sizeof(header)+12,(u16 *)nb->data_ptr);
				//fnet_memcpy(nb->data_ptr, data_ptr, (unsigned int)len);
                /* Network-layer input.*/
                fnet_eth_prot_input( &fnet_eth0_if, nb, *((u16*)(header.snap.ether_type)) );
				//Con_Printf("R3: %d %04x\n",len-sizeof(header)+12, *((u16*)(header.snap.ether_type)) );
				//while(1);
			}
		}
		
		base+=fulllen/2;
		if(base>=(WIFI_RXBUFFER_SIZE/2)) base-=(WIFI_RXBUFFER_SIZE/2);
		wifi_rx_buffer->bufIn=base;

		if(cnt++>80) break;
	}
}

u32 Wifi_TxBufferWordsAvailable() {
	s32 size=wifi_tx_buffer->bufIn-wifi_tx_buffer->bufOut-1;
	if(size<0) size += WIFI_RXBUFFER_SIZE/2;
	return size;
}

void Wifi_TxBufferWrite(s32 start, s32 len, u16 * data) {
	int writelen;
	while(len>0) {
		writelen=len;
		if(writelen>(WIFI_RXBUFFER_SIZE/2)-start) writelen=(WIFI_RXBUFFER_SIZE/2)-start;
		len-=writelen;
		while(writelen) {
			wifi_tx_buffer->bufData[start++]=*(data++);
			writelen--;
		}
		start=0;
	}
}

void wifi_copy_mac(volatile void * dest, volatile void * src) {
	((u16 *)dest)[0]=((u16 *)src)[0];
	((u16 *)dest)[1]=((u16 *)src)[1];
	((u16 *)dest)[2]=((u16 *)src)[2];
}

void Con_Printf (char *fmt, ...);

void fnet_netbuf_to_wifi_tx_buffer( fnet_netbuf_t *nb, unsigned short type, const fnet_mac_addr_t dest_addr)
{
    //unsigned char *u_buf;
    fnet_netbuf_t *tmp_nb;
    long tot_len = 0, cur_len;
	s32 b1,base = wifi_tx_buffer->bufOut;
	b1 = base;
	struct {
		DATA_FRAME_HEADER data;
		WIFI_LLC llc;
		WIFI_SNAP snap;
	} header;

	if(wifi_tx_buffer == 0) return;
    //u_buf = data_ptr;

	//Con_Printf("T0: %02x:%02x:%02x:%02x:%02x:%02x\n",dest_addr[0],dest_addr[1],dest_addr[2],dest_addr[3],dest_addr[4],dest_addr[5]);
    tmp_nb = nb;

    /* This part is similar to the corresponding part in fnet_netbuf_copy */
    do
    {
        tot_len += tmp_nb->length;
        tmp_nb = tmp_nb->next;
    } while (tmp_nb);

	if((tot_len+sizeof(header)) > (Wifi_TxBufferWordsAvailable()*2)) {
		return;
	}

	//Con_Printf("T1: %4d\n",tot_len);

	/*if(type ==  FNET_ETH_TYPE_ARP) {
		int i;
		unsigned char *b = nb->data_ptr;
		for(i = 0;i<nb->length;i++) {
			printf("x%02x ",b[i]);
		}
		while(1);
	}*/


	memset(&header,0,sizeof(header));
	header.data.fc.type = 2;
	header.data.fc.toDS = wifi->ap.adhoc ? 0 : 1;
	header.llc.DSAP = 0xAA;
	header.llc.SSAP = 0xAA;
	header.llc.command = 3;
	header.snap.ether_type[1] = type & 0xff;
	header.snap.ether_type[0] = (type>>8) & 0xff;
	header.data.tx.tx_length = tot_len+sizeof(header)-12+4;
	header.data.tx.tx_rate = 0x0014;
	if(wifi->ap.adhoc) {
		wifi_copy_mac(header.data.A1,(u16*)dest_addr);
		wifi_copy_mac(header.data.A2,wifi->mac);
		wifi_copy_mac(header.data.A3,wifi->ap.bssid);
	} else {
		wifi_copy_mac(header.data.A1,wifi->ap.bssid);
		wifi_copy_mac(header.data.A2,wifi->mac);
		wifi_copy_mac(header.data.A3,(u16*)dest_addr);
	}

	//tot_len = (tot_len + 1)/2;
	Wifi_TxBufferWrite(base,sizeof(header)/2,(u16 *)&header);
	base += (sizeof(header)/2);
	if(base>=(WIFI_RXBUFFER_SIZE/2)) base -= WIFI_RXBUFFER_SIZE/2;

    tmp_nb = nb;

    do
    {
        /* Calculate the quantity of bytes we copy from the current net_buf*/
        cur_len = tmp_nb->length;
        /* and substract from the total quantity of bytes we copy */
        tot_len -= cur_len;

		Wifi_TxBufferWrite(base,(cur_len+1)/2,(u16 *)tmp_nb->data_ptr);

        base += (cur_len+1)/2; /* move the pointer for the next copy */
		if(base>=(WIFI_RXBUFFER_SIZE/2)) base -= WIFI_RXBUFFER_SIZE/2;

		tmp_nb = tmp_nb->next;   /* go to the next net_buf in the chain */     
    } while (tot_len > 0);
	wifi_tx_buffer->bufOut=base; // update fifo out pos, done sending packet.  

	wifi_msg.type = WIFI_MSG_TX;
	DC_FlushAll();
	fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);
	//Con_Printf("T2: %4d %08x %08x\n",header.data.tx.tx_length,wifi_tx_buffer->bufIn,wifi_tx_buffer->bufOut);
}


void wifi_from7(u32 value, void* data) {
//---------------------------------------------------------------------------------

	switch (value) {
		case 0:
			fnet_event_raise(FNET_EVENT_RX_DATA);
			break;
		default:
			break;
	}
}

int wifi_check_init() {
	return (wifi->initialized & 0x1);
}

int wifi_init() {
	int result;
	
	fifoSetValue32Handler(FIFO_DSWIFI2,  wifi_from7, 0);
	wifi = (WIFI_MAINSTRUCT *)malloc(sizeof(*wifi));
	memset((void *)wifi,0,sizeof(*wifi));
	wifi_msg.type = WIFI_MSG_INIT;
	wifi_msg.p = (void *)wifi;
	wifi = (volatile WIFI_MAINSTRUCT *)memUncached((void *)wifi);
	wifi_rx_buffer = (WIFI_BUFFER *)&wifi->rx_buffer;
	wifi_tx_buffer = (WIFI_BUFFER *)&wifi->tx_buffer;
	DC_FlushAll();
	fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);

	result = 0;
	//while(!fifoCheckValue32(FIFO_DSWIFI)) {
	while(!wifi_check_init()) {
		printf("result: %d\n",result++);
		swiWaitForVBlank();
	}
	//result = fifoGetValue32(FIFO_DSWIFI);
	printf("wifi init finished %d\n",wifi->initialized);
	
	scanlist = scanlist_cached = alloc_APSCANLIST(10);
	scanlist = (ACCESSPOINTSCANLIST *)memUncached(scanlist);

	debug_buffer = (char *)memUncached(debug_buffer_cached);
	*debug_buffer = 0;
	wifi_msg.debug_buffer = debug_buffer_cached;
	wifi_msg.type = WIFI_MSG_DEBUG_BUFFER;
	DC_FlushAll();
	fifoSendAddress(FIFO_DSWIFI, (void *)&wifi_msg);

	return result;
}

#endif

/************************************************************************
* NAME: fapp_dhcp_handler_updated
*
* DESCRIPTION: Event handler on new IP from DHCP client. 
************************************************************************/
static void fapp_dhcp_handler_updated( fnet_netif_desc_t netif, void *shl_desc )
{
}

/************************************************************************
* NAME: fapp_dhcp_handler_discover
*
* DESCRIPTION: Event handler on new IP from DHCP client. 
************************************************************************/
static void fapp_dhcp_handler_discover( fnet_netif_desc_t netif,void *shl_desc )
{
}

int fnet_init_global()
{
    struct fnet_init_params init_params;
	fnet_heap_global = (unsigned char *)malloc(FNET_CFG_HEAP_SIZE);
	if(fnet_heap_global == 0) {
		printf("failed to allocate fnet heap\n");
		while(1);
	}

    init_params.netheap_ptr = fnet_heap_global;
    init_params.netheap_size = FNET_CFG_HEAP_SIZE;

    return fnet_init(&init_params);
}

void wifi_set_ip() {
#ifdef ARM9
	if(wifi->aid) {
		int id = wifi->aid + 1;
        fnet_netif_set_ip4_addr(FNET_ETH_IF, FNET_IP4_ADDR_INIT(192, 168, 1,(id & 0xff)));
	}
#else
        fnet_netif_set_ip4_addr(FNET_ETH_IF, FNET_IP4_ADDR_INIT(192, 168, 1,2));
#endif
}

void wifi_fnet_init(int dhcp) {
	fnet_netif_desc_t netif;
	struct fnet_dhcp_params dhcp_params;

	/* FNET Initialization */
    if (fnet_init_global() != FNET_ERR) 
    {
        fnet_printf(__TIME__);
        fnet_printf("\nTCP/IP stack initialization is done.\n");
    }
    else 
    {
        fnet_printf("\nError:TCP/IP stack initialization is failed.\n");
    }

	if(!dhcp) {
		wifi_set_ip();
		return;
	}

	// Get current net interface.
	if((netif = fnet_netif_get_default()) == 0)
	{
		fnet_printf("ERROR: Network Interface is not configurated!");
	}
	else
	{
		fnet_memset_zero(&dhcp_params, sizeof(struct fnet_dhcp_params));
		// Enable DHCP client.
		if(fnet_dhcp_init(netif, &dhcp_params) != FNET_ERR)
		{
			// Register DHCP event handlers.
			fnet_dhcp_handler_updated_set(fapp_dhcp_handler_updated, 0);
			fnet_dhcp_handler_discover_set(fapp_dhcp_handler_discover, 0);

			while(fnet_dhcp_state() != FNET_DHCP_STATE_BOUND) {
				fnet_poll_services();
#ifdef ARM9
				swiWaitForVBlank();
#endif
			}
			fnet_printf("DHCP done %d\n",fnet_dhcp_state());
#ifdef ARM9
			//while((keysCurrent()&KEY_L) == 0);
#endif
			//tcp_test();
		}
		else
		{
			fnet_printf("ERROR: DHCP initialization is failed!");
		}
	}
}
