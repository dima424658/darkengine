/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/questprp.cpp,v 1.4 1999/06/02 16:55:22 XEMU Exp $
#include <questprp.h>
#include <propert_.h>
#include <propbase.h>
#include <dataops_.h>
#include <propstor.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <questapi.h>
#include <str.h>
#include <appagg.h>
#include <objquery.h>

// Include these last! 
#include <dbmem.h>
#include <initguid.h>
#include <qvpropid.h>

//
// Property Class
//

typedef cGenericProperty<IQuestVarProperty,&IID_IQuestVarProperty,tQVarVal> cQVarPropBase ; 

typedef cAutoIPtr<IPropertyStore> cPS; 

static cSimpleDataOps gOps; 


class cQVarProp : public cQVarPropBase
{
public:
   cQVarProp(const char* var, const sPropertyDesc* desc, ePropertyImpl impl)
      : cQVarPropBase(desc,cPS(CreateGenericPropertyStore(impl)),&gOps), 
        mVarName(var), 
        mpQuestData(AppGetObj(IQuestData))
   {
   }

   ~cQVarProp()
   {
      SafeRelease(mpQuestData); 
   }

   STANDARD_DESCRIBE_TYPE(tQVarVal); 

   STDMETHOD_(BOOL,Matches)(ObjID obj)
   {
      int qval = mpQuestData->Get(mVarName); // defaults to zero 
      int oval = 0; 
      Get(obj,&oval); 
      return oval & (1 << qval); 
   }
   
   STDMETHOD_(BOOL,Matches)(ObjID obj, int qval)
   {
      int oval = 0; 
      Get(obj,&oval); 
      return oval & (1 << qval); 
   }

   STDMETHOD_(IObjectQuery*,QueryAllMatches)(THIS_ eQVarMatch match); 


protected:
   cStr mVarName;  
   IQuestData* mpQuestData; 
   
}; 

//
// OBJECT QUERY CLASS 
//


class cQuestObjQuery: public cCTUnaggregated<IObjectQuery,&IID_IObjectQuery,kCTU_Default>
{
protected:
   IQuestVarProperty* prop; 
   sPropertyObjIter iter; 
   int qmask; 
   BOOL match; 
   ObjID obj; 
   
   // skip to the next match 
   void Skip()
   {
      int oval; 
      while(prop->IterNextValue(&iter,&obj,&oval))
         if (match == ((oval & qmask) != 0))
            return; 
      obj = OBJ_NULL; 
   }

public: 
   cQuestObjQuery(IQuestVarProperty* p, BOOL m, int qv)
   : match(m != 0), prop(p), qmask(1 << qv)
   {
      prop->AddRef(); 
      prop->IterStart(&iter); 
      Skip(); 
   }

   ~cQuestObjQuery()
   {
      prop->IterStop(&iter); 
      SafeRelease(prop); 
   }

   STDMETHOD_(BOOL,Done)() 
   {
      return obj == OBJ_NULL; 
   }

   STDMETHOD_(ObjID,Object)()
   {
      return obj; 
   }

   STDMETHOD(Next)()
   {
      if (obj != OBJ_NULL)
         Skip(); 
      return S_OK; 
   }
}; 

//
// Query Factory Method
//

STDMETHODIMP_(IObjectQuery*) cQVarProp::QueryAllMatches(THIS_ eQVarMatch match)
{
   int qv = mpQuestData->Get(mVarName); 
   return new cQuestObjQuery(this, match == kQVarMatches, qv);
}

//
// The world's dumbest sdesc
// 

static const char* numbers[] = { 
   "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", 
   //   "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", 
   //   "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", 
   //   "30", "31"
};

#define NUM_NUMBERS (sizeof(numbers)/sizeof(numbers[0]))

static sFieldDesc qvar_fields[] = 
{
   { "quest var values", kFieldTypeBits, sizeof(tQVarVal), 0,  kFieldFlagNone, 0, NUM_NUMBERS, NUM_NUMBERS, numbers },
}; 

static sStructDesc qvar_sdesc = StructDescBuild(tQVarVal,kStructFlagNone,qvar_fields); 

//
// Property Factory 
//
IQuestVarProperty* CreateQuestVarProperty(const char* qvar, const sPropertyDesc* desc, ePropertyImpl impl)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&qvar_sdesc); 
   return new cQVarProp(qvar,desc,impl); 
} 





