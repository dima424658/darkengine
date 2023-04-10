/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/otreeed.cpp,v 1.9 2000/02/19 13:11:10 toml Exp $

#include <wtypes.h>
#include <dynfunc.h>

#include <comtools.h>
#include <appagg.h>
#include <traitman.h>

#include <objtype.h>
#include <osysbase.h>
#include <osystype.h>
#include <otreeed.h>
#include <otreebas.h>

// For create archetype
#include <brtypes.h>
#include <editobj.h>
#include <gedit.h>
#include <winui.h>
#include <ged_room.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//
// Special Tree ops
//

static eTreeCBResult LGAPI arch_create(ObjID obj)
{
   editObjSetDefaultArchetype(obj); 
   gedit_set_default_brush_type(brType_OBJECT);
   return (eTreeCBResult)(kObjTreeNormal|kObjTreeQuit); 
} 


// For rooms

static eTreeCBResult LGAPI arch_add_room(ObjID obj, const char* name)
{
   AutoAppIPtr_(TraitManager, pTM);
   ObjID archetype;
   BOOL  concrete = TRUE;

   archetype = pTM->GetArchetype(obj);

   if (OBJ_IS_ABSTRACT(obj))
   {
      concrete = winui_GetYorN("Concrete room?");
      ged_create_room_obj((char *)name, archetype, concrete);
      return (eTreeCBResult)(kObjTreeChanged|kObjTreeCancel);
   }
   else
   {
      ged_make_room_obj(obj);
      return (eTreeCBResult)(kObjTreeNormal);
   }
}

static eTreeCBResult LGAPI arch_delete_room(ObjID obj)
{
   if ((obj == ged_room_get_root()) ||
       (obj == ged_room_get_default()))
      return (eTreeCBResult)(kObjTreeCancel);
   else
      return (eTreeCBResult)(kObjTreeNormal);
}

static eTreeCBResult LGAPI arch_create_room(ObjID obj)
{
   ged_room_set_default_arch(obj);
   gedit_set_default_brush_type(brType_ROOM);
   return (eTreeCBResult)(kObjTreeNormal|kObjTreeQuit);
}


//
// Here is the tree list.  Add your tree here.
//

// This is a big hack so the room tree starts up first when I want it to.

#define ARCHETYPE_TREEDESC { "Archetypes", "Object", 0, { NULL, NULL, arch_create}  }
#define METAPROP_TREEDESC  { "MetaProperties", "Metaproperty", kObjTreeNoCreateButton,  }
#define ROOM_TREEDESC      { "Rooms", "Base Room", kObjTreePermitConcretes, { arch_add_room, arch_delete_room, arch_create_room} }
#define TEXTURE_TREEDESC   { "Textures", "Texture", kObjTreeNoCreateButton|kObjTreePermitConcretes }
#define STIMULUS_TREEDESC  { "Act/React Stimuli", "Stimulus", kObjTreeNoCreateButton }
#define FLOW_TREEDESC { "Flow Groups", "Flow Group", kObjTreeNoCreateButton|kObjTreePermitConcretes }


static sObjTreeDesc trees[] = 
{
   ARCHETYPE_TREEDESC,
   METAPROP_TREEDESC,
   ROOM_TREEDESC,
   TEXTURE_TREEDESC,
   STIMULUS_TREEDESC, 
   FLOW_TREEDESC,
   
   NULL_TREEDESC  // terminator
}; 

static sObjTreeDesc room_trees[] = 
{
   ROOM_TREEDESC,
   ARCHETYPE_TREEDESC,
   METAPROP_TREEDESC,
   TEXTURE_TREEDESC,
   STIMULUS_TREEDESC, 
   FLOW_TREEDESC,

   NULL_TREEDESC
};

sObjTreeEditorDesc edit_trees = 
{
   "Object Hierarchy",   // title
   0,             // flags
   trees
};

sObjTreeEditorDesc room_edit_trees = 
{
   "Room Hierarchy",
   0,
   room_trees
};


static void tree_woe(const sObjTreeEditorDesc* , ObjID )
{
   CriticalMsg("Could not load dialog!");
}


DeclDynFunc_(void, LGAPI, DoHierarchy, (const sObjTreeEditorDesc* , ObjID ));
ImplDynFunc(DoHierarchy, "darkdlgs.dll", "_DoHierarchy@8", tree_woe);

#define ObjTree (DynFunc(DoHierarchy).GetProcAddress())


void EditObjHierarchyRoom(ObjID focus)
{
   ObjTree(&room_edit_trees, focus);
}

void EditObjHierarchy(ObjID focus)
{
   ObjTree(&edit_trees,focus); 
}



