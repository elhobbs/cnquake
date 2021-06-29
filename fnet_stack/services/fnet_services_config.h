/**************************************************************************
* 
* Copyright 2012-2013 by Andrey Butok. FNET Community.
* Copyright 2005-2011 by Andrey Butok. Freescale Semiconductor, Inc.
*
***************************************************************************
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License Version 3 
* or later (the "LGPL").
*
* As a special exception, the copyright holders of the FNET project give you
* permission to link the FNET sources with independent modules to produce an
* executable, regardless of the license terms of these independent modules,
* and to copy and distribute the resulting executable under terms of your 
* choice, provided that you also meet, for each linked independent module,
* the terms and conditions of the license of that module.
* An independent module is a module which is not derived from or based 
* on this library. 
* If you modify the FNET sources, you may extend this exception 
* to your version of the FNET sources, but you are not obligated 
* to do so. If you do not wish to do so, delete this
* exception statement from your version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* You should have received a copy of the GNU General Public License
* and the GNU Lesser General Public License along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*
***********************************************************************/ /*!
*
* @file fnet_services_config.h
*
* @author Andrey Butok
*
* @date Dec-19-2012
*
* @version 0.1.29.0
*
* @brief Services default configuration.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_SERVICES_CONFIG_H_

#define _FNET_SERVICES_CONFIG_H_

#include "fnet_config.h"  

/*! @addtogroup fnet_services_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_POLL_MAX
 * @brief   Maximum number of registered services in the polling list.@n
 *          Default value is @b @c 5.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_POLL_MAX
    #define FNET_CFG_POLL_MAX   (5)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_DHCP
 * @brief    DHCP Client service support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_DHCP
    #define FNET_CFG_DHCP       (0)
#endif

#if FNET_CFG_DHCP 
    #include "fnet_dhcp_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_HTTP
 * @brief    HTTP Server service support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_HTTP
    #define FNET_CFG_HTTP       (0)
#endif

#if FNET_CFG_HTTP
    /* Force FS if HTTP is defined. */
    #undef FNET_CFG_FS
    #define FNET_CFG_FS         (1)

    #include "fnet_http_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FS
 * @brief    File System Interface support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_FS
    #define FNET_CFG_FS         (0) 
#endif

#if FNET_CFG_FS 
    #include "fnet_fs_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TFTP_CLN
 * @brief    TFTP Client support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_CLN
    #define FNET_CFG_TFTP_CLN   (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TFTP_SRV
 * @brief    TFTP Server support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TFTP_SRV
    #define FNET_CFG_TFTP_SRV   (0)
#endif

#if FNET_CFG_TFTP_CLN || FNET_CFG_TFTP_SRV  
    #include "fnet_tftp_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_FLASH
 * @brief    On-chip Flash driver support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_FLASH
    #define FNET_CFG_FLASH      (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TELNET
 * @brief    Telnet server support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TELNET
    #define FNET_CFG_TELNET     (0)
#endif

#if FNET_CFG_TELNET 
    #include "fnet_telnet_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_DNS_RESOLVER
 * @brief    DNS client/resolver support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_DNS_RESOLVER
    #define FNET_CFG_DNS_RESOLVER   (0)
#endif


#if FNET_CFG_DNS_RESOLVER 
    #include "fnet_dns_config.h"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_PING
 * @brief    PING service support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_PING
    #define FNET_CFG_PING           (0)
#endif

#if FNET_CFG_PING 
    /* Force RAW sockets, if PING is defined. */
    #undef FNET_CFG_RAW
    #define FNET_CFG_RAW            (1)


    #include "fnet_ping_config.h"
#endif

#if 0
/* Include Serial Library default configuration. */
#include "fnet_serial_config.h"

/* Include Shell Library default configuration. */
#include "fnet_shell_config.h"
#endif

/*! @} */

#endif
