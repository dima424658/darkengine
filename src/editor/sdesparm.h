/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/sdesparm.h,v 1.2 1997/12/30 17:50:13 mahk Exp $
#pragma once  
#ifndef __SDESPARM_H
#define __SDESPARM_H

////////////////////////////////////////////////////////////
// SDESC MULTIPARM TOOLS
//

class cMultiParm; 

extern void SetFieldFromParm(const struct sFieldDesc* field, void *struc, const cMultiParm& parm);
extern cMultiParm GetParmFromField(const struct sFieldDesc* field, void *struc); 

extern void FillStructFromParams(const struct sStructDesc* sdesc, void* struc, cMultiParm** params, int nparams); 



#endif // __SDESPARM_H



