/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasatm.cpp,v 1.2 2000/02/19 12:17:23 toml Exp $
//
//
//

#include <lg.h>

#include <aibasatr.h>
#include <aibasabl.h>

#include <aibasctm.h>
#include <aibasatm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#if defined(_MSC_VER)
template cAIActorBase<IAIActor, &IID_IAIActor>;
template cAIActorBase<IAIAbility, &IID_IAIAbility>;
#endif
