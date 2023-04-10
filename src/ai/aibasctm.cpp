/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasctm.cpp,v 1.4 2000/02/19 12:17:25 toml Exp $
//
//
//

#include <lg.h>

#include <aiapi.h>
#include <aiapiabl.h>
#include <aiapiact.h>
#include <aiapibhv.h>
#include <aiapicmp.h>
#include <aiapiiai.h>
#include <aiapimov.h>
#include <aiapisnd.h>
#include <aiapisns.h>
#include <aiapipth.h>

#include <aibascmp.h>

#include <aibasctm.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#if defined(_MSC_VER)
template cAIComponentBase<IAIActor, &IID_IAIActor>;
template cAIComponentBase<IAIAbility, &IID_IAIAbility>;
template cAIComponentBase<IAIComponent, &IID_IAIComponent>;
template cAIComponentBase<IAIMoveEnactor, &IID_IAIMoveEnactor>;
template cAIComponentBase<IAIMoveRegulator, &IID_IAIMoveRegulator>;
template cAIComponentBase<IAISenses, &IID_IAISenses>;
template cAIComponentBase<IAISoundEnactor, &IID_IAISoundEnactor>;
template cAIComponentBase<IAIPathfinder, &IID_IAIPathfinder>;
// template cAIComponentBase<IAISubability, &IID_IAISubability>;
#endif
