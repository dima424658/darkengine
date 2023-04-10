/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/scrpted.h,v 1.1 1998/04/12 17:04:17 mahk Exp $
#pragma once  
#ifndef __SCRPTED_H
#define __SCRPTED_H

#ifdef EDITOR
EXTERN BOOL PickAndDropScript(void);
#else
#define PickAndDropScript() FALSE 
#endif 

#endif // __SCRPTED_H
