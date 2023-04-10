/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vbrush.c,v 1.52 2000/03/16 19:17:52 patmac Exp $
// virtual brush/multi brush concentrator abstractions

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <lg.h>
#include <matrix.h>
#include <matrixd.h>
#include <mprintf.h>
#include <config.h>

#include <command.h>
#include <status.h>

#include <tagfile.h>
#include <editsave.h>
#include <filetool.h>
#include <filemenu.h>

#include <gridsnap.h>

#include <editbr.h>
#include <editbr_.h>
#include <editobj.h>
#include <brinfo.h>
#include <brlist.h>
#include <brundo.h>
#include <brrend.h>
#include <primal.h>
#include <gedit.h>
#include <brquery.h>
#include <hilight.h>

#include <vbrush.h>
#include <vbr_math.h>

#include <namenum.h>
#include <prompts.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//#define WATCH_LAST_LAST

extern BOOL brushIsLocked (editBrush* brush);

void br_update_obj(editBrush *us);

#ifdef WATCH_LAST_LAST
#define invalidNotify(msg) mprintf(msg)
#else
#define invalidNotify(msg)
#endif

// simple safe get accessor for now
editBrush *vBrush_GetSel(void)
{
   static editBrush safety_brush;
   editBrush *p=blistGet();
   return p ? p : &safety_brush;
}

/////////////
// misc simple "operators"

static void br_degroup(editBrush *us)
{
   brSelect_Group(us)=0;
}

static BOOL delete_shuffle=FALSE;
static void br_delete(editBrush *toast)
{
   editBrush *us=blistGet(), *modBr;
   BOOL shuffle=((toast!=NULL)&&(us!=toast));
   if (shuffle)   // @TODO: get rid of dorky requirement that you delete at cursor
      if (blistSetPostoBrush(toast)==-1)
         return;
   editUndoStoreStart(toast);
   editUndoStoreDelete(toast);
   modBr=blistDelete();
   if (modBr!=NULL)
   {
      if (!delete_shuffle)
      {
         modBr->flags |=  kEBF_BeingDeleted;
         brush_field_delete(modBr);
         modBr->flags &= ~kEBF_BeingDeleted;
      }
      brushDelete(modBr);
   }
   if (shuffle)
      if (blistSetPostoBrush(us)==-1)
         return;
}

// @TODO: get primal type from old brush
static int        clone_type=-1;
static editBrush *clone_cursor=NULL;
static void br_clone(editBrush *us)
{
   editBrush *modBrush;
   int type=(brushGetType(us)==brType_TERRAIN)?us->primal_id:0;
   modBrush=brushInstantiate(type);
   mx_copy_vec(&modBrush->pos,&us->pos);
   if (clone_type==-1)
      mx_copy_vec(&modBrush->sz,&us->sz);
   else
      modBrush->sz.el[0]=modBrush->sz.el[1]=modBrush->sz.el[2]=1.0;
   modBrush->ang=us->ang;
   gedit_full_create_brush(modBrush,us,GEDIT_CREATE_AT_END|GEDIT_CREATE_CLONE,clone_type);
   brSelect_Group(modBrush)=brSelect_Group(us);    // new group is us
   brSelect_Group(us)=0;    // our old brush goes back to group 0
   if (us==clone_cursor) clone_cursor=modBrush;
}

// actually move this brush to the end of time
static void br_eot(editBrush *move_me)
{
   if (move_me!=blistGet())
      blistSetPostoBrush(move_me);
   blistDelete();
   blistSeek(0,SEEK_END);
   blistInsert(move_me);
}

/////////////
// "loop clients" - ie. things which actually do operations... currently somewhat icky

// the frame callback snoops on the focused brush every frame
//   all editor brush modifiers should vBrush_editBrush_Get, not blistGet()
//   if the current focus is a real brush, the modifiers just do so, and it changes
//     (though now it saves last frame, so it can auto do the undos from here)
//   if the current focus is a virtual brush, it snoops the changes
//     it goes through all fields whose changes it can react to
//     it then calls the operation for all brushes in the vBrush

// delete and clone are specials, basically, since they need to be called out to here
// as is cycle_brush (which now has a within vBrush parameter, basically)

// group id invariant is
//  cur_Group is the "Active group selection"
//  0 means no selection active
//  1 is reserved for the "in progress" selection
//  2-> are saved (real) selections
static int        cur_Group=0;

static editBrush  _virtual_Br;           // static for holding a multiBrush virtual object
static editBrush  _last_Br;              // static for holding copy of last brush, v or not
static BOOL       last_br_valid=FALSE;   // does _last_Br have valid data
static int        last_Group=0;          // what's the last group we had

static sNameNum  *pNN_MultiBrush=NULL;

#define abandon_Group() last_Group=cur_Group=0

// singular callback for vBrush Focus Changes
void (*vBrush_Change_Callback)(editBrush *brush) = NULL;

static void create_virtual_Br(void)
{
   editBrush *p=&_virtual_Br;
   brushZero(p,0);
   brSelect_Group(p)=cur_Group;
   brSelect_Flag(p)=brSelect_VIRTUAL;
   memcpy(&_last_Br,p,sizeof(editBrush));
   p->br_id=VIRTUAL_BRUSH_ID;
   invalidNotify("create invalidation\n");
   last_br_valid=FALSE;  // so we know not to try and "do" anything in the frame callback
}

// get the current vBrush
editBrush *vBrush_editBrush_Get(void)
{
   editBrush *p=vBrush_GetSel();
   if (cur_Group==0) return p;
   // otherwise, we better have a virtual brush around for the gfh
   if (brSelect_Group(&_virtual_Br)!=cur_Group)
      create_virtual_Br();  // this test is bad, since we want to reset it more often
   return &_virtual_Br;
}

// is this brush in the current group?
BOOL vBrush_inCurGroup(editBrush *us)
{
   if (cur_Group==0) return FALSE;
   return brSelect_Group(us)==cur_Group;
}

// ok.. things get ugly
//   if we changed the last brush (i_changed_the_brush) we have to redraw everything
//     (to get correct new brush as part of back and store)
//   else we just change, so we just need to do a select update
// still broken, we really want i_changed to somehow update the old picture for the old brush loc
void we_switched_brush_focus(void)
{
   editBrush *cur=vBrush_GetSel();
   if (cur_Group)
      if (brSelect_Group(cur)!=cur_Group)
      {
         last_Group=cur_Group;
         cur_Group=0; // we changed groups on this focus change, so "deactivate" old
         editUndoStoreGroup(last_Group,cur_Group);
      }
   invalidNotify("switch focus invalidation\n");
   last_br_valid=FALSE;
   // set up GFH Stuff? anyone?
   if (vBrush_Change_Callback)
      (*vBrush_Change_Callback)(vBrush_editBrush_Get());
   gedit_change_selection();
}

// run a callback on all brushes in the group, or on all brushes if run_all is TRUE
void vBrush_GroupOp(BOOL run_all, void (*br_op)(editBrush *us))
{
   bool grid_state=cur_global_grid.grid_enabled;
   editBrush *us, *tmp;
   int hIter;

   if (!run_all && cur_Group==0) return; // well, no group, so we just dont care
   cur_global_grid.grid_enabled=0;
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      tmp=us;                     // save us off
      us=blistIterNext(hIter);   // get to next now, then do the callback
      if (run_all || (brSelect_Group(tmp)==cur_Group))
         (*br_op)(tmp);           // look, go run the op
   }
   cur_global_grid.grid_enabled=grid_state;   
}

// run a callback on all brushes in the group, or on all brushes if run_all is TRUE
static void run_simple_bounded_group_br_op(void (*br_op)(editBrush *us))
{
   bool grid_state=cur_global_grid.grid_enabled;
   editBrush *us, *tmp, *end;
   int hIter;

   if (cur_Group==0) return; // well, no group, so we just dont care
   cur_global_grid.grid_enabled=0;
   blistSeek(0,SEEK_END);
   end=blistGet();
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      tmp=us;                    // save us off
      us=blistIterNext(hIter);  // get to next now, then do the callback
      if (brSelect_Group(tmp)==cur_Group)
         (*br_op)(tmp);          // look, go run the op
      if (tmp==end)              // should be tmp, not us, since we want to run the original end of list too
      {
         blistIterDone(hIter);  // tell next system we are done
         break;                  // at original end of list, lets go home
      }
   }
   cur_global_grid.grid_enabled=grid_state;   
}

static BOOL group_ok(editBrush *br)
{
   return (cur_Group==0) || (brSelect_Group(br)==cur_Group);
}

BOOL vBrush_cycle(int dir)
{
   BOOL rv=brCycle(dir,group_ok);
   if (!rv) abandon_Group();       // since we cant find any of it
   return rv;
}


// @TODO: for now just cycle randomly (should have a look for closest call back, eh?)
static BOOL getToCurGroup(void)
{
   return vBrush_cycle(1);
}

///// backdoors for things like undo and such
BOOL vBrush_GoToGroup(int group_id)
{
   editBrush *cur=vBrush_GetSel();
   BOOL rv=TRUE;
   
   if ((cur_Group==group_id)&&(brSelect_Group(cur)==cur_Group))
      return rv;
   cur_Group=group_id;
   if (brSelect_Group(cur)!=cur_Group)
      rv=vBrush_cycle(1);
   if (!rv)
      vBrush_SelectBrush(cur);
   return rv;
}

BOOL vBrush_DontUpdateFocus=FALSE;

// this is used for mouse or other direct brush selection
void vBrush_SelectBrush(editBrush *sel)
{
    if (brushIsLocked(sel))
    {
        Status ("* LOCKED *");
        return;
    }

   if (sel==NULL) return;
   blistSetPostoBrush(sel);
   if (!vBrush_DontUpdateFocus)
      we_switched_brush_focus();
}

// @SOLVE: do we want to do things with parm for real or fake, or 2 calls
// ie. there is user does delete, and internal system does delete
void vBrush_DeletePtr(editBrush *what)
{
   BOOL need_db_update=TRUE; // cause on multibrush we are gonna wuss out and always update

   if (what->flags & kEBF_BeingDeleted)
      return;

   if (what==&_virtual_Br)   // we are the virtual brush
   {  // @TODO: put an undo package around this
      editUndoStoreBlock(TRUE);
      vBrush_GroupOp(FALSE,br_delete);
      editUndoStoreBlock(FALSE);
      abandon_Group();   // as we know we just got rid of the last of it
   }
   else
   {
      delete_shuffle=(what!=blistGet());
      //need_db_update=(brushGetType(what)==brType_LIGHT)||(brushGetType(what)==brType_TERRAIN);
      need_db_update = (brushGetType(what)==brType_TERRAIN);
      br_delete(what);
      delete_shuffle=FALSE;   // make this less of a @HACK
   }
   if (need_db_update)
      gedit_db_update(TRUE);  // and redraw, as well
   else
      gedit_full_redraw();    // just redraw, since the db was not changed (in a portal/WR way)
   we_switched_brush_focus();
}

static BOOL vBrush_singlecopy(editBrush *br, BOOL clone)
{
   if (!clone) clone_type=gedit_default_brush_type();
   br_clone(br);
   if (!clone) clone_type=-1;
   return (brushGetType(br)==brType_TERRAIN);
}

static void vBrush_clonefinish(BOOL update_p)
{
   vBrush_SelectBrush(clone_cursor);
   if (update_p)
      gedit_db_update(TRUE);  // know about new brushes in one place
   else
      gedit_full_redraw();
}

// clone this brush, in a group status, clone punts group id
void vBrush_ClonePtr(editBrush *what)
{
   BOOL need_update=FALSE;
   clone_cursor=vBrush_GetSel();
   if (what==&_virtual_Br)   // we are the virtual brush
   {
      editUndoStoreBlock(TRUE);      
      run_simple_bounded_group_br_op(br_clone);
      editUndoStoreBlock(FALSE);
      need_update=TRUE;
   }
   else
      need_update=vBrush_singlecopy(what,TRUE);
   vBrush_clonefinish(need_update);
}

void vBrush_CreateSingle(void)
{
   clone_cursor=vBrush_GetSel();
   vBrush_clonefinish(vBrush_singlecopy(clone_cursor,FALSE));
}

// a new Brush (singleton) has been created in the editor
void vBrush_NewBrush(editBrush *new_br)
{
   if (cur_Group)
      brSelect_Group(new_br)=cur_Group;
   we_switched_brush_focus();
}

editBrush *vBrush_UndoVBrush(editBrush *old_br)
{
   if (cur_Group==brSelect_Group(old_br))
   {
      _last_Br=_virtual_Br;
      invalidNotify("undo invalidation\n");
      last_br_valid=TRUE;
      return &_virtual_Br;
   }
   Warning(("vbrush undo fail: curG %d, brush is %d\n",cur_Group,brSelect_Group(old_br)));
   return NULL;
}

// first see what sort of context we are in
// ie. if our vBrush is just a real brush, great, nothing to really do

//   @TODO: should undoredo really be through here even for single brush???

extern BOOL gedit_is_drag_in_prog(void);

static int       last_vec_type=brVec_NONE;
static editBrush last_vec_vb;

#define brVec_RESET (brVec_NONE-1)
static void switch_vb_base_p(int type, editBrush *base_br)
{
   if (type!=last_vec_type)
   {
      last_vec_vb=*base_br;
      store_BrushVecs();
      if (type==brVec_RESET)
         last_vec_type=brVec_NONE;
      else
         last_vec_type=type;
#ifdef WATCH_LAST_LAST      
      mprintf("new vBase %x type %d\n",base_br->ang.el[2],type);
#endif
   }
}

// per frame setup
void vBrush_FrameCallback(void)
{
   static BOOL      last_last_valid=FALSE;
   editBrush *curB=vBrush_editBrush_Get();
   
   if (last_br_valid) // then just record it off, no changes
   {
      if (!last_last_valid)
         invalidNotify("Trigger In\n");
      if (cur_Group==0) // nothing to do
         ;
      else if (brSelect_Group(curB)!=cur_Group)
      {  // @TODO: do undo stuff with last brush?
         invalidNotify("Local invalidation\n");
         last_br_valid=FALSE;
         return;
      }
      else
      {  // if here, curB is part of the current active Group
         // if trigger in, save off all vectors and initial multibrush
         if (!last_last_valid)
            switch_vb_base_p(brVec_RESET,&_last_Br);  // always reset since < Vec_NONE
         if (memcmp(curB,&_last_Br,sizeof(editBrush))!=0)
         {
            mxs_vector vec_change;
            mxds_vector ang_diff;
            BOOL deal=FALSE;

            if (memcmp(&curB->pos,&_last_Br.pos,sizeof(mxs_vector)))
            {
               switch_vb_base_p(brVec_POS,&_last_Br);
               mx_sub_vec(&vec_change,&curB->pos,&_last_Br.pos);
               br_translate_setup(&vec_change);
               vBrush_GroupOp(FALSE,br_translate);
               vBrush_GroupOp(FALSE,br_update_obj);    
               deal=TRUE;
            }
            if (memcmp(&curB->sz,&_last_Br.sz,sizeof(mxs_vector)))
            {
               mxs_vector scale_change;
               //               mx_sub_vec(&vec_change,&curB->sz,&_last_Br.sz);
               if (compute_scale_fac(&scale_change,&_last_Br.sz,&curB->sz))
               {
                  switch_vb_base_p(brVec_SZ,&_last_Br);
                  compute_scale_fac(&scale_change,&last_vec_vb.sz,&curB->sz);
                  // mprintf("scale d %g from %g %g %g\n",scale_change.x,last_vec_vb.sz.x,_last_Br.sz.x,curB->sz.x);
                  restore_BrushVecs(brVec_SZ|brVec_POS);
                  br_scale_setup(&scale_change);
                  vBrush_GroupOp(FALSE,br_scale);
               }
               else
               {
                  extern void label_vs_update(void); // only do this if it isnt the last one, or something
                  memcpy(&curB->sz,&_last_Br.sz,sizeof(mxs_vector));   // save back
                  label_vs_update();
               }
               deal=TRUE;
            }
            if (ang_quantize_and_sub(&ang_diff,&curB->ang,&_last_Br.ang))
            {
               mxs_angvec ang_change;
               switch_vb_base_p(brVec_ANG,&_last_Br);
               sub_angvecs(&ang_change,&curB->ang,&last_vec_vb.ang);
               //               ang_quantize_and_sub(&ang_diff,&curB->ang,&last_vec_vb.ang);
               restore_BrushVecs(brVec_ANG|brVec_POS);
               br_rotate_setup(&ang_change);
               vBrush_GroupOp(FALSE,br_rotate);
               deal=TRUE;
            }
            else if (memcmp(&curB->ang,&_last_Br.ang,sizeof(mxs_angvec)))
               deal=TRUE;
            if (!deal)
               Warning(("cant multibrush that, batman\n"));
            else if (!gedit_is_drag_in_prog())  // else drag is dealing with it?
               gedit_raw_change_brush(curB,FALSE,FALSE,TRUE);
         }
      }
      last_last_valid=last_br_valid;      
   }
   else
   {
      if (last_last_valid)
         invalidNotify("Trigger Out\n");
      // if deselected, reset scale to normal
      _virtual_Br.sz.el[0]=_virtual_Br.sz.el[1]=_virtual_Br.sz.el[2]=2.0;
   }
   last_last_valid=last_br_valid;
   memcpy(&_last_Br,curB,sizeof(editBrush));
   last_br_valid=TRUE;
}

/////////////
// the group public vBrush operations

// when we add to group we check cur_Group
// if >=1, we can just add to it, woo-woo
// if 0, we then assume we are starting a new group
//   which means we first go clear group 1
//   then set curGroup to 1, and do the add
static BOOL new_group;
static void add_to_group(editBrush *us)
{
   new_group=FALSE;
   if (cur_Group==0)
   {
      if (brSelect_Group(us))
         cur_Group=brSelect_Group(us);
      else
      {  // @TODO: i dont think this is working
         cur_Group=1;
         vBrush_editBrush_Op(NULL,vBrush_OP_RESET);
         new_group=TRUE;
      }
   }
   brSelect_Group(us)=cur_Group;
}

// this little section is the save system
static char used_IDs[256];
static int  new_ID=0;

static void make_used_id_map(editBrush *us)
{
   used_IDs[brSelect_Group(us)]=1;
}

static void switch_to_new_group(editBrush *us)
{
   brSelect_Group(us)=new_ID;
}

int vBrush_GetFreeGroup(void)
{
   int i;
   memset(used_IDs,0,sizeof(char)*256);
   vBrush_GroupOp(TRUE,make_used_id_map);
   for (i=2; i<256; i++)
      if (used_IDs[i]==0)
         return i;
   return 0;
}

static int vBrush_NextUsedGroup(int cur, int dir)
{
   int idx=cur;
   memset(used_IDs,0,sizeof(char)*256);
   vBrush_GroupOp(TRUE,make_used_id_map);
   do {
      cur=(cur+dir)&0xff;  // move to next candidate
   } while ((cur!=idx)&&(used_IDs[cur]==0));
   return cur;
}

// editBrush statechanges with respect to the current vBrush
BOOL vBrush_editBrush_Op(editBrush *us, int op)
{
   BOOL rv=TRUE;
   switch (op)
   {
      case vBrush_OP_ADD:
         new_group=FALSE;
         if (brSelect_Group(us)!=0)
          { Warning(("cant add from an existing group\n")); rv=FALSE; }
         else
            add_to_group(us);
         break;
      case vBrush_OP_REM:
         if (brSelect_Group(us)==0)
          { Warning(("Trying to remove an ungrouped brush\n")); rv=FALSE; }
         else if (brSelect_Group(us)!=cur_Group)
          { Warning(("removing from a non-current group?\n")); rv=FALSE; }
         brSelect_Group(us)=0;
         break;         
      case vBrush_OP_TOG:
         new_group=FALSE;
         if (brSelect_Group(us)==0)     
            add_to_group(us);           // we have no group, so put us in the group
         else if (cur_Group==0)
            cur_Group=brSelect_Group(us);
         else if (brSelect_Group(us)==cur_Group)
            brSelect_Group(us)=0;
         else
          { Warning(("Togging a brush in another group?\n")); rv=FALSE; }
         break;
      case vBrush_OP_SAVE:
         rv=FALSE;
         if (cur_Group==1)
         {
            new_ID=vBrush_GetFreeGroup();
            if (new_ID)
            {
               editUndoStoreBlock(TRUE);               
               vBrush_GroupOp(FALSE,switch_to_new_group);
               editUndoStoreBlock(FALSE);               
               cur_Group=new_ID;  // @TODO: Status here?
               rv=TRUE;
            }
         }
         break;
      case vBrush_OP_RESET:
         editUndoStoreBlock(TRUE);
         vBrush_GroupOp(FALSE,br_degroup);
         editUndoStoreBlock(FALSE);
         break;
   }
   return rv;
}

/////////////////////
// debug stuff

#ifdef DBG_ON
static void show_vbrush_info(editBrush *us)
{
   mprintf("%d> group %d\n",us->br_id,brSelect_Group(us));
}

void vBrush_DebugInfo(int dbg_level)
{  // for now, if dbg_level, do a full dump, else just the current group
   vBrush_GroupOp(dbg_level>0,show_vbrush_info);
   mprintf(" cur %d, last %d, get %x, blist %x, virt %x group %d\n",
           cur_Group,last_Group,vBrush_editBrush_Get(),blistGet(),
           &_virtual_Br,brSelect_Group(&_virtual_Br));
}
#endif   

///////////////////
// command system stuff

static int cycle_type_target=brType_TERRAIN;
static BOOL type_ok(editBrush *br)
{
   return brushGetType(br)==cycle_type_target;
}

static BOOL highlight_ok(editBrush *br)
{
   return isActiveHighlight(br->br_id);
}

static BOOL filter_ok(editBrush *br)
{
   return (brrendCheckTypeFilter(br) && !brushIsLocked(br));
}

static editBrush *near_brush = NULL;
static BOOL near_ok(editBrush *br)
{
    float posDiff[3];
    float szDiff[3];
    
    posDiff[0] = abs(br->pos.x - near_brush->pos.x) + abs(br->pos.y - near_brush->pos.y);
    posDiff[1] = abs(br->pos.y - near_brush->pos.y) + abs(br->pos.z - near_brush->pos.z);
    posDiff[2] = abs(br->pos.z - near_brush->pos.z) + abs(br->pos.x - near_brush->pos.x);

    szDiff[0] = abs(br->sz.x - near_brush->sz.x) + abs(br->sz.y - near_brush->sz.y);
    szDiff[1] = abs(br->sz.x - near_brush->sz.x) + abs(br->sz.y - near_brush->sz.y);
    szDiff[2] = abs(br->sz.x - near_brush->sz.x) + abs(br->sz.y - near_brush->sz.y);

    return ((posDiff[0] <= 1.0 && szDiff[0] <= 1.0) ||
            (posDiff[1] <= 1.0 && szDiff[1] <= 1.0) ||
            (posDiff[2] <= 1.0 && szDiff[2] <= 1.0));
}

// find next highlighted brush
// useful when you can't find the first highlighted brush in a set
static void vbrui_cycle_highlight(int dir)
{
   brCycle(dir, highlight_ok);
}

// cycle through brushes that overlap the current brush
static void vbrui_cycle_near(int dir)
{
   near_brush = blistGet();
   brCycle(dir, near_ok);
}

// cycle based on current "context", ie. brush type
static void vbrui_cycle_context(int dir)
{
   editBrush *cur=vBrush_editBrush_Get();
   if (isActiveHighlight(cur->br_id))
      vbrui_cycle_highlight(dir);
   else if (brSelect_Flag(cur)&brSelect_VIRTUAL) // currently a multibrush
      vBrush_cycle(dir);
   else
   {
      cycle_type_target=brushGetType(cur);
      brCycle(dir,type_ok);
   }
}

void vbrui_select_obj(int obj_id)
{
   editBrush *br=editObjGetBrushFromObj(obj_id);
   if (br!=NULL)
      vBrush_SelectBrush(br);
   else
      Status("No brush for objID");
}

void vbrui_select_brid(int br_id)
{
   editBrush *br=brFind(br_id);
   if (br!=NULL)
      vBrush_SelectBrush(br);
   else
      Status("No brush with that ID");
}

// should cycle within current type filter
static void vbrui_cycle_brush(int dir)
{
   if (brrendTypeFilterActive() || config_is_defined ("hens_changes"))
      brCycle(dir,filter_ok);
   else
      brCycle(dir,NULL);
}

static void vbrui_save_vbrush(void)
{
   if (cur_Group==1)
   {  // @TODO: make sure there are elements in our group?
      char buf[PROMPT_STR_LEN], def[PROMPT_STR_LEN], *val;
      vBrush_editBrush_Op(NULL,vBrush_OP_SAVE);
      strcpy(def,"Group_");
      itoa(cur_Group,def+strlen(def),10);
      val=prompt_string(def,buf);
      NamenumStore(pNN_MultiBrush,val,cur_Group);
      Status("Saved Group");
   }
   else if (cur_Group)
      Status("Group already saved");
   else
      Status("No group to save");
}

static void vbrui_select_by_name(char *name)
{
   int picked_tag;
   if (name && *name)
      picked_tag=NamenumFetchTagForString(pNN_MultiBrush,name);
   else
      picked_tag=NamenumSimpleMenuChooseTag(pNN_MultiBrush);
   if (picked_tag!=NAMENUM_NO_TAG)
   {
      cur_Group=picked_tag;
      getToCurGroup();
   }
}

static void vbrui_breakup_vbrush(void)
{
   if (cur_Group)
   {
      NamenumClearByTag(pNN_MultiBrush,cur_Group);
      vBrush_editBrush_Op(NULL,vBrush_OP_RESET);
      gedit_full_redraw();
      Status("Group dissolved");
   }
   else
      Status("No group");
}

static void change_brushes_by_id(char *args, int op)
{
   char *s=args, *p, tmp;
   BOOL any_new_groups=FALSE;
   int val, max;

   if (s==NULL) return;
   do {
      while ((*s)!='\0'&&!isdigit(*s))
         ++s;
      if ((*s)=='\0') break;
      p=s;
      while ((*p)!='\0'&&isdigit(*p))
         p++;  // go over the digits
      tmp=*p; *p='\0'; val=atoi(s); *p=tmp;
      vBrush_editBrush_Op(brFind(val),op);
      any_new_groups|=new_group;
      s=p;
      if ((*s)=='-')
      {
         s++;
         p=s;
         while ((*p)!='\0'&&isdigit(*p))
            p++;  // go over the digits
         tmp=*p; *p='\0'; max=atoi(s); *p=tmp;
         for (++val;val<=max;++val)
         {
            vBrush_editBrush_Op(brFind(val),op);
            any_new_groups|=new_group;
         }
         s=p;
      }
   } while (*s!='\0');
   if (any_new_groups)
      getToCurGroup();
   gedit_full_redraw();
}

static void vbrui_add_brush_by_id(char *args)
 { change_brushes_by_id(args,vBrush_OP_ADD); }

static void vbrui_rem_brush_by_id(char *args)
 { change_brushes_by_id(args,vBrush_OP_REM); }

static void vbrui_tog_brush_by_id(char *args)
 { change_brushes_by_id(args,vBrush_OP_TOG); }

// @TODO: code this right
static void vbrui_group_cycle(int arg)
{
   int targ_Group;
   if (arg==0) targ_Group=last_Group;
   else targ_Group=(cur_Group+arg)&0xff;  // really should have a max vBrush or something
   if (targ_Group!=0)
   {  // correctly validate/invalidate virtual brush here
      int tmp=cur_Group;
      BOOL fixup=(brSelect_Group(vBrush_editBrush_Get())!=targ_Group);
      cur_Group=targ_Group;
      last_Group=tmp;
      if (fixup)
         if (!vBrush_getToCurGroup())
            if (arg==0)
               abandon_Group();
            else  // should really go look for a used group
               cur_Group=vBrush_NextUsedGroup(cur_Group,arg);
   }
}

static void vbrui_delete_brush(void)
{
   vBrush_DeletePtr(vBrush_editBrush_Get());
}

static void vbrui_clone_brush(void)
{
   vBrush_ClonePtr(vBrush_editBrush_Get());
}

static void vbrui_build_brush(void)
{
   vBrush_CreateSingle();
}


// if a 2 brush object group, sets src and dst to start and end of brush


static char path_buf[256];
static char *make_brush_fname(char *str)
{
   char dir_name[256];
   if (!config_get_raw("vBrush_dir",dir_name,256))
      strcpy(dir_name,".");
   diskPathMunge(path_buf,dir_name,NULL,str);
   return path_buf;
}

// filter for save load fun
static sFileMenuFilter db_file_filters[] = 
{
   { "MultiBrush Files (*.vbr)", "*.vbr"},
   { "All Files (*.*)", "*.*"},
   { NULL }
};

static char _fname_buf[256]="";

static char *get_real_fname(char *str, eFileMenuMode fMode)
{
   if (str != NULL)
      while (isspace(*str)) str++;
   if (str == NULL || *str == '\0')
   {
      str=FileDialog(fMode, db_file_filters, "VBR", _fname_buf, sizeof(_fname_buf));
      if (str == NULL || *str == '\0')
         return NULL;
   }
   else
      str=make_brush_fname(str);
   return str;
}

void br_update_obj(editBrush *us)
{
   if (brushGetType(us) == brType_OBJECT)
      editobjUpdateObjfromBrush(us);
}

// @NEEDUNDO
// load a new group from disk
static void vbrui_load_group(char *str)
{
   int newGroup=vBrush_GetFreeGroup();
   mxs_vector loc_origin;

   if ((str=get_real_fname(str,kFileMenuLoad))==NULL)
      return;
   find_brush_origin(vBrush_GetSel(),&loc_origin,NULL);
   if (editor_LoadMiniBrush(str,newGroup))
   {  // now go translate into local frame of reference, ie. where we are
      br_trans_vec=&loc_origin;
      cur_Group=newGroup;
      vBrush_GroupOp(FALSE,br_translate);    // zero relative them all
      vBrush_GroupOp(FALSE,br_update_obj);    
      vBrush_getToCurGroup();
      Status("Loaded Group");
   }
   else
      Status("Couldn't load Group");
}

// save of cur_Group to file 
static void vbrui_save_group(char *str)
{
   if (cur_Group)
   {
      mxs_vector loc_origin;

      if ((str=get_real_fname(str,kFileMenuSave))==NULL)
         return;
      br_trans_vec=&loc_origin;
      find_brush_origin(vBrush_GetSel(),br_trans_vec,NULL);
      mx_negeq_vec(br_trans_vec);
      vBrush_GroupOp(FALSE,br_translate);    // zero relative them all
      vBrush_GroupOp(FALSE,br_update_obj);    
      editor_SaveMiniBrush(str,cur_Group);
      mx_negeq_vec(br_trans_vec);
      vBrush_GroupOp(FALSE,br_translate);    // and now put them back
      vBrush_GroupOp(FALSE,br_update_obj);    

      Status("Saved group");
   }
   else
      Status("No current group");
}

// should go somewhere better, eh?
void brFlag_Toggle(editBrush *us, int flag)
{
   if (brSelect_Flag(us)&flag)
      brSelect_Flag(us)&=~flag;
   else
      brSelect_Flag(us)|= flag;
}

// 
void vBrush_click(editBrush *us, int flags, int x, int y)
{
   editBrush *last=vBrush_GetSel();
//   mprintf("vbClick: us %x last %x... flags %x - grp us %d last %d cur %d\n",
//            us,last,flags,brSelect_Group(us),last_Group,cur_Group);
   if (flags&GEDIT_CLICK_FANCY) // well, still in a valid brushSetupDraw
      brushSelectFancy(us,x,y);
   if ((last==us)&&((flags&GEDIT_CLICK_NOTOGGLE)==0)&&
       ((flags&(GEDIT_CLICK_FANCY|GEDIT_CLICK_VBRUSH))==0))
   {  // if we are not multi and our brushType has complex type (terrain), toggle complex flag
      if ((brSelect_Flag(us)&brSelect_VIRTUAL)==0)
         if ((brushGetType(us)==brType_HOTREGION)||
             (brushGetType(us)==brType_TERRAIN))
            brFlag_Toggle(us,brSelect_COMPLEX);       // if !click_fancy or Vbrush, you just reclicked
   }
   if (flags&GEDIT_CLICK_VBRUSH)
   {
      vBrush_editBrush_Op(us,vBrush_OP_TOG);
      if (brSelect_Group(us)==0)  // this is a "did Tog remove us from the group" hack
         if (brSelect_Group(last)==cur_Group)
            us=last;              // @HACK: so the select will just go back to us
         else
         {
            vBrush_getToCurGroup();  // if so, get back to our groups  (was a minor_update here?)
            return;               // since we dont want to do the select
         }
      else if (new_group) // we want to, if this is a new group, make last in our group tooo!!
         if ((last!=us)&&(brSelect_Group(last)==0)) // last not in another group
            brSelect_Group(last)=brSelect_Group(us);
   }
   vBrush_SelectBrush(us);        // if not a vbrush op, then just select us
}

// @TODO: real focusing Code!!! - modularize this crap, jesus
static void vbrui_click(void)
{
   vBrush_click(vBrush_GetSel(),GEDIT_CLICK_VBRUSH,0,0);
}

void vBrush_go_EOT(void)
{  
   editBrush *cursor=vBrush_GetSel();

   if (cur_Group==0)   // just do us
      br_eot(cursor);
   else
      run_simple_bounded_group_br_op(br_eot);
   vBrush_SelectBrush(cursor);
   gedit_db_update(FALSE);
}

// raw cycling code
int cycle_us(int val, int dir, int max, int min)
{
   val+=dir;
   if (val==max)     val=min;
   else if (val<min) val=max-1;
   return val;
}

// @TODO: figure out if this should use GetSel or editBrush_Get????
static void vbrui_cycle_face(int dir)
{
   editBrush *us=vBrush_GetSel();
   brSelect_Flag(us)&=~brSelect_COMPLEX;
   us->cur_face=cycle_us(us->cur_face,dir,us->num_faces,-1);   // should just use our internal sense of it
   gedit_redraw_selection();
}

static void vbrui_cycle_edge(int dir)
{
   editBrush *us=vBrush_GetSel();
   brSelect_Flag(us)|=brSelect_COMPLEX|brSelect_EDGE;
   us->edge=(char)cycle_us(us->edge,dir,primalBr_EdgeCnt(us->primal_id),0);
   gedit_redraw_selection();
}

static void vbrui_cycle_point(int dir)
{
   editBrush *us=vBrush_GetSel();   
   brSelect_Flag(us)|=brSelect_COMPLEX;
   brSelect_Flag(us)&=~brSelect_EDGE;
   us->point=(char)cycle_us(us->point,dir,primalBr_PointCnt(us->primal_id),0);
   gedit_redraw_selection();
}

// virtual brush command stuff
Command vbrush_keys[] =
{
   { "delete_brush", FUNC_VOID, vbrui_delete_brush },
   { "insert_brush", FUNC_VOID, vbrui_clone_brush },
   { "new_brush", FUNC_VOID, vbrui_build_brush },
   { "add_brush_num", FUNC_STRING, vbrui_add_brush_by_id },
   { "rem_brush_num", FUNC_STRING, vbrui_rem_brush_by_id },
   { "tog_brush_num", FUNC_STRING, vbrui_tog_brush_by_id },
   { "cycle_brush", FUNC_INT, vbrui_cycle_brush },
   { "cycle_context", FUNC_INT, vbrui_cycle_context, "cycle to next like brush (vBrush or type)" },
   { "cycle_highlight", FUNC_INT, vbrui_cycle_highlight, "cycle to next highlighted brush" },
   { "cycle_near", FUNC_INT, vbrui_cycle_near, "cycle to brushes near to current brush" },
   { "store_group", FUNC_VOID, vbrui_save_vbrush, "save cur vBrush to new ID" },
   { "dissolve_group", FUNC_VOID, vbrui_breakup_vbrush, "remove group ID of vBrush" },
   { "pick_group_name", FUNC_STRING, vbrui_select_by_name, "pick group by name" },
   { "cycle_group", FUNC_INT, vbrui_group_cycle, "move group by arg (0 is last)" },
   { "brush_relative", TOGGLE_BOOL, &vBrush_relative_motion },
   { "axial_scale", TOGGLE_BOOL, &vBrush_axial_scale },
   { "cycle_face", FUNC_INT, vbrui_cycle_face },
   { "cycle_edge", FUNC_INT, vbrui_cycle_edge },
   { "cycle_point", FUNC_INT, vbrui_cycle_point },
   { "brush_select", FUNC_INT, vbrui_select_brid },
   { "obj_brush_select", FUNC_INT, vbrui_select_obj, "set current brush to objID" },
   { "vBrush_click", FUNC_VOID, vbrui_click, "alt-click cur brush" },  // , auto last_Group if no multi" },
   { "vBrush_EOT", FUNC_VOID, vBrush_go_EOT, "send to end of time" },
   { "load_group", FUNC_STRING, vbrui_load_group },
   { "save_group", FUNC_STRING, vbrui_save_group },
#ifdef DBG_ON
   { "vBrush_dump", FUNC_INT, vBrush_DebugInfo },
#endif
};

void vBrush_UI_register(void)
{
   COMMANDS(vbrush_keys, HK_BRUSH_EDIT);
}

/////////////
// setup/so on

#define VB_NN_NAME "MultiBrush"
#define VB_NN_SIZE 256

// initialize virtual brush tracking system
void vBrushInit(void)
{
   vBrush_UI_register();
   pNN_MultiBrush=NamenumInit(VB_NN_NAME,VB_NN_SIZE);
}

void vBrushReset(void)
{
   if (pNN_MultiBrush)
      NamenumFree(pNN_MultiBrush);      
   pNN_MultiBrush=NamenumInit(VB_NN_NAME,VB_NN_SIZE);
}

void vBrushFree(void)
{
   if (pNN_MultiBrush)
      NamenumFree(pNN_MultiBrush);
   clear_BrushVecs();
}

///////////
// save/load setup

BOOL vBrush_NameSave(ITagFile *file)
{
   if (pNN_MultiBrush)
      return NamenumSave(pNN_MultiBrush,file);
   return FALSE;
}

BOOL vBrush_NameLoad(ITagFile *file)
{
   if (pNN_MultiBrush)
      NamenumFree(pNN_MultiBrush);      
   pNN_MultiBrush=NamenumLoad(VB_NN_NAME,file);
   if (pNN_MultiBrush)
      return TRUE;
   pNN_MultiBrush=NamenumInit(VB_NN_NAME,VB_NN_SIZE);   
   return FALSE;
}

// call this when about to save
int vBrushGetSaveData(void)
{
   return cur_Group;
}

// call this after load, with value from the save
// assumes you will vBrush_SetPos soon
int vBrushParseLoadData(int saved)
{
   cur_Group=saved;
   last_Group=0;
   return cur_Group;
}
