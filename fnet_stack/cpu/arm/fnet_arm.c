#include "fnet.h"
#include "fnet_cpu.h"
#include <stdio.h>

#ifdef WIN32
#define iprintf printf
#endif

//extern FILE* tx_tcp_output;

#if FNET_ARM

unsigned long fnet_checksum_low1(unsigned long sum, int current_length, unsigned short *d_ptr)
{
        unsigned short p_byte1;
        
        while((current_length -= 32) >= 0)
        {
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
        }
        current_length += 32;

        while((current_length -= 8) >= 0)
        {
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
            sum += *d_ptr++;
        }
        current_length += 8;

        while((current_length -= 2) >= 0)
            sum += *d_ptr++;  
            
        if(current_length += 2)
        {
            p_byte1 = (unsigned short)((*((unsigned short *)d_ptr)) & FNET_NTOHS(0xFF00));
           
            sum += (unsigned short)p_byte1;
        }
        return sum;
} 

unsigned long fnet_checksum_low(unsigned long sum, int current_length, unsigned short *d_ptr)
{
    unsigned short p_byte1;
	//fprintf(tx_tcp_output,"fnet_checksum_low: %08x %04x\r\n",sum,current_length);
    while((current_length -= 2) >= 0) {
		//fprintf(tx_tcp_output,"%04x\r\n",*d_ptr);
        sum += *d_ptr++;  
	}
            
    if(current_length += 2)
    {
        p_byte1 = (unsigned short)((*((unsigned short *)d_ptr)) & FNET_NTOHS(0xFF00));
           
		//fprintf(tx_tcp_output,"%04x\r\n",p_byte1);
        sum += (unsigned short)p_byte1;
    }

	//fprintf(tx_tcp_output,"\r\n%08x\r\n",sum);
	//fflush(tx_tcp_output);

	return sum;
}

unsigned long fnet_checksum_low2(unsigned long sum, int current_length, unsigned short *d_ptr)
{
	unsigned char *data = (unsigned char *)d_ptr;
	unsigned short src;
	//fprintf(tx_tcp_output,"fnet_checksum_low: %08x %04x\r\n",sum,current_length);
#if 1
	while(current_length > 1) {
		//fprintf(tx_tcp_output,"%02x\r\n",*data);
		src = ((*data) << 8);
		data++;
		//fprintf(tx_tcp_output,"%02x\r\n",*data);
		src |= (*data);
		data++;
		sum += src;
		current_length -= 2;
	}

	if(current_length > 0) {
		//fprintf(tx_tcp_output,"%02x\r\n",*data);
		src = ((*data) << 8);
		sum += src;
	}
#else
		while(startbyte+offset+1<mb->thislength && chksum_length>1) {
			chksum_temp+= ((unsigned char *)mb->datastart)[startbyte+offset] + (((unsigned char *)mb->datastart)[startbyte+offset+1]<<8);
			offset+=2;
			chksum_length-=2;
		}
#endif

	//fprintf(tx_tcp_output,"\r\n%08x\r\n",sum);
	//fflush(tx_tcp_output);

	return sum;
}


int fnet_printf(const char *format, ... )
{
    fnet_va_list ap;
    /*
     * Initialize the pointer to the variable length argument list.
     */
    fnet_va_start(ap, format);
    return vprintf(format, ap);  
}

int fnet_println(const char *format, ... )
{
	int len;
    fnet_va_list ap;
    /*
     * Initialize the pointer to the variable length argument list.
     */
    fnet_va_start(ap, format);
    len =  vprintf(format, ap);
	iprintf("\n");
	return len + 1;
}

int fnet_sprintf( char *str, const char *format, ... )
{
    fnet_va_list ap;
    int result = 0;
   
    if(str != 0)
    {
        /*
         * Initialize the pointer to the variable length argument list.
         */
        fnet_va_start(ap, format);
        result = vsprintf(str, format, ap);
    }
    
    return result;
}

int fnet_snprintf( char *str, unsigned int size, const char *format, ... )
{
    fnet_va_list ap;
    int result = 0;
       
    
    if((str != 0) && (size != 0))
    {
        --size; /* Space for the trailing null character.*/
        
        /*
         * Initialize the pointer to the variable length argument list.
         */
        fnet_va_start(ap, format);
        result = vsnprintf(str, size, format, ap);
        if(result > size)
            result = (int)size;
    } 
    return result;
}

void fnet_cpu_irq_enable(fnet_cpu_irq_desc_t irq_desc)
{
    FNET_COMP_UNUSED_ARG(irq_desc);  
    //fnet_mk_irq_enable();
}

int fnet_cpu_isr_install(unsigned int vector_number, unsigned int priority)
{
	return FNET_OK;
}
#endif