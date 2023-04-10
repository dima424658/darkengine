/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editprop.c,v 1.9 2000/02/19 13:10:35 toml Exp $

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>

#include <editprop.h>

#include <property.h>
#include <propbase.h>
#include <propname.h>
#include <propraw.h>

#include <editprst.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// callback for the property editor (installed in a structure editor)
static void LGAPI edit_prop_cb(sStructEditEvent *event, StructEditCBData data)
{
   PropertyEditor *ped = (PropertyEditor*)data;

   if (ped == NULL)
      return;

   switch (event->kind)
   {
      case kStructEdApply:
      case kStructEdOK:
         IPropertyRaw_Set(ped->propraw, ped->objid, ped->struc);
         if (ped->cb)
         {
            PropEditEvent event;

            event.type = kPropEdEventDone;
            event.editor = ped;
            event.eventdata = NULL;

            ped->cb(&event, ped->cbdata);
         }

         if (event->kind == kStructEdOK)
            PropertyEditorDestroy(ped);
         else
            IPropertyRaw_Get(ped->propraw,ped->objid,ped->struc);
         break;


      case kStructEdCancel:
         if (ped->cb)
         {
            PropEditEvent event;

            event.type = kPropEdEventCancel;
            event.editor = ped;
            event.eventdata = NULL;

            ped->cb(&event, ped->cbdata);
         }
   
         PropertyEditorDestroy(ped);
         break;

   }
}

// create a property editor for a named property
PropertyEditor* PropertyEditorCreate(IProperty* prop, ObjID objid)
{
   PropertyEditor *ped;
   static sStructDesc *desc = NULL;
   IPropertyRaw* propraw;
   ulong size;
   void* struc;
   const char* name = NULL;


   if (IProperty_GetID(prop) == PROPID_NULL)
   {
      Warning(("Attempt to edit nonexistent property\n"));
      return NULL;
   }

   name = IProperty_Describe(prop)->name;

   if (SUCCEEDED(COMQueryInterface(prop, IID_IPropertyRaw, (void**)&propraw)))
   {
      IStructDescTools* pTools = AppGetObj(IStructDescTools); 
      const sPropertyTypeDesc * propdesc = IProperty_DescribeType(prop);

      desc = COMCall1(pTools,Lookup,propdesc->type);
      SafeRelease(pTools); 
      
      if (desc == NULL)
      {
         Warning(("PropertyEditorCreate: Property '%s' doesn't have an sdesc!\n", name));
         SafeRelease(propraw);
         return NULL;
      }

      size = IPropertyRaw_Size(propraw);
      if (size == 0)
      {
         Warning(("PropertyEditorCreate: Property '%s' is zero bytes long!\n", name));
         SafeRelease(propraw);
         return NULL;
      }

      struc = Malloc(size);

      // Get the initial value
      IPropertyRaw_Get(propraw, objid, struc);


      {
         IStructEditor* sed;
         sStructEditorDesc editdesc;

         ped = Malloc(sizeof(PropertyEditor));
         memset(ped, 0, sizeof(PropertyEditor));

         ped->struc = struc;
         ped->size = size;
         ped->objid = objid;
         ped->propraw = propraw;

         strncpy(editdesc.title, name, sizeof(editdesc.title));
         editdesc.flags = kStructEditAllButtons;
         sed = CreateStructEditor(&editdesc, desc, struc);

         if (sed)
         {
            IStructEditor_SetCallback(sed, edit_prop_cb, ped);
            ped->sed = sed;
            //            IStructEditor_Go(sed,kStructEdModeless);
         }
         else
         {
            Warning(("PropertyEditorCreate: can't create a StructEditor!\n"));
            ped->sed = NULL;
            PropertyEditorDestroy(ped);
            return NULL;
         }
      }
   }
   else
      Warning(("PropertyEditorCreate: '%s' doesn't have an IPropertyRaw!\n", name));

   return ped;
}

// install a user callback on a property editor
void PropertyEditorInstallCallback(PropertyEditor* ped, PropEditCB cb, PropEditCBData data)
{
   if (ped == NULL)
   {
      Warning(("PropertyEditorInstallCallback: NULL editor passed!\n"));
      return;
   }

   if (ped->cb)
   {
      Warning(("PropertyEditorInstallCallback: There is already a callback installed for this editor!\n"));
      return;
   }
   else
   {
      ped->cb = cb;
      ped->cbdata = data;
   }

   // @HACK: Modeless struct editors have tab-key problems right now, so I'm adding this so 
   // that it will be modal
   IStructEditor_Go(ped->sed,kStructEdModal);

}

// given a property editor, kill it!
void PropertyEditorDestroy(PropertyEditor *ed)
{
   if (ed)
   {
      SafeRelease(ed->sed);
      SafeRelease(ed->propraw);
      Free(ed->struc);
      Free(ed);
   }
   else
      Warning(("PropertyEditorDestroy: NULL editor!\n"));
}







