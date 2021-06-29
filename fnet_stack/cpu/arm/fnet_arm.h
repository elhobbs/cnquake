#ifndef _FNET_ARM_H_

#define _FNET_ARM_H_


#include "fnet_config.h"

#if FNET_ARM

/*********************************************************************
*
* The basic data types.
*
*********************************************************************/
typedef unsigned char fnet_uint8;       /*  8 bits */

typedef unsigned short int fnet_uint16; /* 16 bits */
typedef unsigned long int fnet_uint32;  /* 32 bits */

typedef signed char fnet_int8;          /*  8 bits */
typedef signed short int fnet_int16;    /* 16 bits */
typedef signed long int fnet_int32;     /* 32 bits */

typedef volatile fnet_uint8 fnet_vuint8;     /*  8 bits */
typedef volatile fnet_uint16 fnet_vuint16;   /* 16 bits */
typedef volatile fnet_uint32 fnet_vuint32;   /* 32 bits */

#endif

#endif
