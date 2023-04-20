/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mnamprop.cpp,v 1.13 1998/12/07 16:50:43 dc Exp $

//
// Model name property
//

#include <mnamprop.h>
#include <mnumprop.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propfac_.h>

#include <osysbase.h>

#include <appagg.h>

#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>

#include <objmodel.h>
#include <mprintf.h>

#include <report.h>

#ifdef REPORT

#include <dlist.h>
#include <iobjsys.h>
#include <reputil.h>

#endif

#include <dbmem.h>

static ILabelProperty* modelnameprop = NULL;

////////////////////////////////////////////////////////////
// MODEL NAME PROPERTY CREATION 
//

#define MODELNAMEPROP_IMPL kPropertyImplDense

static sPropertyConstraint modelnameprop_const[] =
{
   { kPropertyNullConstraint, NULL }
};

static sPropertyDesc modelnameprop_desc =
{
   PROP_MODELNAME_NAME,
   0,
   modelnameprop_const,
   0,0, // verison
   { "Shape", "Model Name" }, 
};


class cModelNameProp : public cGenericLabelProperty
{
public:
   cModelNameProp() : cGenericLabelProperty(&modelnameprop_desc,MODELNAMEPROP_IMPL)
   {
      SetRebuildConcretes(TRUE); 
   }; 

   void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val , ObjID donor)
   {
      if (fIsRelevant)
         ObjLoadModel(donor);
      cGenericLabelProperty::RebuildConcrete(obj,fIsRelevant,val,donor); 
   }

};

// Reporting

#ifdef REPORT

// list for sorting names
class cModelNameNode: public cDListNode<cModelNameNode,1>
{
public:
   Label m_modelName;
   ObjID m_objID;
};

class cModelNameList: public cDList<cModelNameNode,1>
{
public:
   cModelNameNode *Find(const char *pModelName) const
   {
      cModelNameNode *pNode = GetFirst();
      
      while (pNode != NULL)
      {
         if (!strcmp((const char*)(pNode->m_modelName.text), pModelName))
            return pNode;
         pNode = pNode->GetNext();
      }
      return NULL;
   }
   void InsertSorted(cModelNameNode *pInsertNode)
   {
      cModelNameNode *pNode = GetFirst();
      while (pNode != NULL)
      {
         if (stricmp((const char*)(pNode->m_modelName.text), (const char*)(pInsertNode->m_modelName.text))>0)
         {
            InsertBefore(pNode, pInsertNode);
            break;
         }
         pNode = pNode->GetNext();
      }
      if (pNode == NULL)
         Append(pInsertNode);
   }
};

// Report prints out all models used by abstract objects
static void ModelNameReport(int WarnLevel, void *data, char *buffer)
{
   IObjectQuery* pQuery;
   IObjectSystem* pOS = AppGetObj(IObjectSystem);
   cModelNameList modelNameList;
   cModelNameNode* pModelNameNode;
   cModelNameNode* pNext;
   ObjID obj;
   int count = 0;

   rsprintf(&buffer,"\nReport on Object Model Names\n");

   // okay, sift through every object in the world and collect stats
   pQuery = pOS->Iter(kObjectAbstract);
   pModelNameNode = new cModelNameNode;
   while (!pQuery->Done())
   {
      obj = pQuery->Object();
      if (ObjGetModelName(obj, (char *)(pModelNameNode->m_modelName.text)))
      {
         pModelNameNode->m_objID = obj;
         // count unique
         if (!modelNameList.Find(pModelNameNode->m_modelName.text))
            ++count;
         // add sorted
         modelNameList.InsertSorted(pModelNameNode);
         pModelNameNode = new cModelNameNode;
      }
      pQuery->Next();
   }
   delete pModelNameNode;

   // now print out
   rsprintf(&buffer, "Total models used: %d\n\n", count);
   pModelNameNode = modelNameList.GetFirst();
   while (pModelNameNode != NULL)
   {
      rsprintf(&buffer, "Model %s used by obj %s\n", pModelNameNode->m_modelName.text, pOS->GetName(pModelNameNode->m_objID));
      // move on to next, deleting as we go
      pNext = pModelNameNode->GetNext();
      delete pModelNameNode;
      pModelNameNode = pNext;
   }

   SafeRelease(pQuery);
   SafeRelease(pOS);
}
#endif

// Init the property
void ModelNamePropInit(void)
{
   modelnameprop = new cModelNameProp; 
   ReportRegisterGenCallback(ModelNameReport,kReportModels,"ModelName",NULL);   
}

void ModelNamePropTerm(void)
{
   SafeRelease(modelnameprop);   
   ReportUnRegisterGenCallback(ModelNameReport,kReportModels,NULL);
}

// get and set functions
BOOL ObjGetModelName(ObjID obj, char *name)
{
   Assert_(modelnameprop);
   char *temp;
   BOOL retval = modelnameprop->Get(obj, (Label**)&temp);
   if (retval) // hope name long enough!
      strcpy(name,temp);
   return retval;
}

void ObjSetModelName(ObjID obj, const char *name)
{
   Assert_(modelnameprop);
   modelnameprop->Set(obj, (Label*)name);
}

