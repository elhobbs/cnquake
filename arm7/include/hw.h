#ifndef __HW_H__
#define __HW_H__

#include <nds\ndstypes.h>

// Wifi HW Register Indices
#define WIFI_MODE_RST		0x0004
#define WIFI_MODE_WEP		0x0006
#define WIFI_IF				0x0010
#define WIFI_IE				0x0012
#define WIFI_MAC_ADDR		0x0018
#define WIFI_BSSID			0x0020
#define WIFI_AIDS			0x0028
#define WIFI_AIDS_FULL		0x002A
#define WIFI_RETRYLIMIT		0x002C
#define WIFI_RXCNT			0x0030
#define WIFI_RXWEPCNT		0x0032
#define WIFI_POWER_US		0x0036
#define WIFI_PSCNT			0x0038
#define WIFI_POWERSTATE		0x003C	
#define WIFI_FORCE_PS		0x0040
#define WIFI_RANDOM			0x0044
#define WIFI_RXRANGE_BEGIN	0x0050
#define WIFI_RXRANGE_END	0x0052
#define WIFI_RXWRITECSR		0x0054
#define WIFI_WRITECSRLATCH	0x0056
#define WIFI_CIRCBUFRADDR	0x0058
#define WIFI_RXREADCSR		0x005A
#define WIFI_CIRCBUFREAD	0x0060
#define WIFI_FIFOEND		0x0062
#define WIFI_CIRCBUFWADDR	0x0068
#define WIFI_CIRCBUFWRITE	0x0070
#define WIFI_CIRCBUFW_END	0x0074
#define WIFI_CIRCBUFFER_SKP	0x0076
#define WIFI_BEACONTRANS	0x0080
#define WIFI_LISTENCOUNT	0x0088
#define WIFI_BEACONPERIOD	0x008C
#define WIFI_LISTENINT		0x008E
// old TXLOC:
#define WIFI_TXLOC1			0x00A0
#define WIFI_TXLOC2			0x00A4
#define WIFI_TXLOC3			0x00A8
// better:
#define WIFI_TXSLOT(n)		(0x00A0 + (n) * 4)
#define WIFI_TXRESET		0x00AC
#define WIFI_TXCNT			0x00AE
#define WIFI_TXINFO			0x00B0
#define WIFI_TXBUSY			0x00B6
#define WIFI_TXSTAT			0x00B8
#define WIFI_TXPREAMBLE		0X00BC
#define WIFI_RXFILTER		0x00D0
#define WIFI_RXFILTER2		0x00E0
#define WIFI_USCOUNTER_CR	0x00E8
#define WIFI_USCOMPARE_CR	0x00EA
#define WIFI_USCOMPARE_VAL0	0x00F0
#define WIFI_USCOMPARE_VAL1	0x00F2
#define WIFI_USCOMPARE_VAL2	0x00F4
#define WIFI_USCOMPARE_VAL3	0x00F6
#define WIFI_USCOUNTER_VAL0	0x00F8
#define WIFI_USCOUNTER_VAL1	0x00FA
#define WIFI_USCOUNTER_VAL2	0x00FC
#define WIFI_USCOUNTER_VAL3	0x00FE
#define WIFI_PRE_BEACON		0x0110
#define W_BEACONCOUNT1		0x011C
#define WIFI_BBSIO_CR		0x0158
#define WIFI_BBSIO_WRITE	0x015A
#define WIFI_BBSIO_READ		0x015C
#define WIFI_BBSIO_STATUS	0x015E
#define WIFI_BBSIO_MODE		0x0160
#define WIFI_BBSIO_POWER	0x0168
#define WIFI_RFSIO_DATA2	0x017C
#define WIFI_RFSIO_DATA1	0x017E
#define WIFI_RFSIO_STATUS	0x0180
#define WIFI_RFSIO_CR		0x0182
#define WIFI_TXMODIFY		0x0194
#define WIFI_IRQDEBUG		0x021C
#define WIFI_MEMORY			WIFI_MEMORYSTART
#define WIFI_MEMORYSTART	0x4000
#define WIFI_WEPKEY0		0x5F80
#define WIFI_WEPKEY1		0x5FA0
#define WIFI_WEPKEY2		0x5FC0
#define WIFI_WEPKEY3		0x5FE0
#define WIFI_MEMORYEND		0x6000

#define WIFI_REG(n)			(*(volatile unsigned short *)(0x04800000 + n))
#define WIFI_REGISTER(n)			(*(volatile unsigned short *)(0x04800000 + n))
#define WIFI_REGISTER_NOACTION(n)	(*(volatile unsigned short *)(0x04801000 + n))

#define WIFI_BB_CNT			0x0001
#define WIFI_BB_CCAOP		0x0013
#define WIFI_BB_CHANNEL		0x001E
#define WIFI_BB_EDTHRESHOLD	0x0035


// Wifi register values
// RXCNT:
#define WIFI_RX_ENABLE			BIT(15)
#define WIFI_RXRANGE_LATCH		BIT(0)
// WEP:
#define WIFI_RXWEBENABLE		BIT(15)
// TXCNT:
#define WIFI_SLOT(n)			((n)>0?BIT(((n)+1)):BIT(0))
// IRQs:
#define WIFI_IRQ_RXCOMPLETE		BIT(0)
#define WIFI_IRQ_TXCOMPLETE		BIT(1)
#define WIFI_IRQ_RXCOUNT		BIT(2)
#define WIFI_IRQ_TXERROR		BIT(3)
#define WIFI_IRQ_STATOVERFLOW	BIT(4)
#define WIFI_IRQ_STATINCREASE	BIT(5)
#define WIFI_IRQ_RXSTART		BIT(6)
#define WIFI_IRQ_TXSTART		BIT(7)
#define WIFI_IRQ_RF_WAKEUP		BIT(11)
#define WIFI_IRQ_BCNTIMESLOT	BIT(14)
#define WIFI_IRQ_PREBCNTIMESLOT	BIT(15)
// TX modifications
#define WIFI_USESW_DURATION		BIT(0) 
#define WIFI_USESW_FCS			BIT(1) 
#define WIFI_USESW_SEQUENCE		BIT(2) 
// TX Loaction
#define WIFI_TXSLOTENABLE		BIT(15)
// TX Speed
#define WIFI_TX1MBIT			0x000A
#define WIFI_TX2MBIT			0x0014
// BB control
#define WIFI_BB_ENABLE			BIT(7)
// BB CCA Operation
#define WIFI_BB_USECS			0x0000
#define WIFI_BB_USEED			0x0001
#define WIFI_BB_USEEITHERCSED	0x0002
#define WIFI_BB_USEBOTHCSED		0x0003
// RXFILTER:
#define WIFI_RXFILTERDATA		BIT(11)
#define WIFI_RXFILTER_MANAGEMENT		BIT(10)
#define WIFI_RXFILTER_PROBE		BIT(8)
#define WIFI_RXFILTERBEACON		BIT(0)

#define		SPI_CR		(*((u16 volatile *) 0x040001C0))
#define		SPI_DATA	(*((u16 volatile *) 0x040001C2))
#define		POWERCNT7	(*((u16 volatile *) 0x04000304))
  
#define WIFI_WAITCR			(*((volatile unsigned long *)0x04000206))
#define W_WEPKEY0		(((volatile u16 *)(0x04805F80)))
#define W_WEPKEY1		(((volatile u16 *)(0x04805FA0)))
#define W_WEPKEY2		(((volatile u16 *)(0x04805FC0)))
#define W_WEPKEY3		(((volatile u16 *)(0x04805FE0)))
#define W_RANDOM		(*((volatile u16 *)(0x04800044)))
#define W_BSSID			(((volatile u16 *)(0x04800020)))
#define W_MACADDR		(((volatile u16 *)(0x04800018)))
#define W_RETRLIMIT		(*((volatile u16 *)(0x0480002C)))
#define W_IF			(*((volatile u16 *)(0x04800010)))
#define W_IE			(*((volatile u16 *)(0x04800012)))
#define W_POWERSTATE	(*((volatile u16 *)(0x0480003C)))
#define W_AIDS			(*((volatile u16 *)(0x04800028)))

#define POWER_WIFI			0x0002
#define FW_MAC				0x0036

void hw_init_flash_data();

unsigned char hw_bb_read(unsigned short addr);

void hw_bb_write(unsigned short addr, unsigned char data);

void hw_rf_write(unsigned long data);

void hw_read_flash(int address, char * destination, int length);
 
int hw_read_flash_byte(int address);
 
int hw_read_flash_bytes(int address, int numbytes);

int hw_read_flash_short(int address);

void hw_set_wep_mode(int wepmode);

void hw_set_wep_key(void * wepkey);

void hw_set_channel(int channel);

int wifi_compare_mac(volatile void * mac1,volatile  void * mac2);

void wifi_copy_mac(volatile void * dest, volatile void * src);

int wifi_tx_queue(u16 * data, int datalen);

int wifi_send_data_frame(u16 *dest,u8 *buffer, int len);


typedef enum {
	WEPMODE_NONE = 0,
	WEPMODE_40BIT = 1,
	WEPMODE_128BIT = 2
} WEPMODES;


#endif //__HW_H__