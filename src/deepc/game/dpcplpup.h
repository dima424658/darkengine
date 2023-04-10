/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPLPUP_H
#define __DPCPLPUP_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

ObjID PlayerPuppetCreate(const char *pModelName);
void PlayerPuppetDestroy(void);

#endif  // !__DPCPLPUP_H