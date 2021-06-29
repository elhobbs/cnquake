/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// net_udp.c
#if defined(USE_WIFI) || defined(USE_DSNIFI)

#include "quakedef.h"

#include <sys/types.h>

#ifdef USE_DSNIFI
#include "fnet_socket.h"
#include "fnet_cpu.h"
#include "fnet_inet.h"
#include "fnet_eth_prv.h"
#include "fnet_ip.h"

#define htons fnet_htons
#define htonl fnet_htonl
#define ntohs fnet_ntohs
#define ntohl fnet_ntohl

static int dsnifi_initialized = 0;
void wifi_fnet_init(int dhcp);

#endif

#if 0
#ifdef NDS
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <dswifi9.h>
#else
#include <winsock2.h>
#define EWOULDBLOCK             WSAEWOULDBLOCK
#define ECONNREFUSED            WSAECONNREFUSED
#define ioctl					ioctlsocket
#endif
#endif

#include <errno.h>

#ifdef __sun__
#include <sys/filio.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

//extern int gethostname (char *, int);
//extern int close (int);

extern cvar_t hostname;

static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
static struct qsockaddr broadcastaddr;

static unsigned long myAddr;

#define MAXHOSTNAMELEN		256

#include "net_udp.h"

#if 0
#ifdef WIN32
int winsock_initialized = 0;
WSADATA		winsockdata;
#endif
#endif
//=============================================================================

int UDP_Init (void)
{
	struct hostent *local;
	char	buff[MAXHOSTNAMELEN];
	struct qsockaddr addr;
	struct in_addr ip, gateway, mask, dns1, dns2;
	char *colon;

#if 0
#ifdef WIN32
	WORD	wVersionRequested;
	int		r;
#endif
#endif
	
	if (COM_CheckParm ("-noudp"))
		return -1;
#if 0
#ifdef WIN32
	if (winsock_initialized == 0)
	{
		wVersionRequested = MAKEWORD(1, 1); 

		r = WSAStartup (MAKEWORD(2, 2), &winsockdata);

		if (r)
		{
			Con_SafePrintf ("Winsock initialization failed.\n");
			return -1;
		}
	}
	winsock_initialized++;
#endif
#endif

	if(dsnifi_initialized == 0) {
		//wifi_fnet_init(0);
		dsnifi_initialized = 1;
	}
	// if the quake hostname isn't set, set it to the machine name
	if (Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		buff[15] = 0;
		Cvar_Set ("hostname", "CQUAKE");
	}

	if ((net_controlsocket = UDP_OpenSocket (0)) == -1)
		Sys_Error("UDP_Init: Unable to open control socket\n");
#if 0
#ifdef WIN32
	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons(net_hostport);

	UDP_GetSocketAddr (net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address,  UDP_AddrToString (&addr));
	colon = Q_strrchr (my_tcpip_address, ':');
	if (colon)
		*colon = 0;
#else
	// determine my name & address
	ip = Wifi_GetIPInfo(&gateway, &mask, &dns1, &dns2);
	
	/*
	unsigned long dummy;
	unsigned long subnet_mask;
	
	Wifi_GetIPInfo(&dummy, &subnet_mask, &dummy, &dummy);*/
	myAddr = Wifi_GetIP();
	
	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;//ntohl(myAddr);
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons(net_hostport);

	UDP_GetSocketAddr (net_controlsocket, &addr);
	Q_strcpy(my_tcpip_address,  UDP_AddrToString (&addr));
#endif
#endif

#ifdef USE_DSNIFI

	do {
	((struct sockaddr_in *)&broadcastaddr)->sin_family = AF_INET;
	((struct sockaddr_in *)&broadcastaddr)->sin_addr.s_addr = INADDR_BROADCAST;//ntohl(myAddr);
	((struct sockaddr_in *)&broadcastaddr)->sin_port = htons(net_hostport);

	ip.s_addr = fnet_netif_get_ip4_addr(FNET_ETH_IF);
		fnet_inet_ntoa(ip,my_tcpip_address);
	} while(0);

#endif

	Con_Printf("UDP Initialized\n");
	tcpipAvailable = true;

	return net_controlsocket;
}

//=============================================================================

void UDP_Shutdown (void)
{
	UDP_Listen (false);
	UDP_CloseSocket (net_controlsocket);
}

//=============================================================================

void UDP_Listen (qboolean state)
{
	// enable listening
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = UDP_OpenSocket (net_hostport)) == -1)
			Sys_Error ("UDP_Listen: Unable to open accept socket\n");
		return;
	}

	// disable listening
	if (net_acceptsocket == -1)
		return;
	UDP_CloseSocket (net_acceptsocket);
	net_acceptsocket = -1;
}

//=============================================================================

int UDP_OpenSocket (int port)
{
	int newsocket;
	struct sockaddr_in address;
	qboolean _true = true;

	if ((newsocket = socket (AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		int error_code = fnet_error_get();
		Con_Printf("UDP_OpenSocket: socket failed %d\n",error_code);
		return -1;
	}

#ifndef USE_DSNIFI
	if (ioctl (newsocket, FIONBIO, (char *)&_true) == -1)
	{
		Con_Printf("UDP_OpenSocket: ioctl failed\n");
		goto ErrorReturn;
	}
#endif

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if( bind (newsocket, (const struct sockaddr *)&address, sizeof(address)) == -1)
	{
		Con_Printf("UDP_OpenSocket: bind failed\n");
		goto ErrorReturn;
	}

	return newsocket;

ErrorReturn:
	closesocket (newsocket);
	return -1;
}

//=============================================================================

int UDP_CloseSocket (int socket)
{
	if (socket == net_broadcastsocket)
		net_broadcastsocket = 0;
	return closesocket (socket);
}


//=============================================================================
/*
============
PartialIPAddress

this lets you type only as much of the net address as required, using
the local network components to fill in the rest
============
*/
static int PartialIPAddress (char *in, struct qsockaddr *hostaddr)
{
	char buff[256];
	char *b;
	int addr;
	int num;
	int mask;
	int run;
	int port;
	
	buff[0] = '.';
	b = buff;
	strcpy(buff+1, in);
	if (buff[1] == '.')
		b++;

	addr = 0;
	mask=-1;
	while (*b == '.')
	{
		b++;
		num = 0;
		run = 0;
		while (!( *b < '0' || *b > '9'))
		{
		  num = num*10 + *b++ - '0';
		  if (++run > 3)
		  	return -1;
		}
		if ((*b < '0' || *b > '9') && *b != '.' && *b != ':' && *b != 0)
			return -1;
		if (num < 0 || num > 255)
			return -1;
		mask<<=8;
		addr = (addr<<8) + num;
	}
	
	if (*b++ == ':')
		port = Q_atoi(b);
	else
		port = net_hostport;

	hostaddr->sa_family = AF_INET;
	((struct sockaddr_in *)hostaddr)->sin_port = htons((short)port);	
	((struct sockaddr_in *)hostaddr)->sin_addr.s_addr = (myAddr & htonl(mask)) | htonl(addr);
	
	return 0;
}
//=============================================================================

int UDP_Connect (int socket, struct qsockaddr *addr)
{
	return 0;
}

//=============================================================================

int UDP_CheckNewConnections (void)
{
	unsigned long	available;
	int cb=sizeof(available);

	if (net_acceptsocket == -1)
		return -1;

#ifdef USE_DSNIFI
	if(getsockopt(net_acceptsocket,SOL_SOCKET,SO_RCVNUM,(char *)&available,&cb))
#else
	if (ioctl (net_acceptsocket, FIONREAD, &available) == -1)
#endif
		Sys_Error ("UDP: ioctlsocket (FIONREAD) failed\n");

	if (available)
		return net_acceptsocket;
	return -1;
}

//=============================================================================

int UDP_Read (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int addrlen = sizeof (struct qsockaddr);
	int ret;

	ret = recvfrom (socket, (char *)buf, len, 0, (struct sockaddr *)addr, &addrlen);
	if (ret == -1)//USE_DSNIFI && (errno == EWOULDBLOCK || errno == ECONNREFUSED))
		return 0;
	return ret;
}

//=============================================================================

int UDP_MakeSocketBroadcastCapable (int socket)
{
	int				i = 1;

#ifndef USE_DSNIFI
	// make this socket broadcast capable
	if (setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char *)&i, sizeof(i)) < 0)
		return -1;
#endif

	net_broadcastsocket = socket;

	return 0;
}

//=============================================================================

int UDP_Broadcast (int socket, byte *buf, int len)
{
	int ret;

	if (socket != net_broadcastsocket)
	{
		if (net_broadcastsocket != 0)
			Sys_Error("Attempted to use multiple broadcasts sockets\n");
		ret = UDP_MakeSocketBroadcastCapable (socket);
		if (ret == -1)
		{
			Con_Printf("Unable to make socket broadcast capable\n");
			return ret;
		}
	}
	return UDP_Write (socket, buf, len, &broadcastaddr);
}

//=============================================================================

int UDP_Write (int socket, byte *buf, int len, struct qsockaddr *addr)
{
	int ret,cb_write=len,cb_sent = 0;

	do {
		ret = sendto (socket, buf, cb_write, 0, (struct sockaddr *)addr, sizeof(struct qsockaddr));
		if (ret == -1 && errno == EWOULDBLOCK)
			return 0;
		if(ret < 0)
			break;
		cb_sent += ret;
		cb_write -= ret;
		buf += ret;
	} while (cb_write > 0);
	return cb_sent;
}

//=============================================================================

static char addr_buffer[64];
char *UDP_AddrToString (struct qsockaddr *addr)
{
	int haddr;

	haddr = ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr);
	sprintf(addr_buffer, "%d.%d.%d.%d:%d", (haddr >> 24) & 0xff, (haddr >> 16) & 0xff, (haddr >> 8) & 0xff, haddr & 0xff, ntohs(((struct sockaddr_in *)addr)->sin_port));
	return addr_buffer;
}

//=============================================================================

int UDP_StringToAddr (char *string, struct qsockaddr *addr)
{
	int ha1, ha2, ha3, ha4, hp;
	int ipaddr;

	sscanf(string, "%d.%d.%d.%d:%d", &ha1, &ha2, &ha3, &ha4, &hp);
	ipaddr = (ha1 << 24) | (ha2 << 16) | (ha3 << 8) | ha4;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_addr.s_addr = htonl(ipaddr);
	((struct sockaddr_in *)addr)->sin_port = htons(hp);
	return 0;
}

//=============================================================================

int UDP_GetSocketAddr (int socket, struct qsockaddr *addr)
{
    struct in_addr      ip_address;
	int addrlen = sizeof(struct qsockaddr);
	unsigned int a;

	fnet_inet_aton("127.0.0.1",&ip_address);
	Q_memset(addr, 0, sizeof(struct qsockaddr));
	getsockname(socket, (struct sockaddr *)addr, &addrlen);
	a = ((struct sockaddr_in *)addr)->sin_addr.s_addr;
	if (a == 0 || a == ip_address.s_addr)
		((struct sockaddr_in *)addr)->sin_addr.s_addr = myAddr;

	return 0;
}

//=============================================================================

int UDP_GetNameFromAddr (struct qsockaddr *addr, char *name)
{
	struct hostent *hostentry;

/*	hostentry = gethostbyaddr ((char *)&((struct sockaddr_in *)addr)->sin_addr, sizeof(struct in_addr), AF_INET);
	if (hostentry)
	{
		Q_strncpy (name, (char *)hostentry->h_name, NET_NAMELEN - 1);
		return 0;
	}*/

	Q_strcpy (name, UDP_AddrToString (addr));
	return 0;
}

//=============================================================================

int UDP_GetAddrFromName(char *name, struct qsockaddr *addr)
{
	struct hostent *hostentry;

	Con_Printf("AddrFromName: %s\n",name);

	if (name[0] >= '0' && name[0] <= '9')
		return PartialIPAddress (name, addr);

#ifndef USE_DSNIFI
	hostentry = gethostbyname (name);
	if (!hostentry)
		return -1;

	addr->sa_family = AF_INET;
	((struct sockaddr_in *)addr)->sin_port = htons(net_hostport);	
	((struct sockaddr_in *)addr)->sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];
#endif
	return 0;
}

//=============================================================================

int UDP_AddrCompare (struct qsockaddr *addr1, struct qsockaddr *addr2)
{
	/*if (addr1->sa_family != addr2->sa_family)
	{
		Con_Printf("UDP_AddrCompare: sa_family %d %d\n",addr1->sa_family,addr2->sa_family);
		return -1;
	}*/

	if (((struct sockaddr_in *)addr1)->sin_addr.s_addr != ((struct sockaddr_in *)addr2)->sin_addr.s_addr)
	{
		Con_Printf("UDP_AddrCompare: s_addr %d %d\n",((struct sockaddr_in *)addr1)->sin_addr.s_addr,((struct sockaddr_in *)addr2)->sin_addr.s_addr);
		return -1;
	}

	if (((struct sockaddr_in *)addr1)->sin_port != ((struct sockaddr_in *)addr2)->sin_port)
	{
		Con_Printf("UDP_AddrCompare: sin_port %d %d\n",((struct sockaddr_in *)addr1)->sin_port,((struct sockaddr_in *)addr2)->sin_port);
		return 1;
	}

	return 0;
}

//=============================================================================

int UDP_GetSocketPort (struct qsockaddr *addr)
{
	return ntohs(((struct sockaddr_in *)addr)->sin_port);
}


int UDP_SetSocketPort (struct qsockaddr *addr, int port)
{
	((struct sockaddr_in *)addr)->sin_port = htons(port);
	return 0;
}

//=============================================================================
#endif
