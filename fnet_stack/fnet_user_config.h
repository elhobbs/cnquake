/**********************************************************************/ /*!
*
* @file fnet_user_config.template.h
*
* @brief Template of FNET User configuration file.
*        It should be used to change any default configuration parameter.
*
***************************************************************************/
/*! @cond */
#ifndef _FNET_USER_CONFIG_H_
#define _FNET_USER_CONFIG_H_

/*****************************************************************************
* Enable proper compiler support.
******************************************************************************/
#define FNET_CFG_COMP_CW            (0)  
#define FNET_CFG_COMP_IAR           (0)  
#define FNET_CFG_COMP_UV            (0)  
#define FNET_CFG_COMP_GCC           (1)    

/*****************************************************************************
* Processor type.
* Selected processor definition should be only one and must be defined as 1. 
* All others may be defined but must have 0 value.
******************************************************************************/
#define FNET_CFG_CPU_MCF52235       (0)
#define FNET_CFG_CPU_MCF52259       (0)
#define FNET_CFG_CPU_MCF5282        (0)
#define FNET_CFG_CPU_MCF51CN128     (0)
#define FNET_CFG_CPU_MK60N512       (0)
#define FNET_CFG_CPU_ARM	        (1)

/*****************************************************************************
* IPv4 and/or IPv6 protocol support.
******************************************************************************/
#define FNET_CFG_IP4                (1)
#define FNET_CFG_IP6                (0)

/*****************************************************************************
* IP address for the Ethernet interface. 
* At runtime it can be changed by the fnet_netif_set_address() or 
* by the DHCP client service.
******************************************************************************/
#define FNET_CFG_ETH_IP4_ADDR        (FNET_IP4_ADDR_INIT(192, 168, 1, 1))

/*****************************************************************************
* IP Subnet mask for the Ethernet interface. 
* At runtime it can be changed by the fnet_netif_set_netmask() or 
* by the DHCP client service.
******************************************************************************/
#define FNET_CFG_ETH_IP4_MASK        (FNET_IP4_ADDR_INIT(255, 255, 255, 0))

/*****************************************************************************
* Gateway IP address for the Ethernet interface.
* At runtime it can be changed by the fnet_netif_set_gateway() or 
* by the DHCP client service.
******************************************************************************/
#define FNET_CFG_ETH_IP4_GW          (FNET_IP4_ADDR_INIT(192, 168, 1, 1))

/*****************************************************************************
* DNS server IP address for the Ethernet interface.
* At runtime it can be changed by the fnet_netif_set_dns() or 
* by the DHCP client service. 
* It is used only if FNET_CFG_DNS is set to 1.
******************************************************************************/
#define FNET_CFG_ETH_IP4_DNS         (FNET_IP4_ADDR_INIT(192, 168, 1, 1)) 

/*****************************************************************************
* Size of the internal static heap buffer. 
* This definition is used only if the fnet_init_static() was 
* used for the FNET initialization.
******************************************************************************/
#define FNET_CFG_HEAP_SIZE          (30 * 1024)

/*****************************************************************************
* TCP protocol support.
* You can disable it to save a substantial amount of code if 
* your application only needs UDP. By default it is enabled.
******************************************************************************/
#define FNET_CFG_TCP                (1)

/*****************************************************************************
* UDP protocol support.
* You can disable it to save a some amount of code if your 
* application only needs TCP. By default it is enabled.
******************************************************************************/
#define FNET_CFG_UDP                (1)

/*****************************************************************************
* UDP checksum.
* If enabled, the UDP checksum will be generated for transmitted 
* datagrams and be verified on received UDP datagrams.
* You can disable it to speedup UDP applications. 
* By default it is enabled.
******************************************************************************/
#define FNET_CFG_UDP_CHECKSUM       (1)

/*****************************************************************************
* IP fragmentation.
* If the IP fragmentation is enabled, the IP will attempt to reassemble IP 
* packet fragments and will able to generate fragmented IP packets.
* If disabled, the IP will  silently discard fragmented IP packets..
******************************************************************************/
#define FNET_CFG_IP4_FRAGMENTATION  (1)

/*****************************************************************************
* MTU.
* Defines the Maximum Transmission Unit for the Ethernet interface.
* The largest value is 1500. Recommended range is 600 - 1500
******************************************************************************/
#define FNET_CFG_ETH_MTU            (1500)

/*****************************************************************************
* DHCP Client service support.
******************************************************************************/
#define FNET_CFG_DHCP               (1)

/*****************************************************************************
* HTTP Server service support.
******************************************************************************/
#define FNET_CFG_HTTP                       (0)
#define FNET_CFG_HTTP_AUTHENTICATION_BASIC  (0) /* Enable HTTP authentication.*/
#define FNET_CFG_HTTP_POST                  (0) /* Enable HTTP POST-method support.*/

/*****************************************************************************
* Telnet Server service support.
******************************************************************************/
#define FNET_CFG_TELNET                     (0)

/*****************************************************************************
* DNS support by network interface.
******************************************************************************/
#define FNET_CFG_DNS                        (1)

/*****************************************************************************
* DNS address client/resolver service support.
******************************************************************************/
#define FNET_CFG_DNS_RESOLVER               (1)

/*****************************************************************************
* PING service support.
******************************************************************************/
#define FNET_CFG_PING                       (1)

/*****************************************************************************
* Flash Module driver support.
******************************************************************************/
#define FNET_CFG_FLASH                      (0)

/*****************************************************************************
* provide own checksum low.
******************************************************************************/
#define FNET_CFG_OVERLOAD_CHECKSUM_LOW      (1)
#endif
/*! @endcond */
