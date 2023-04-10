/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcsndpr.h>

#include <label.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesc.h>
#include <sdestype.h>
#include <sdesbase.h>

#include <schema.h>

/////////////////////////////////////////////////////////////
// Object Sound Name Property
// This should support killing existing schemas when you modify the property.
// But it will all happen through the ambient system oneday and then this can
// all go away.
/////////////////////////////////////////////////////////////

// the all new improved property system...
// data ops
class cObjSoundNameDataOps: public cClassDataOps<Label>
{
};

// storage class
class cObjSoundNameStore: public cHashPropertyStore<cObjSoundNameDataOps>
{
};

// property implementation class
class cObjSoundNameProp: public cSpecificProperty<IObjSoundNameProperty, &IID_IObjSoundNameProperty, Label *, cObjSoundNameStore>
{
   typedef cSpecificProperty<IObjSoundNameProperty, &IID_IObjSoundNameProperty, Label *, cObjSoundNameStore> cParent; 

public:
   cObjSoundNameProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(Label); 

};

IObjSoundNameProperty* g_propObjSoundName;

static sPropertyDesc objSoundNameDesc =
{
   PROP_OBJ_SOUND_NAME, kPropertyInstantiate,
   NULL, 0, 0, // constraints, versions
   { "Sound", "Object Sound" },
};

/*
DECLARE_COMPLEX_PROPERTY_CLASS(cLabelProp, Label, Label);

// listen for property modifies
class cObjSoundNameProp : public cLabelProp
{
public:
   
   cObjSoundNameProp(sPropertyDesc *pDesc, ePropertyImpl impl):
      cLabelProp(pDesc, impl)
   {
   }

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, void* val)
   {
      if (msg&kListenPropModify)
      {
         if (OBJ_IS_CONCRETE(obj))
            SchemaPlayObj((Label*)val, obj);
      }
   }
};

*/

void ObjSoundNamePropInit()
{
   g_propObjSoundName = new cObjSoundNameProp(&objSoundNameDesc); // , kPropertyImplVerySparse);
}

