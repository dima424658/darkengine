/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __FLREGION_H
#define __FLREGION_H
/*
 * $Source: r:/t2repos/thief2/src/reg.h,v $
 * $Revision: 1.2 $
 * $Author: adurant $
 * $Date: 2000/01/29 12:41:42 $
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
#include <region.h>

// THIS FILE CONTAINS A FLIGHT LAYER ABOVE THE REGION API
// THAT WILL BE MORE COMPATIBLE WITH THE FUTURE(TM)


#define make_region(parent,region,rect,z,data) \
   region_create(parent,region,rect,z,0,REG_USER_CONTROLLED|AUTOMANAGE_FLAG,NULL,NULL,NULL,data);
   
#define set_region_expose(reg,exp) \
   do { \
      (reg)->status_flags |= EXPOSE_CB; \
      (reg)->expose = (exp);  \
   } while(0)


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __FLREGION_H

