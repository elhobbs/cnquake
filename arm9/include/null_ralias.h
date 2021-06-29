#ifndef __NULL_RALIAS_H__
#define __NULL_RALIAS_H__

#define floattodsv16(n)       ((int)((n) * (1 << 2)))
#define BYTE_OFFSET(_n,_o) ( ((byte *)(_n)) + (_o) )

void R_DrawViewModel (void);
extern cvar_t	r_drawentities;

#endif
