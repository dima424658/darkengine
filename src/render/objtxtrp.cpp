/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objtxtrp.cpp,v 1.4 1998/08/24 20:56:59 mahk Exp $
// control for doing replacable textures on objects

#include <lg.h>
#include <appagg.h>
#include <mprintf.h>

#include <storeapi.h>
#include <resapi.h>
#include <imgrstyp.h>

#include <config.h>
#include <cfgdbg.h>

#include <propface.h>
#include <propbase.h>
#include <proptype.h>
#include <objnotif.h>

#include <hashpp.h>
#include <hshpptem.h>

#include <objtxtrp.h>
#include <objedit.h>  // for debugging

#include <strprop_.h>

#include <dbmem.h>

///////////////////
// handle storage group data structure
struct ObjTxtReplaceData {
   IRes *pTxt[MAX_REPL_TXT];
};

typedef cHashTableFunctions<ObjID> ObjIDHashFunctions;
typedef cHashTable<ObjID, ObjTxtReplaceData *, ObjIDHashFunctions> cObjTxtReplaceTable;
typedef cObjTxtReplaceTable::cIter cObjTxtIter;

static cObjTxtReplaceTable g_ObjTxtReplaceTable;

///////////////////
// horrible hacks for the string class which i get RebuildConcrete and Notify for

// Forward Declare these two, so i can use them in the class
void _ClearPtxt(IRes **pTxt);
void _GogetPtxt(IRes **pTxt, const char *txtName);

typedef cAutoIPtr<IPropertyStore> cPS;

// what am i supposed to do here?
class cTextureReplStringProperty: public cGenericStringProperty
{
public:
   
   cTextureReplStringProperty(sPropertyDesc *sdesc, ePropertyImpl impl, int index)
      : cGenericStringProperty(sdesc,cPS(CreateGenericPropertyStore(impl))), m_Index(index)
   {
      SetRebuildConcretes(TRUE);
   }

private:
   ////////////////
   // actual stuff to do things
   void _check_final_state(ObjTxtReplaceData *trData, ObjID obj)
   {
      if (trData)
      {
         for (int i=0; i<MAX_REPL_TXT; i++)
            if (trData->pTxt[i]!=NULL)
               break;
         if (i==MAX_REPL_TXT)
         {  // then there is no data left in this record, so toast it
            g_ObjTxtReplaceTable.Delete(obj);
            delete trData;
            Warning(("Removing txtRepl data for %s\n",ObjEditName(obj)));
         }
      }
   }

   void _add_or_set(ObjID obj)
   {
      const char *newTxtName=NULL;
      ObjTxtReplaceData *txtReplData=NULL;
      // @TODO: is this a safe way to do "Get" for whatever property i am
      if (!Get(obj,&newTxtName))
         Warning(("No name for new texture replace prop for %s index %d\n",ObjEditName(obj),m_Index));
      else
      {
         if (!g_ObjTxtReplaceTable.Lookup(obj,&txtReplData))
         {  // if it doesnt exist, create one
            txtReplData=new ObjTxtReplaceData;
            memset(txtReplData->pTxt,0,sizeof(IRes *)*MAX_REPL_TXT);
            g_ObjTxtReplaceTable.Set(obj,txtReplData);
         }
         if (txtReplData->pTxt[m_Index])
            _ClearPtxt(&txtReplData->pTxt[m_Index]);
         _GogetPtxt(&txtReplData->pTxt[m_Index],newTxtName);
      }
      _check_final_state(txtReplData,obj);
   }

   void _unset_or_del(ObjID obj)
   {
      ObjTxtReplaceData *txtReplData=NULL;      
      if (!g_ObjTxtReplaceTable.Lookup(obj,&txtReplData))
         Warning(("ObjTxtReplace listener called with no hash entry for %s\n",ObjEditName(obj)));
      else
         _ClearPtxt(&txtReplData->pTxt[m_Index]);
      _check_final_state(txtReplData,obj);
   }

   ////////////////
   // overloads
   // i dont need to do this as virtual void, i dont thing....
   void RebuildConcrete(ObjID obj, BOOL fIsRelevant, uPropListenerValue val,ObjID donor)
   {
      // @TODO: is this right syntax, ie. is fIsRel equiv to "do i have this prop at all"
      if (!fIsRelevant)
         _unset_or_del(obj);
      else
         _add_or_set(obj);
      cGenericStringProperty::RebuildConcrete(obj,fIsRelevant,val,donor);
   }

   // do i want to do this if im rebuildconcrete-ing anyway
   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      if (type&kListenPropUnset)
         _unset_or_del(obj);
      else
         _add_or_set(obj);
      cGenericStringProperty::OnListenMsg(type,obj,val);
   }

   // which of these am i, for use in my horrible hash joy
   int m_Index;
};

///////////////////
// Declarations/Creations

#define OBJ_TXT_REPL_PROP_LISTENTO (kListenPropModify|kListenPropSet|kListenPropUnset|kListenPropLoad)
#define OBJ_TXT_REPL_PROP_IMPL     kPropertyImplSparseHash
#define OBJ_TXT_REPL_PROP_DECLARE(str) \
   static sPropertyDesc str##PropDesc = { "OTxtRep"#str, 0, NULL, 0, 0, { "Shape", "TxtRepl "#str }}; \
   static IStringProperty *##str##Prop = NULL
#define OBJ_TXT_REPL_PROP_CREATE(str,index) \
   str##Prop = new cTextureReplStringProperty(&##str##PropDesc,OBJ_TXT_REPL_PROP_IMPL,index)
#define OBJ_TXT_REPL_PROP_RELEASE(str) \
   SafeRelease(str##Prop)

// count much match MAX_REPL_TXT                
OBJ_TXT_REPL_PROP_DECLARE(r0);
OBJ_TXT_REPL_PROP_DECLARE(r1);
OBJ_TXT_REPL_PROP_DECLARE(r2);
OBJ_TXT_REPL_PROP_DECLARE(r3);

///////////////////
// load time instantiation stuff
static void _ClearPtxt(IRes **pTxt)
{
   if ((*pTxt)==NULL)
   {
      ConfigSpew("TxtReplaceSpew",("ObjTxtReplace being asked to ClearPtxt NULL\n"));
      return;
   }
   (*pTxt)->Unlock();
   SafeRelease((*pTxt));
   *pTxt=NULL;
}

static void _GogetPtxt(IRes **pTxt, const char *txtName)
{
   if (txtName[0]=='\0') // null string
   {
      ConfigSpew("TxtReplaceSpew",("ObjTxtReplace GogetPtxt NULL string\n"));
      return;
   }
   AutoAppIPtr(ResMan);
   // should really investigate txtName, check for txt16 and so on, expose from objmodel
   IRes *pRes = pResMan->Bind(txtName, RESTYPE_IMAGE, NULL);
   if (pRes)
      pRes->Lock();
   else
      Warning(("ObjTxtReplace can't seem to load %s as typed\n",txtName));
   *pTxt=pRes;
}

///////////////////
// run time data acquisition stuff

IRes **ObjTxtReplaceForObj(ObjID obj)
{
   ObjTxtReplaceData *txtReplData=NULL;
   if (!g_ObjTxtReplaceTable.Lookup(obj,&txtReplData))
      return NULL;
   else
      return txtReplData->pTxt;
}

///////////////////
// cleanup/access of the data

// hmmm, can we do this by name, maybe
void ObjTxtReplaceDropIRes(IRes *drop_me)
{
   ConfigSpew("TxtReplaceSpew",("Trying to reset TxtRepl for %x\n",drop_me));
   cObjTxtIter fullIter=g_ObjTxtReplaceTable.Iter();
   for (;!fullIter.Done();fullIter.Next())
   {
      ObjTxtReplaceData *trData=fullIter.Value();
      ObjID us=fullIter.Key();
      int i;
      for (i=0; i<MAX_REPL_TXT; i++)
         if (trData->pTxt[i]!=NULL)
            if (drop_me==NULL || trData->pTxt[i]==drop_me)
               _ClearPtxt(&trData->pTxt[i]);
      for (i=0; i<MAX_REPL_TXT; i++)
         if (trData->pTxt[i]!=NULL)
            break;
      if (i==MAX_REPL_TXT)
      {
         g_ObjTxtReplaceTable.Delete(us);
         delete trData;
      }
      else if (drop_me==NULL)
         Warning(("Told to drop all, but failed to for Res %x obj %s\n",drop_me,ObjEditName(us)));
   }
}

void ObjTxtReplaceDropAll(void)
{
   ObjTxtReplaceDropIRes(NULL);
}

////////////////////
// actual initialization and term

void ObjTxtReplaceInit(void)
{
   OBJ_TXT_REPL_PROP_CREATE(r0,0);
   OBJ_TXT_REPL_PROP_CREATE(r1,1);
   OBJ_TXT_REPL_PROP_CREATE(r2,2);
   OBJ_TXT_REPL_PROP_CREATE(r3,3);
}

void ObjTxtReplaceTerm(void)
{
   ObjTxtReplaceDropAll();
   OBJ_TXT_REPL_PROP_RELEASE(r0);
   OBJ_TXT_REPL_PROP_RELEASE(r1);
   OBJ_TXT_REPL_PROP_RELEASE(r2);
   OBJ_TXT_REPL_PROP_RELEASE(r3);
}
