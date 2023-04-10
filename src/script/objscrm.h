/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/objscrm.h,v 1.1 1997/12/31 00:08:42 mahk Exp $
#pragma once  
#ifndef __OBJSCRM_H
#define __OBJSCRM_H

#define OnCreate()      SCRIPT_MESSAGE_HANDLER_SIGNATURE(Create)
#define BaseOnCreate()  SCRIPT_CALL_BASE(Create)

#define OnDestroy()     SCRIPT_MESSAGE_HANDLER_SIGNATURE(Destroy)
#define BaseOnDestroy() SCRIPT_CALL_BASE(Destroy)

#define OBJSYS_MESSAGE_DEFAULTS() \
        SCRIPT_DEFINE_MESSAGE_DEFAULT(Create)  \
        SCRIPT_DEFINE_MESSAGE_DEFAULT(Destroy) 

#define OBJSYS_MESSAGE_MAP_ENTRIES() \
        SCRIPT_MSG_MAP_ENTRY(Create) \
        SCRIPT_MSG_MAP_ENTRY(Destroy)

#endif // __OBJSCRM_H
