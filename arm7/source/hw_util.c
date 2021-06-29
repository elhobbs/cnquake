#include <nds\ndstypes.h>
#include <nds\arm7\serial.h>
#include "hw.h"

unsigned char hw_bb_read(unsigned short addr)
{
	int timeout = 10000 ;
	while ((WIFI_REGISTER(WIFI_BBSIO_STATUS) & 1) && (--timeout)) ;
	if (!timeout) 
	{
		return 0 ;
	}
	WIFI_REGISTER(WIFI_BBSIO_CR) = addr | 0x6000 ;
	while (WIFI_REGISTER(WIFI_BBSIO_STATUS) & 1) ;
	return WIFI_REGISTER(WIFI_BBSIO_READ) ;
}

void hw_bb_write(unsigned short addr, unsigned char data)
{
	int timeout = 10000;
	while ((WIFI_REGISTER(WIFI_BBSIO_STATUS) & 1) && (--timeout)) ;
	if (!timeout) 
	{
		return ;
	}
	WIFI_REGISTER(WIFI_BBSIO_WRITE) = data ;
	WIFI_REGISTER(WIFI_BBSIO_CR) = addr | 0x5000 ;
	timeout = 10000;
	while ((WIFI_REGISTER(WIFI_BBSIO_STATUS) & 1) && (--timeout)) ;
	return ;
}

void hw_rf_write(unsigned long data)
{
	WIFI_REGISTER(WIFI_RFSIO_CR) = 0x18 ;		
	while (WIFI_REGISTER(WIFI_RFSIO_STATUS) & 1) ;
	WIFI_REGISTER(WIFI_RFSIO_DATA1) = (data & 0xFFFF) ;
	WIFI_REGISTER(WIFI_RFSIO_DATA2) = (data >> 16) ;
	while (WIFI_REGISTER(WIFI_RFSIO_STATUS) & 1) ;
}

int wifi_compare_mac(volatile void * mac1,volatile  void * mac2) {
	return (((u16 *)mac1)[0]==((u16 *)mac2)[0]) && (((u16 *)mac1)[1]==((u16 *)mac2)[1]) && (((u16 *)mac1)[2]==((u16 *)mac2)[2]);
}

void wifi_copy_mac(volatile void * dest, volatile void * src) {
	((u16 *)dest)[0]=((u16 *)src)[0];
	((u16 *)dest)[1]=((u16 *)src)[1];
	((u16 *)dest)[2]=((u16 *)src)[2];
}

//////////////////////////////////////////////////////////////////////////
//
//  Flash support functions
//
char FlashData[512];

void hw_init_flash_data() {
	readFirmware(0,FlashData,512);
}

int hw_read_flash_byte(int address) {
	if(address<0 || address>511) return 0;
	return FlashData[address];		
}

int hw_read_flash_bytes(int address, int numbytes) {
	int dataout=0;
	int i;
	for(i=0;i<numbytes;i++) {
		dataout |= hw_read_flash_byte(i+address)<<(i*8);
	}
	return dataout;
}
int hw_read_flash_short(int address) {
	if(address<0 || address>510) return 0;
	return hw_read_flash_bytes(address,2);
}

