#include <nds\ndstypes.h>
#include "hw.h"
#include "802.11.h"
#include "wifi_state.h"
#include <string.h>


int wifi_send_data_frame(u16 *dest,u8 *buffer, int len) {
	DATA_FRAME_HEADER *head = ((DATA_FRAME_HEADER *)buffer)-1;
	u8 *frame = (u8 *)head;
	u8 *data = buffer + len;
	memset(head,0,sizeof(*head));
	head->fc.type = 0x02;
	head->fc.subType = 0x0;
	wifi_copy_mac(head->A1,dest);
	wifi_copy_mac(head->A2,wifi->mac);
	wifi_copy_mac(head->A3,wifi->ap.bssid);

	head->tx.tx_length = data-frame-12+4;

	wifi->data_send++;

	return wifi_tx_queue((u16 *)frame,data-frame);
}
