/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCBLOOD_H
#define __DPCBLOOD_H

#ifndef __DMGMODEL_H
#include <dmgmodel.h>
#endif // !__DMGMODEL_H

extern void DPCBloodInit(void);
extern void DPCBloodTerm(void);
extern void DPCReleaseBlood(const sDamageMsg* msg);

#endif // __DPCBLOOD_H
