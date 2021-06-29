#include "fnet_config.h"
#if FNET_ARM && FNET_CFG_ETH

#include "fnet_eth_prv.h"
#include "fnet_socket_prv.h"

#if NDS
#include "wifi_state.h"
#include "802.11.h"
#include "802.2.h"
#endif

int fnet_arm_init(fnet_netif_t *netif);
void fnet_arm_release(fnet_netif_t *netif);
int fnet_arm_is_connected(fnet_netif_t *netif);
void fnet_netbuf_to_wifi_tx_buffer( fnet_netbuf_t *nb, unsigned short type, const fnet_mac_addr_t dest_addr);
void fnet_arm_eth_output(fnet_netif_t *netif, unsigned short type, const fnet_mac_addr_t dest_addr, fnet_netbuf_t* nb);
extern const fnet_netif_api_t fnet_arm_api;

#if 1
struct fnet_eth_if fnet_arm_eth0_if =
{
    0,
    fnet_arm_eth_output
#if FNET_CFG_MULTICAST
    ,      
    fnet_fec_multicast_join,
    fnet_fec_multicast_leave,
#endif /* FNET_CFG_MULTICAST */    
};

fnet_netif_t fnet_eth0_if =
{
	0,                      /* Pointer to the next net_if structure.*/
	0,                      /* Pointer to the previous net_if structure.*/
	"eth0",                 /* Network interface name.*/
	FNET_CFG_ETH_MTU,       /* Maximum transmission unit.*/
	&fnet_arm_eth0_if,      /* Points to interface specific data structure.*/
	&fnet_arm_api           /* Interface API */  
};

void wifi_copy_mac(volatile void * dest, volatile void * src);
int fnet_arm_get_hw_addr(fnet_netif_t *netif, unsigned char * hw_addr) {
#if NDS
	wifi_copy_mac(hw_addr, wifi->mac);
#endif
}

const fnet_netif_api_t fnet_arm_api =
{
    FNET_NETIF_TYPE_ETHERNET,       /* Data-link type. */
    sizeof(fnet_mac_addr_t),
    fnet_arm_init,                  /* Initialization function.*/
	fnet_arm_release,               /* Shutdown function.*/
#if FNET_CFG_IP4	
	fnet_eth_output_ip4,            /* IPv4 Transmit function.*/
#endif	
	fnet_eth_change_addr_notify,    /* Address change notification function.*/
	fnet_eth_drain,                 /* Drain function.*/
	fnet_arm_get_hw_addr,
	0,//fnet_fec_set_hw_addr,
	fnet_arm_is_connected,
	0//fnet_fec_get_statistics
#if FNET_CFG_MULTICAST 
    #if FNET_CFG_IP4
        ,	
	    fnet_eth_multicast_join_ip4,
	    fnet_eth_multicast_leave_ip4
    #endif
    #if FNET_CFG_IP6
        ,
	    fnet_eth_multicast_join_ip6,
	    fnet_eth_multicast_leave_ip6
    #endif    
#endif
#if FNET_CFG_IP6
    ,
    fnet_eth_output_ip6            /* IPv6 Transmit function.*/
#endif	
};

void wifi_rx_data();

void fnet_arm_input_low() {

	fnet_isr_lock();

#if NDS
	wifi_rx_data();
#endif

	fnet_isr_unlock();
}

int fnet_arm_init(fnet_netif_t *netif)
{
    int result = FNET_OK;

    /* Install SW Interrupt handler. */
    result = fnet_event_init(FNET_EVENT_RX_DATA, fnet_arm_input_low);

	return result;
}

void fnet_arm_release(fnet_netif_t *netif)
{
}

int fnet_arm_is_connected(fnet_netif_t *netif)
{
	return 1;
}


void fnet_arm_eth_output(fnet_netif_t *netif, unsigned short type, const fnet_mac_addr_t dest_addr, fnet_netbuf_t* nb) {

#ifdef NDS
	fnet_netbuf_to_wifi_tx_buffer(nb, type, dest_addr);
#endif

    fnet_netbuf_free_chain(nb);   
}
#endif

#endif