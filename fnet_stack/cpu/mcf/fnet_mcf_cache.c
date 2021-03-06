/**************************************************************************
* 
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
**********************************************************************/ /*!
*
* @file fnet_mcf_cache.c
*
* @author Andrey Butok
*
* @date Aug-2-2012
*
* @version 0.1.10.0
*
* @brief ColdFire cache invalidation.
*
***************************************************************************/

#include "fnet.h" 

#if FNET_MCF


#if FNET_OS == 0
/************************************************************************
* NAME: fnet_cpu_cache_invalidate
*
* DESCRIPTION: Invalidate cache.
*************************************************************************/
#if FNET_CFG_CPU_CACHE
    void fnet_cpu_cache_invalidate()
    {
        fnet_mcf_cacr_wr((0|FNET_CFG_MCF_CACHE_CACR|FNET_MCF_CACR_CINV));     
    }
#else
    void fnet_cpu_cache_invalidate()
    {}   
#endif    

#endif 
        
#endif /*FNET_MCF*/
