/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/objedit.cpp,v 1.31 2000/02/24 23:40:12 mahk Exp $

#include <windows.h>
#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <mprintf.h>

#include <appagg.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objedit.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <objspace.h>

#include <linkbase.h>
#include <linkman.h>
#include <relation.h>
#include <linkedit.h>
#include <linkedst.h>
#include <linkdraw.h>

#include <objhistp.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <vbrush.h>
#include <status.h>
#include <gedit.h>
#include <uiedit.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <edittool.h>

#include <command.h>

#include <ctype.h>
#include <string.h>
#include <hilight.h>
#include <iobjed.h>
#include <lnktrait.h>

#include <dialogs.h>
#include <dynfunc.h>
#include <gen_bind.h>

#include <dbmem.h>

//  Brush lock stuff
//
extern "C" int MAX_LOCKED_BRUSHES;
extern "C" int TERRAIN_ID;
extern "C" ObjID gLockedBrushes [];
extern "C" BOOL brushIsLocked (editBrush* brush);

char* TERRAIN_STRING = "all_terrain";

static void linked_woe(sLink&)
{
	CriticalMsg("Could not load dialog!");
}

//  Dynamic function to open the new link dialog
//
DeclDynFunc_(BOOL,LGAPI,DisplayLinkDialog,(sLink&));
ImplDynFunc(DisplayLinkDialog,"darkdlgs.dll","_DisplayLinkDialog@4",linked_woe);

#define AskForLink	(DynFunc(DisplayLinkDialog).GetProcAddress())

////////////////////////////////////////////////////////////
// OBJ EDITING COMMANDS
//

//
// highlight objects of type
//

static void hilight_descendents(char* donorname)
{
   ObjID donor = EditGetObjNamed(donorname);
   AutoAppIPtr_(TraitManager,TraitMan);

   hilight_begin();
   IObjectQuery* query = TraitMan->Query(donor,kTraitQueryAllDescendents);
   ulong flags = kHilightCancel;

   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();
      if (OBJ_IS_CONCRETE(obj))
      {
         hilightAddByObjId(obj);
         flags = kHilightDone;
      }
   }
   SafeRelease(query);
   hilight_end(flags);
}

static void hilight_archetype(char* donorname)
{
   ObjID donor = EditGetObjNamed(donorname);
   AutoAppIPtr_(TraitManager,TraitMan);

   hilight_begin();
   IObjectQuery* query = TraitMan->Query(donor,kTraitQueryAllDescendents);
   ulong flags = kHilightCancel;

   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object();
      if (OBJ_IS_CONCRETE(obj))
      {
         // see whether we are simply descended
         if (TraitMan->ObjHasDonorIntrinsically(obj,donor))
         {
            hilightAddByObjId(obj);
            flags = kHilightDone;
         }
      }
   }
   SafeRelease(query);
   hilight_end(flags);
}

//
// Edit link data
//


#define IS_DELIM(x) ((x) == ',')
#define RELATION_ARG 2

static long parse_relation(const char* name)
{
   long id = atoi(name);
   if (id != RELID_NULL) return id;
   AutoAppIPtr_(LinkManager,LinkMan);
   cAutoIPtr<IRelation> Rel(LinkMan->GetRelationNamed(name));
   return Rel->GetID();
}


extern "C"
{
BOOL show_all_links = FALSE;
}


void edit_link_cmd(char* arg)
{
   // extract three args
   int args[3] = { LINKOBJ_WILDCARD, LINKOBJ_WILDCARD, RELID_WILDCARD } ;

   const int num_args = sizeof(args)/sizeof(args[0]);

   if (*arg == '\0')  // No args
   {
      editBrush* cur = vBrush_GetSel();
      if (brushGetType(cur) == brType_OBJECT)
         arg[0] = brObj_ID(cur);
   }
   else  // Find three args
      for (int i = 0; i < num_args; i++)
      {
         while(*arg && isspace(*arg)) arg++;

         char* delim;
         for (delim = arg; *delim && !IS_DELIM(*delim); delim++);
         char save = *delim;
         *delim = '\0';
         if (i == RELATION_ARG)
            args[i] = parse_relation(arg);
         else
            args[i] = EditGetObjNamed(arg);

         *delim = save;
         arg = delim+1;
      }

   sLinkEditorDesc desc = { "", kLinkEditAllButtons};
   if (show_all_links) desc.flags |= kLinkEditShowAll;
   EditLinks(&desc,args[0],args[1],args[2]);
}

////////////////////////////////////////


////////////////////////////////////////

static int cnt=0;
static editBrush *src, *dst;

static void getOurBrushes(editBrush *us)
{
   cnt++;
   if (us!=dst)
   {
      src=us;
   }
}

static void vbrui_build_link(char* arg)
{
   // Remove trailing whitespace;
   int len = strlen(arg);
   for (char* s = arg + len - 1; s > arg && isspace(*s); s--)
      *s = '\0';

   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> rel ( pLinkMan->GetRelationNamed(arg) );
   if (rel->GetID() == RELID_NULL)
   {
      Status("No such relation");
      return;
   }

   cnt=0;
   dst=vBrush_GetSel();
   if (brushGetType(dst)==brType_OBJECT)
   {
      vBrush_GroupOp(FALSE,getOurBrushes);
      if ((cnt==2)&&(brushGetType(src)==brType_OBJECT))
      {
         rel->Add(brObj_ID(src),brObj_ID(dst));
         Status("Linked");
         vBrush_editBrush_Op(src, vBrush_OP_REM);
         gedit_full_redraw();
         return;
      }
   }
   Status("Not two obj brushes");
}

static void link_two_objs(char *arg)
{
   char src[64], dst[64], flav[64];

   sscanf(arg,"%s %s %s",src,dst,flav);
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> rel ( pLinkMan->GetRelationNamed(flav) );
   if (rel->GetID() == RELID_NULL)
   {
      Status("No such relation");
      return;
   }

   ObjID src_obj=EditGetObjNamed(src);
   ObjID dst_obj=EditGetObjNamed(dst);
   AutoAppIPtr_(ObjectSystem,pObjSys);
   if ((src_obj==OBJ_NULL)||(dst_obj==OBJ_NULL)||
       !pObjSys->Exists(src_obj)||!pObjSys->Exists(dst_obj))
   {
      Status("No such object");
      return;
   }
   rel->Add(src_obj,dst_obj);
   Status("Linked");
}

////////////////////////////////////////

void list_all_objs(int arg)
{
   int i, cnts[2]={0,0};

   AutoAppIPtr_(ObjectSystem,pObjSys);
   for (i=1; i<gMaxObjID; i++)
   {
      char src_str[64];
      sprintf(src_str,"%d",i);
      ObjID src_obj=EditGetObjNamed(src_str);
      if ((src_obj==OBJ_NULL)||!pObjSys->Exists(src_obj))
      {
         cnts[1]++;
      }
      else
      {
         cnts[0]++;
         if (arg>0)
            mprintf("%s\n",ObjEditName(i));
      }
   }
   mprintf("Total %d used, %d free (of %d total)\n",cnts[0],cnts[1],gMaxObjID);
}

////////////////////////////////////////

// helper for link_draw_on and link_draw_off: sanity checker/status printer
static BOOL check_link_for_drawing(char *link_name, char *operation)
{
   char status_string[120];
   BOOL rv = FALSE;

   // We'll change this if our link type exists.
   sprintf(status_string, "No link type %s.", link_name);

   ILinkManager *pLinkMan = AppGetObj(ILinkManager);
   const sRelationDesc* reldesc;
   sRelationIter iter;
   RelationID flavor;

   // Does the given link type exist?
   pLinkMan->IterStart(&iter);

   while (pLinkMan->IterNext(&iter, &flavor)) {
      IRelation* pRel = pLinkMan->GetRelation(flavor);
      if (pRel) {
         reldesc = pRel->Describe();
         if (strnicmp(link_name, reldesc->name, 31) == 0) {
            sprintf(status_string, "%s link type %s.", operation, link_name);
            rv = TRUE;
         }
      }
      SafeRelease(pRel);
   }

   pLinkMan->IterStop(&iter);
   SafeRelease(pLinkMan);

   Status(status_string);
   return rv;
}

////////////////////////////////////////

static uint link_get_color_from_config(char *link_name)
{
   char config_name[64];
   char config_data[64];
   uint r = 255, g = 255, b = 255;  // default color is white

   sprintf(config_name, "link_draw_%s", link_name);

   if (config_get_raw(config_name, config_data, 63))
      sscanf(config_data, "%d %d %d", &r, &g, &b);

   return (r << 16) + (g << 8) + b;
}

////////////////////////////////////////

static void link_draw_on(char *link_name)
{
   if (check_link_for_drawing(link_name, "drawing")) {
      uint rgb, r, g, b;
      rgb = link_get_color_from_config(link_name);

      r = rgb >> 16;
      g = (rgb >> 8) & 255;
      b = (rgb) & 255;

      AddDrawnRelation(link_name, r, g, b);
      redraw_all_cmd();
   }
}


// from linkdraw.cpp:
extern int g_nLinkDrawSourceId;
static void link_draw_from(int nFrom)
{
   g_nLinkDrawSourceId = nFrom;
}


extern int g_nLinkDrawSplitId;
static void link_draw_split(int nSplitId)
{
   g_nLinkDrawSplitId = nSplitId;
}



////////////////////////////////////////

static void link_draw_off(char *link_name)
{
   if (check_link_for_drawing(link_name, "not drawing")) {
      RemoveDrawnRelation(link_name);
      redraw_all_cmd();
   }
}

////////////////////////////////////////

//	Brings up the link dialog with the From and To fields filled in with the
//	objects in the current multibrush (To is the current selection)
//
static void link_edit_from_multibrush ()
{
    sLink link = { 0, 0, 0 };

    bool twoObjs = false;
    cnt=0;
    dst=vBrush_GetSel();
    if (brushGetType(dst)==brType_OBJECT)
    {
        vBrush_GroupOp(FALSE,getOurBrushes);
        if ((cnt==2)&&(brushGetType(src)==brType_OBJECT))
        {
            gedit_full_redraw();

            link.source = brObj_ID(src);
            link.dest = brObj_ID(dst);
            twoObjs = true;
        }
    }

	if (!twoObjs)
	{
		Status("Not two obj brushes");
		return;
	}

	//	Display the new link dialog, with the objects filled in
	//
	if (AskForLink (link))
	{
        AutoAppIPtr_(LinkManager,pLinkMan);
		pLinkMan = AppGetObj(ILinkManager);
		cAutoIPtr<IRelation> rel (pLinkMan->GetRelation(link.flavor));
		rel->Add(link.source, link.dest);
		Status("Linked");
	}

    //  Re-poll for keys to avoid 'stickyness'
    //
    g_pInputBinder->PollAllKeys();
}

////////////////////////////////////////

static char *chain_flavour;
static char chain_arg[63];
static void makeChainLink(editBrush *us)
{
    if (us!=src)
    {
        dst=us;
        sprintf (chain_arg, "%d %d %s", brObj_ID(src), brObj_ID(dst), chain_flavour);
        link_two_objs (chain_arg);
        src=us;
    }
}

//	Links together a chain of multibrushed objects
//
static void link_chain_from_multibrush (char *arg)
{
    chain_flavour = arg;
    src=vBrush_GetSel();

    vBrush_GroupOp(FALSE, makeChainLink);

    char drawString [31];
    sprintf (drawString, "link_draw_on %s", arg);
    CommandExecute (drawString);
    gedit_full_redraw();

	Status("Chain linked.");
}

////////////////////////////////////////

static void lock_brush (char *brush_name)
{
    ObjID brush;
    if (strcmp (brush_name, TERRAIN_STRING) == 0)
        brush = TERRAIN_ID;
    else
        brush = EditGetObjNamed(brush_name);

    if (brush != OBJ_NULL)
    {
        for (int i = 0; i < MAX_LOCKED_BRUSHES; i++)
        {
            if (gLockedBrushes[i] == 0)
            {
                gLockedBrushes[i] = brush;
                Status("Locked.");

                editBrush* cur = vBrush_GetSel();
                if (brushIsLocked (cur))
                    CommandExecute ("cycle_brush 1");
                return;
            }
        }
        Status("Too many locked brushes!");
    }
    else
    {
        Status("Can't find brush.");
    }
}

////////////////////////////////////////

static void unlock_brush (char *brush_name)
{
    ObjID brush;
    if (strcmp (brush_name, TERRAIN_STRING) == 0)
        brush = TERRAIN_ID;
    else
        brush = EditGetObjNamed(brush_name);

    if (brush != OBJ_NULL)
    {
        for (int i = 0; i < MAX_LOCKED_BRUSHES; i++)
        {
            if (gLockedBrushes[i] == brush)
            {
                gLockedBrushes[i] = 0;
                Status("Unlocked.");
                return;
            }
        }
        Status("Brush is not locked.");
    }
    else
    {
        Status("Can't find brush.");
    }
}

////////////////////////////////////////

static void unlock_all_brushes (void)
{
    memset (gLockedBrushes, 0, MAX_LOCKED_BRUSHES * sizeof(ObjID));
    Status("All brushes unlocked.");
}

//------------------------------------------------------------
// Struct desc for sObjBounds
//

static sFieldDesc obj_bounds_fields[] = 
{
   { "Min", kFieldTypeInt, FieldLocation(sObjBounds,min) },
   { "Max", kFieldTypeInt, FieldLocation(sObjBounds,max) },
}; 

static sStructDesc obj_bounds_sdesc = StructDescBuild(sObjBounds,0,obj_bounds_fields); 

////////////////////////////////////////

static void resize_obj_space()
{
   AutoAppIPtr(ObjIDManager); 
   sObjBounds bounds = pObjIDManager->GetObjIDBounds(); 

   AutoAppIPtr(EditTools); 
   HRESULT result = pEditTools->EditTypedData("Object ID Space Bounds","sObjBounds",&bounds); 

   if (result == S_OK)
   {
      result = pObjIDManager->ResizeObjIDSpace(bounds); 
      if (SUCCEEDED(result))
         Status("Object space resize successful"); 
      else
         Status("Object space resize failed."); 

   }
}


////////////////////////////////////////

static Command edit_mode_commands[] =
{
   { "edit_links", FUNC_STRING, edit_link_cmd,
     "edit_links [<src> [, <dest> [, <flavor>]]]", HK_EDITOR},
   { "link_show_all", VAR_BOOL, &show_all_links,
     "show links marked as hidden in the link editor" },
   { "hilight_obj_type", FUNC_STRING, hilight_descendents,
     "hilight all instances of an archetype, including descendants", HK_EDITOR},
   { "hilight_archetype", FUNC_STRING, hilight_archetype,
     "hilight all non-descendant instances of an archetype", HK_EDITOR},

   { "link_group", FUNC_STRING, vbrui_build_link, "a group of two objs get linked, select is dest, arg is relation" },
   { "link_objs", FUNC_STRING, link_two_objs, "link_objs src dst flavor" },

   // controlling which relations are drawn in editor
   { "link_draw_on", FUNC_STRING, link_draw_on,
     "show a kind of link in wireframe" },

   { "link_draw_from", FUNC_INT, link_draw_from,
     "Draw only links who's source is this" },

   { "link_draw_split", FUNC_INT, link_draw_split,
     "If link source has multple destinations, and this ID is among them, use that branch only." },

   { "link_draw_off", FUNC_STRING, link_draw_off,
     "don't show a kind of link in wireframe" },

   { "link_multi", FUNC_VOID, link_edit_from_multibrush,
     "adds a new link between two objs in a multibrush" },
   { "link_chain", FUNC_STRING, link_chain_from_multibrush,
     "adds a chain of links between objs in a multibrush" },
   { "lock", FUNC_STRING, lock_brush,
     "lock <brush type>: asks for confirmation whenever brush is changed" },
   { "unlock", FUNC_STRING, unlock_brush,
     "unlock <brush type>" },
   { "unlock_all", FUNC_VOID, unlock_all_brushes,
     "unlocks all brushes" },
   { "resize_obj_id_space", FUNC_VOID, resize_obj_space,
     "Change obj_min and obj_max on the fly." },

};

static Command all_mode_commands[] =
{
   { "list_obj", FUNC_INT, list_all_objs, "w/no arg, shows counts, w/args, lists all objs in id order" },
   { "obj_histogram_popup", FUNC_VOID, popup_obj_histogram, "Brings up obj list sorted by usage.", HK_EDITOR},
   { "obj_alpha_popup", FUNC_VOID, popup_obj_alpha, "Brings up obj list sorted alphabetically.", HK_EDITOR},
};


//------------------------------------------------------------
// Struct desc for ObjID
//

StructDescDefineSingleton(objedit_objid_sdesc, ObjID, kFieldTypeInt, 0);

//
// Add relations to object editor
//

static void create_rel_traits(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   AutoAppIPtr_(ObjEditors,pEdit);


   IEditTrait* trait;

   trait = CreateLinkEditTrait(RELID_WILDCARD,FALSE);
   pEdit->AddTrait(trait);
   SafeRelease(trait);

   trait = CreateLinkEditTrait(RELID_WILDCARD,TRUE);
   pEdit->AddTrait(trait);
   SafeRelease(trait);

}



////////////////////////////////////////////////////////////
// ObjEditInit
//

void ObjEditInit(void)
{
   COMMANDS(edit_mode_commands,HK_EDITOR);
   COMMANDS(all_mode_commands,HK_ALL);
   StructDescRegister(&objedit_objid_sdesc);
   StructDescRegister(&obj_bounds_sdesc); 
   create_rel_traits();
}

