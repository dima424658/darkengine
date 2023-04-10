/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phreport.cpp,v 1.10 1999/08/05 17:24:29 Justin Exp $
//
// Physics report generation
//

#ifdef REPORT

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <lazyagg.h>
#include <traitman.h>

#include <hashpp.h>
#include <hashfunc.h>
#include <hshpptem.h>

#include <report.h>
#include <objtype.h>
#include <objedit.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phprop.h>
#include <phreport.h>

#include <contain.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

static LazyAggMember(ITraitManager) pTraitMan;

void PhysCheckForErrors(ObjID objID, char *buffer);
void PhysCheckForWarnings(ObjID objID, char *buffer);
void PhysCheckForSpew(ObjID objID, char *buffer);


// okay, so we maintain these two tables, which basically just let
// us look up by archetype ID how many bad fellows we have found
// of this variety, and what the set of all bad things that we
// have found.
#define WARNFLAG_TYPE      0x01
#define WARNFLAG_ATTRIB    0x02
#define WARNFLAG_STATE     0x04
#define WARNFLAG_CONTROL   0x08
#define WARNFLAG_DIMS      0x10
#define WARNFLAG_NOPHYS    0x20

typedef cHashTableFunctions<ObjID> PhysCountHashFunctions;
typedef cHashTable<ObjID, int, PhysCountHashFunctions> cPhysCountTable;

#ifdef _MSC_VER
template cPhysCountTable;
#endif

static cPhysCountTable count_table;

typedef cHashTableFunctions<ObjID> PhysFlagsHashFunctions;
typedef cHashTable<ObjID, int, PhysFlagsHashFunctions> cPhysFlagsTable;

#ifdef _MSC_VER
template cPhysFlagsTable;
#endif

static cPhysFlagsTable flags_table;

////////////////////////////////////////////////////////////////////////////////

void PhysReportObjCallback(ObjID objID, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   switch (phase)
   {
      case kReportPhase_Init:
      {
         rsprintf(&buffer, "\nPhysics\n");
         rsprintf(&buffer, "-------\n");

         count_table.Clear();
         flags_table.Clear();
         break;
      }

      case kReportPhase_Loop:
      {
         switch (WarnLevel)
         {
            case kReportLevel_DumpAll:
            {
               PhysCheckForSpew(objID, buffer);
            }

            case kReportLevel_Info:
            {
            }

            case kReportLevel_Warning:
            {
               PhysCheckForWarnings(objID, buffer);
            }

            case kReportLevel_Errors:
            {
               PhysCheckForErrors(objID, buffer);
            }
         }

         break;
      }

      case kReportPhase_Term:
      {
         cPhysCountTable::cIter hashiter;
         int count = 0;
         int flags;

         hashiter = count_table.Iter();
         while (!hashiter.Done())
         {
            if (hashiter.Value() > 0)
            {
               count++;

               // okay, we have at least one bad instance of this thing
               flags = flags_table.Search(hashiter.Key());
               rsprintf(&buffer, "WARNING: %d of \'%s\' have conflicting phys props, including: ", 
                        hashiter.Value(),ObjEditName(hashiter.Key()));
               if (flags & WARNFLAG_TYPE)
               {
                  rsprintf(&buffer, "TYPE ");
               }
               if (flags & WARNFLAG_ATTRIB)
               {
                  rsprintf(&buffer, "ATTRIB ");
               }
               if (flags & WARNFLAG_STATE)
               {
                  rsprintf(&buffer, "STATE "); 
               }
               if (flags & WARNFLAG_CONTROL)
               {
                  rsprintf(&buffer, "CONTROL "); 
               }
               if (flags & WARNFLAG_DIMS)
               {
                  rsprintf(&buffer, "DIMS ");
               }
               if (flags & WARNFLAG_NOPHYS)
               {
                  rsprintf(&buffer, "NOPHYS ");
               }
               rsprintf(&buffer,"\n");
            }
            hashiter.Next();
         }

         if (count > 0)
            rsprintf(&buffer,"See full info report for specific obj IDs if needed.\n");
         rsprintf(&buffer, "-------\n");

         break;
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void InitPhysReports()
{
   ReportRegisterObjCallback(PhysReportObjCallback, "Physics System", NULL);
}

void TermPhysReports()
{
   ReportUnRegisterObjCallback(PhysReportObjCallback, NULL);
}

////////////////////////////////////////////////////////////////////////////////

void PhysCheckForErrors(ObjID objID, char *buffer)
{
   if (OBJ_IS_CONCRETE(objID))
   {
      // Check that if we have the physics property, we also have physics
      if (g_pPhysTypeProp->IsRelevant(objID) ^ PhysObjHasPhysics(objID))
      {
         rsprintf(&buffer, "ERROR: \'%s\' has out-of-sync physics!\n", ObjEditName(objID));
      }

      cPhysModel *pModel;

      pModel = g_PhysModels.GetInactive(objID);

      if (pModel)
      {
         rsprintf(&buffer, "ERROR: \'%s\' has an inactive physics model!\n", ObjEditName(objID));
      }

      pModel = g_PhysModels.Get(objID);
      if (pModel)
      {
         if (!PhysObjValidPos(objID, NULL))
            rsprintf(&buffer, "ERROR: \'%s\' is placed out of the world!\n", ObjEditName(objID));

         if (!pModel->IsRope() && (pModel->GetType(0) != kPMT_Point))
         {
            cPhysDimsProp *pDimsProp=NULL;
            g_pPhysDimsProp->Get(objID, &pDimsProp);
            if (pDimsProp)
            {  // make sure no objs with dims 0
               int k, m;
               BOOL a_ok=FALSE;
               for (k=0; k<MAX_PROP_SUBMODELS; k++)
               {
                  a_ok|=pDimsProp->radius[k]!=0;
                  for (m=0; m<3; m++)
                     a_ok|=(pDimsProp->offset[k].el[m]!=0);
               }
               for (m=0; m<3; m++)
                  a_ok|=pDimsProp->size.el[m]!=0;
               if (!a_ok)
                  rsprintf(&buffer, "ERROR: \'%s\' has zero dimensions!\n", ObjEditName(objID));
            }
         }

         if ((pModel->GetRestAxes() != NoAxes) && !pModel->IsRotationControlled())
         {
            for (int i=0; i<pModel->NumSubModels(); i++)
            {
               if (mx_mag2_vec(&pModel->GetSubModOffset(i)) > 0.0)
               {
                  rsprintf(&buffer, "ERROR: \'%s\' has both rest axes and submodel offsets!\n", ObjWarnName(objID));
                  break;
               }
            }
         }
      }
   }
}

////////////////////////////////////////
void PhysCheckForSpew(ObjID objID, char *buffer)
{
   if (OBJ_IS_CONCRETE(objID))
   {
      BOOL has_phys = g_pPhysTypeProp->IsRelevant(objID);
      BOOL arch_has_phys = FALSE;

      ObjID arch = pTraitMan->GetArchetype(objID);
      AutoAppIPtr(ContainSys);

      while (arch != OBJ_NULL)
      {
         if (g_pPhysTypeProp->IsRelevant(arch))
         {
            arch_has_phys = TRUE;

            if (!has_phys && (pContainSys->GetContainer(objID) == OBJ_NULL))
            {
               cPhysTypeProp *pTypeProp;
               g_pPhysTypeProp->Get(arch, &pTypeProp);

               if (pTypeProp->type != kNoneProp)
               {
                  rsprintf(&buffer, "WARNING: \'%s\' has no physics, but archetype (%d) does.\n", 
                           ObjEditName(objID), arch);
               }
            }
            break;
         }

         arch = pTraitMan->GetArchetype(arch);
      }

      if (has_phys && !arch_has_phys)
      {
         arch = pTraitMan->GetArchetype(objID);
         rsprintf(&buffer, "WARNING: \'%s\' has physics, but archetype (%d) does not.\n", 
            ObjEditName(objID), arch);
         return;
      }

      // Check that archetype matches physics
      if (has_phys && arch_has_phys)
      {
         // Type property
         cPhysTypeProp *pConcTypeProp;
         cPhysTypeProp *pArchTypeProp;
         cPhysTypeProp fakeOBBTypeProp;
         HRESULT result;

         result = g_pPhysTypeProp->Get(objID, &pConcTypeProp);
         Assert_(SUCCEEDED(result));
         result = g_pPhysTypeProp->Get(arch, &pArchTypeProp);
         Assert_(SUCCEEDED(result));

         // if we're an OBB, we should always have 6 submodels, so just go slam that in the archetype
         if (pArchTypeProp->type == kOBBProp)
         {
            fakeOBBTypeProp = *pArchTypeProp;
            fakeOBBTypeProp.num_submodels = 6;
            pArchTypeProp = &fakeOBBTypeProp;
         }

         if (memcmp(pConcTypeProp, pArchTypeProp, sizeof(cPhysTypeProp)) != 0)
         {
            rsprintf(&buffer, "Note: \'%s\' has conflicting Type props (w/arch %d)\n", 
                     ObjEditName(objID), arch);
         }

         // Attributes property
         cPhysAttrProp *pConcAttrProp;
         cPhysAttrProp *pArchAttrProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysAttrProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysAttrProp->Get(objID, &pConcAttrProp);
            g_pPhysAttrProp->Get(arch, &pArchAttrProp);

            if (memcmp(pConcAttrProp, pArchAttrProp, sizeof(cPhysAttrProp)) != 0)
            {
               rsprintf(&buffer, "Note: \'%s\' has conflicting Attribute props (w/arch %d)\n",
                        ObjEditName(objID), arch);
            }
         }

         // State property
         cPhysStateProp *pConcStateProp;
         cPhysStateProp *pArchStateProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysStateProp->IsRelevant(arch))
                break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysStateProp->Get(objID, &pConcStateProp);
            g_pPhysStateProp->Get(arch, &pArchStateProp);

            if (memcmp(pConcStateProp, pArchStateProp, sizeof(cPhysStateProp)) != 0)
            {
               rsprintf(&buffer, "Note: \'%s\' has conflicting State props (w/arch %d)\n",
                        ObjEditName(objID), arch);
            }
         }

         // Controls property
         cPhysControlProp *pConcCtrlProp;
         cPhysControlProp *pArchCtrlProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysControlProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysControlProp->Get(objID, &pConcCtrlProp);
            g_pPhysControlProp->Get(arch, &pArchCtrlProp);

            if (memcmp(pConcCtrlProp, pArchCtrlProp, sizeof(cPhysControlProp)) != 0)
            {
               rsprintf(&buffer, "Note: \'%s\' has conflicting Control props (w/arch %d)\n",
                        ObjEditName(objID), arch);
            }
         }

         // Dimensions property
         cPhysDimsProp *pConcDimsProp;
         cPhysDimsProp *pArchDimsProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysDimsProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysDimsProp->Get(objID, &pConcDimsProp);
            g_pPhysDimsProp->Get(arch, &pArchDimsProp);

            if (memcmp(pConcDimsProp, pArchDimsProp, sizeof(cPhysDimsProp)) != 0)
            {
               rsprintf(&buffer, "Note: \'%s\' has conflicting Dimension props (w/arch %d)\n",
                        ObjEditName(objID), arch);
            }
         }
      }
   }
}

void AddTableData(ObjID arch, int flags)
{
   int curval, curflags;

   curval = count_table.Search(arch);
   count_table.Set(arch,curval + 1);

   curflags = flags_table.Search(arch);
   flags_table.Set(arch,curflags | flags);
}

void PhysCheckForWarnings(ObjID objID, char *buffer)
{
   if (OBJ_IS_CONCRETE(objID))
   {
      // Check that it has physics, if its archetypes have physics
      BOOL has_phys = g_pPhysTypeProp->IsRelevant(objID);
      BOOL arch_has_phys = FALSE;
      AutoAppIPtr(ContainSys);

      ObjID arch = pTraitMan->GetArchetype(objID);
      while (arch != OBJ_NULL)
      {
         if (g_pPhysTypeProp->IsRelevant(arch))
         {
            arch_has_phys = TRUE;

            if (!has_phys && (pContainSys->GetContainer(objID) == OBJ_NULL))
            {
               cPhysTypeProp *pTypeProp;
               g_pPhysTypeProp->Get(arch, &pTypeProp);

               if (pTypeProp->type != kNoneProp)
               {
                  AddTableData(arch,WARNFLAG_NOPHYS);
                  //rsprintf(&buffer, "WARNING: \'%s\' has no physics, but archetype (%d) does.\n", 
                           //ObjEditName(objID), arch);
               }
            }
            break;
         }

         arch = pTraitMan->GetArchetype(arch);
      }

      /*
      // this warning removed since all monsters have this
      if (has_phys && !arch_has_phys)
      {
         rsprintf(&buffer, "WARNING: \'%s\' has physics, but no archetypes do.\n", ObjEditName(objID));
      }
      */

      // Check that archetype matches physics
      // note that there are a lot of these when they occur
      // so we just fill out some info on them.
      if (has_phys && arch_has_phys)
      {
         // Type property
         cPhysTypeProp *pConcTypeProp;
         cPhysTypeProp *pArchTypeProp;
         cPhysTypeProp fakeOBBTypeProp;

         g_pPhysTypeProp->Get(objID, &pConcTypeProp);
         g_pPhysTypeProp->Get(arch, &pArchTypeProp);

         // if we're an OBB, we should always have 6 submodels, so just go slam that in the archetype
         if (pArchTypeProp->type == kOBBProp)
         {
            fakeOBBTypeProp = *pArchTypeProp;
            fakeOBBTypeProp.num_submodels = 6;
            pArchTypeProp = &fakeOBBTypeProp;
         }

         if (memcmp(pConcTypeProp, pArchTypeProp, sizeof(cPhysTypeProp)) != 0)
         {
            AddTableData(arch,WARNFLAG_TYPE);
         }

         // Attributes property
         cPhysAttrProp *pConcAttrProp;
         cPhysAttrProp *pArchAttrProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysAttrProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysAttrProp->Get(objID, &pConcAttrProp);
            g_pPhysAttrProp->Get(arch, &pArchAttrProp);

            if (memcmp(pConcAttrProp, pArchAttrProp, sizeof(cPhysAttrProp)) != 0)
            {
               AddTableData(arch,WARNFLAG_ATTRIB);
            }
         }

         // State property
         cPhysStateProp *pConcStateProp;
         cPhysStateProp *pArchStateProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysStateProp->IsRelevant(arch))
                break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysStateProp->Get(objID, &pConcStateProp);
            g_pPhysStateProp->Get(arch, &pArchStateProp);

            if (memcmp(pConcStateProp, pArchStateProp, sizeof(cPhysStateProp)) != 0)
            {
               AddTableData(arch,WARNFLAG_STATE);
            }
         }

         // Controls property
         cPhysControlProp *pConcCtrlProp;
         cPhysControlProp *pArchCtrlProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysControlProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysControlProp->Get(objID, &pConcCtrlProp);
            g_pPhysControlProp->Get(arch, &pArchCtrlProp);

            if (memcmp(pConcCtrlProp, pArchCtrlProp, sizeof(cPhysControlProp)) != 0)
            {
               AddTableData(arch,WARNFLAG_CONTROL);
            }
         }

         // Dimensions property
         cPhysDimsProp *pConcDimsProp;
         cPhysDimsProp *pArchDimsProp;

         arch = pTraitMan->GetArchetype(objID);
         while (arch != OBJ_NULL)
         {
            if (g_pPhysDimsProp->IsRelevant(arch))
               break;

            arch = pTraitMan->GetArchetype(arch);
         }

         if (arch != OBJ_NULL)
         {
            g_pPhysDimsProp->Get(objID, &pConcDimsProp);
            g_pPhysDimsProp->Get(arch, &pArchDimsProp);

            if (memcmp(pConcDimsProp, pArchDimsProp, sizeof(cPhysDimsProp)) != 0)
            {
               AddTableData(arch,WARNFLAG_DIMS);
            }
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

#endif


