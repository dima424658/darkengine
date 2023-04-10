/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/frobprop.cpp,v 1.14 1998/11/19 13:03:13 JUSTIN Exp $
// initial frob property - probably not shipping

// property stuff
#include <propert_.h>
#include <dataops_.h>
#include <propbase.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <propface.h> // For IBoolProperty

// link stuff
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

// frob specific stuff
#include <frobscrt.h>
#include <frobprop.h>

#include <engfeat.h> // for the group name

// Must be last header
#include <dbmem.h>

//////////////////////

struct sOldFrobInfo
{
   uchar actions[3]; 
   uchar pad[1]; 
}; 

class cFrobOps : public cClassDataOps<sFrobInfo>
{
public:
   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* file, int version)
   {
      if (version > sizeof(sOldFrobInfo))
         return cClassDataOps<sFrobInfo>::Read(dat,file,version);
      else 
      {
         sOldFrobInfo old; 
         if (!dat->value) *dat = New(); 
         sFrobInfo& young = *(sFrobInfo*)dat->value; 
         file->Read(&old,sizeof(old)); 
         for (int i = 0; i < 3; i++)
            young.actions[i] = old.actions[i];  
         return S_OK; 
      }
   }
}; 

typedef cGenericProperty<IFrobInfoProperty,&IID_IFrobInfoProperty,sFrobInfo*> cFrobInfoPropertyBase;
typedef cAutoIPtr<IPropertyStore> PS; 

class cFrobInfoProperty : public cFrobInfoPropertyBase
{
   cFrobOps mOps; 

public: 

   cFrobInfoProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cFrobInfoPropertyBase(desc,PS(CreateGenericPropertyStore(impl)))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE(sFrobInfo);
};


char *action_bit_names[] = { "Move", "Script", "Delete", "Ignore", "FocusScript", "Tool Cursor", "Use Ammo", "Default", "Deselect" };

// @TODO: really an array of MAX_FROB_CMDS commands, use define when we have array sdescs
// structure descriptor fun
static sFieldDesc frob_fields [] =
{
   { "World Action", kFieldTypeBits, FieldLocation(sFrobInfo,actions[0]), FullFieldNames(action_bit_names) },
   { "Inv Action",   kFieldTypeBits, FieldLocation(sFrobInfo,actions[1]), FullFieldNames(action_bit_names) },
   { "Tool Action",  kFieldTypeBits, FieldLocation(sFrobInfo,actions[2]), FullFieldNames(action_bit_names) },
};

static sStructDesc frob_info_struct = StructDescBuild(sFrobInfo,kStructFlagNone,frob_fields);
static sPropertyDesc frob_info_propdesc = { PROP_FROB_INFO_NAME, 0, NULL, 0, 0, { ENGFEAT_PROP_GROUP, "FrobInfo" } };

#define FROB_INFO_IMPL kPropertyImplSparseHash
IFrobInfoProperty *pFrobInfoProp = NULL;
IRelation         *pFrobProxyRel = NULL;

sFrobInfo *ObjFrobResult(ObjID obj)
{
   Assert_(pFrobInfoProp);
   sFrobInfo *result = NULL;
   pFrobInfoProp->Get(obj,&result);
   return result;
}

uchar ObjFrobResultForLoc(eFrobLoc loc, ObjID obj)
{
   sFrobInfo *s=ObjFrobResult(obj);
   if (s!=NULL)
      return s->actions[loc];
   return 0;
}

// frob proxy link
#define FROB_RELATION_QCASES (kQCaseSetSourceKnown|kQCaseSetDestKnown)
static sRelationDesc     frobProxyRelDesc  = { "FrobProxy", };
static sRelationDataDesc frobProxyDataDesc = { "FrobProxyInfo", sizeof(int), };
// auto-create ??

static char *proxy_bit_names[]= {"Do Inv", "No World", "Do ToolSrc", "No ToolDst", "AllowDirect"};

static sFieldDesc   frob_proxy_fields[] =
 { { "FrobProxyMask", kFieldTypeBits, sizeof(int), 0, FullFieldNames(proxy_bit_names) }, };

static sStructDesc  frob_proxy_struct =
 { "FrobProxyInfo", sizeof(int), kStructFlagNone,
   sizeof(frob_proxy_fields)/sizeof(frob_proxy_fields[0]), frob_proxy_fields, };

////////////////////////////////////////////////////////////
//
// Frob Handler Property
//

static sPropertyDesc frobhand_prop_desc =
{
   PROP_FROB_HANDLER_NAME,
   0,  // flags
   NULL, // constraints
   0, 0, // Version
   { "Networking", "Frob Handler", }, // ui strings
};

//
// Frob Handler sdesc 
//
static char* frob_handler_strings[] = 
{ 
   "Host",  // Default
   "Local Only",
   "All Machines"
};

#define NUM_FROBHAND_STRINGS (sizeof(frob_handler_strings)/sizeof(frob_handler_strings[0]))

#define FROBHAND_TYPENAME "eFrobHandler"

static sFieldDesc frobhand_field[] = 
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, NUM_FROBHAND_STRINGS, NUM_FROBHAND_STRINGS, frob_handler_strings },
}; 

static sStructDesc frobhand_sdesc = 
{
   FROBHAND_TYPENAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(frobhand_field)/sizeof(frobhand_field[0]),
   frobhand_field,
}; 

static sPropertyTypeDesc frobhand_tdesc = 
{
   FROBHAND_TYPENAME,
   sizeof(int),
};

IIntProperty* gFrobHandlerProp = NULL;

eFrobHandler FrobHandler(ObjID obj)
{
   Assert_(gFrobHandlerProp);
   eFrobHandler result = kFrobHandlerHost;
   gFrobHandlerProp->Get(obj,&result);
   return result;
}

void SetFrobHandler(ObjID obj, eFrobHandler handler)
{
   Assert_(gFrobHandlerProp);
   gFrobHandlerProp->Set(obj, handler);
}

static void FrobHandlerPropInit()
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&frobhand_sdesc); 
   gFrobHandlerProp = CreateIntegralProperty(&frobhand_prop_desc,&frobhand_tdesc,kPropertyImplSparseHash);
}

////////////////////////////////////////////////////////////

BOOL FrobPropsInit(void)
{
   StructDescRegister(&frob_info_struct);
   StructDescRegister(&frob_proxy_struct);
   pFrobInfoProp = new cFrobInfoProperty(&frob_info_propdesc,FROB_INFO_IMPL);
   pFrobProxyRel = CreateStandardRelation(&frobProxyRelDesc,&frobProxyDataDesc,FROB_RELATION_QCASES);
   FrobHandlerPropInit();
   return TRUE;
}

void FrobPropsTerm(void)
{
   SafeRelease(pFrobInfoProp);
   SafeRelease(pFrobProxyRel);
   SafeRelease(gFrobHandlerProp);
}
