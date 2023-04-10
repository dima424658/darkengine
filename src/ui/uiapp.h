/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiapp.h,v 1.4 2000/01/31 10:05:02 adurant Exp $
#pragma once

#ifndef __UIAPP_H
#define __UIAPP_H


EXTERN void LGAPI uiSysCreate(void);
DEFINE_LG_GUID(IID_UI, 0x24);


struct _Region;
EXTERN struct _Region* GetRootRegion(void);


#endif // __UIAPP_H

