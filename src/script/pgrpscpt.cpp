/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/pgrpscpt.cpp,v 1.2 1998/09/21 23:17:29 CCAROLLO Exp $
//

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <pgrpprop.h>
#include <pgrpscpt.h>

// Must be last header
#include <dbmem.h>


DECLARE_SCRIPT_SERVICE_IMPL(cPGroupSrv, PGroup)
{
public:
   STDMETHOD(SetActive)(ObjID PGroupObjID, BOOL active)
   {
      ParticleGroup *pg = ObjGetParticleGroup(PGroupObjID);

      if (pg != NULL)
         pg->active = active;

      return TRUE;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPGroupSrv, PGroup);





