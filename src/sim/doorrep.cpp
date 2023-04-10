/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/doorrep.cpp,v 1.1 1998/10/03 16:44:38 CCAROLLO Exp $
//
// Physics report generation
//

#ifdef REPORT

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <lazyagg.h>
#include <traitman.h>

#include <report.h>
#include <objedit.h>

#include <doorrep.h>
#include <doorprop.h>

// Must be last header
#include <dbmem.h>

static LazyAggMember(ITraitManager) pTraitMan;

void DoorCheckForErrors(ObjID objID, char *buffer);
void DoorCheckForWarnings(ObjID objID, char *buffer);

void DoorReportObjCallback(ObjID objID, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   switch (phase)
   {
      case kReportPhase_Init:
      {
         rsprintf(&buffer, "\nDoors\n");
         rsprintf(&buffer, "-----\n");

         break;
      }

      case kReportPhase_Loop:
      {
         switch (WarnLevel)
         {
            case kReportLevel_DumpAll:
            {

            }

            case kReportLevel_Info:
            {
         
            }

            case kReportLevel_Warning:
            {
               DoorCheckForWarnings(objID, buffer);
            }

            case kReportLevel_Errors:
            {
               DoorCheckForErrors(objID, buffer);
            }
         }

         break;
      }

      case kReportPhase_Term:
      {
         rsprintf(&buffer, "-----\n");

         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void InitDoorReports()
{
   ReportRegisterObjCallback(DoorReportObjCallback, "Doors", NULL);
}

void TermDoorReports()
{
   ReportUnRegisterObjCallback(DoorReportObjCallback, NULL);
}

////////////////////////////////////////////////////////////////////////////////

void DoorCheckForErrors(ObjID objID, char *buffer)
{

}

////////////////////////////////////////

void DoorCheckForWarnings(ObjID objID, char *buffer)
{
   if (OBJ_IS_CONCRETE(objID))
   {
      BOOL has_rot_door = g_pRotDoorProperty->IsRelevant(objID);
      BOOL has_trans_door = g_pTransDoorProperty->IsRelevant(objID);

      BOOL arch_has_rot_door = FALSE;
      BOOL arch_has_trans_door = FALSE;

      ObjID arch = pTraitMan->GetArchetype(objID);
      while (arch != OBJ_NULL)
      {
         if (g_pRotDoorProperty->IsRelevant(arch))
         {
            arch_has_rot_door = TRUE;

            if (has_trans_door)
            {
               rsprintf(&buffer, "WARNING: \'%s\' is translating door, but archetype (%s) is rotating\n", 
                        ObjWarnName(objID), ObjWarnName(arch));
            }

            if (!has_trans_door && !has_rot_door)
            {
               rsprintf(&buffer, "WARNING \'%s\' is not a door, but archetype (%s) is\n",
                        ObjWarnName(objID), ObjWarnName(arch));
            }

            break;
         }

         if (g_pTransDoorProperty->IsRelevant(arch))
         {
            arch_has_trans_door = TRUE;

            if (has_rot_door)
            {
               rsprintf(&buffer, "WARNING: \'%s\' is rotating door, but archetype (%s) is translating\n",
                        ObjWarnName(objID), ObjWarnName(arch));
            }

            if (!has_rot_door && !has_trans_door)
            {
               rsprintf(&buffer, "WARNING: \'%s\' is not a door, but archetype (%s) is\n",
                        ObjWarnName(objID), ObjWarnName(arch));
            }

            break;
         }

         arch = pTraitMan->GetArchetype(arch);
      }

      if (has_rot_door && !arch_has_rot_door)
      {
         rsprintf(&buffer, "WARNING: \'%s\' is a rotating door, but no archetype are\n", ObjWarnName(objID));
      }

      if (has_trans_door && !arch_has_trans_door)
      {
         rsprintf(&buffer, "WARNING: \'%s\' is a translating door, but no archetypes are\n", ObjWarnName(objID));
      }

      sRotDoorProp *pRotDoorProp = NULL;
      sRotDoorProp *pArchRotDoorProp = NULL;

      sTransDoorProp *pTransDoorProp = NULL;
      sTransDoorProp *pArchTransDoorProp = NULL;

      sDoorProp *pDoorProp = NULL;
      sDoorProp *pArchDoorProp = NULL;

      if (has_rot_door && arch_has_rot_door)
      {
         g_pRotDoorProperty->Get(objID, &pRotDoorProp);
         g_pRotDoorProperty->Get(arch, &pArchRotDoorProp);

         pDoorProp = pRotDoorProp;
         pArchDoorProp = pArchRotDoorProp;

         Assert_(pDoorProp);
         Assert_(pArchDoorProp);
      }

      if (has_trans_door && arch_has_trans_door)
      {
         g_pTransDoorProperty->Get(objID, &pTransDoorProp);
         g_pTransDoorProperty->Get(arch, &pArchTransDoorProp);

         pDoorProp = pTransDoorProp;
         pArchDoorProp = pArchTransDoorProp;

         Assert_(pDoorProp);
         Assert_(pArchDoorProp);
      }

      if (pDoorProp && pArchDoorProp)
      {
         if (pDoorProp->base_speed != pArchDoorProp->base_speed)
         {
            rsprintf(&buffer, "WARNING: \'%s\' has conflicing base speed with archetype \'%s\'\n",
                     ObjWarnName(objID), ObjWarnName(arch));
         }

         if (pDoorProp->axis != pArchDoorProp->axis)
         {
            rsprintf(&buffer, "WARNING: \'%s\' has conflicing axis with archetype \'%s\'\n",
                     ObjWarnName(objID), ObjWarnName(arch));
         }

         if (pDoorProp->vision_blocking != pArchDoorProp->vision_blocking)
         {
            rsprintf(&buffer, "WARNING: \'%s\' has conflicting vision blocking with archetype \'%s\'\n",
                     ObjWarnName(objID), ObjWarnName(arch));
         }

         if (pDoorProp->sound_blocking != pArchDoorProp->sound_blocking)
         {
            rsprintf(&buffer, "WARNING: \'%s\' has conflicing sound blocking with archetype \'%s\'\n",
                     ObjWarnName(objID), ObjWarnName(arch));
         }

         if (pDoorProp->push_mass != pArchDoorProp->push_mass)
         {
            rsprintf(&buffer, "WARNING: \'%s\' has conflicing push mass with archetype \'%s\'\n",
                     ObjWarnName(objID), ObjWarnName(arch));
         }
      }
   }
}


////////////////////////////////////////////////////////////////////////////////

#endif



