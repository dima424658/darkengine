/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_undo.c,v 1.3 2000/02/19 13:10:48 toml Exp $
// undo support

#include <string.h>
#include <stdlib.h>

#include <lg.h>

#include <editbr.h>
#include <editbr_.h>
#include <undoredo.h>
#include <brinfo.h>
#include <brundo.h>
#include <gedit.h>
#include <ged_undo.h>
#include <vbrush.h>

#include <status.h>
#include <command.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/// absolutely hideous editbr_ specific craziness

#define _DELTA_TX_ID     0
#define _DELTA_POS_X     1
#define _DELTA_POS_Y     2
#define _DELTA_POS_Z     3
#define _DELTA_SZ_X      4
#define _DELTA_SZ_Y      5
#define _DELTA_SZ_Z      6
#define _DELTA_ANG_X     7
#define _DELTA_ANG_Y     8
#define _DELTA_ANG_Z     9
#define _DELTA_TEXBASE  10
#define _DELTA_TOFF_ID   0
#define _DELTA_TOFF_ROT  1
#define _DELTA_TOFF_SC   2
#define _DELTA_TOFF_X    3
#define _DELTA_TOFF_Y    4
#define _DELTA_TOFF_CNT  5
#define MAX_DELTAS     (_DELTA_TEXBASE + (EDITBR_MAX_FACES*_DELTA_TOFF_CNT))

static int get_delta_info(int delta_code, int *size, editBrush *br)
{
   if (delta_code>=_DELTA_TEXBASE)
   {
      int which_face;
      delta_code-=_DELTA_TEXBASE;
      which_face=(delta_code/5);
      delta_code%=5;
      *size=2;  // since i happen to know sizeof(fixang=short=ushort)==2
      switch (delta_code)
      {
         case _DELTA_TOFF_ID:  return (int)(((uchar *)&br->txs[which_face].tx_id)-(uchar *)br);
         case _DELTA_TOFF_ROT: return (int)(((uchar *)&br->txs[which_face].tx_rot)-(uchar *)br);
         case _DELTA_TOFF_SC:  return (int)(((uchar *)&br->txs[which_face].tx_scale)-(uchar *)br);
         case _DELTA_TOFF_X:   return (int)(((uchar *)&br->txs[which_face].tx_x)-(uchar *)br);
         case _DELTA_TOFF_Y:   return (int)(((uchar *)&br->txs[which_face].tx_y)-(uchar *)br);
      }
   }
   else if (delta_code==_DELTA_TX_ID)
   {
      *size=sizeof(short);
      return (int)((uchar *)&br->tx_id-(uchar *)br);
   }
   else if (delta_code<=_DELTA_SZ_Z)
   {
      *size=sizeof(mxs_real);
      return (int)((uchar *)(((float *)(&br->pos.el[0]))+delta_code-_DELTA_POS_X)-(uchar *)br);
   }
   else  // for now, this means an angle
   {
      *size=sizeof(mxs_ang);
      return (int)((uchar *)(((mxs_ang *)(&br->ang.el[0]))+delta_code-_DELTA_ANG_X)-(uchar *)br);
   }
   return 0;
}

static int get_max_deltas(editBrush *br)
{
   if (brushGetType(br)!=brType_TERRAIN)
      return _DELTA_TEXBASE-1;
   else
      return _DELTA_TEXBASE-1+(br->num_faces*_DELTA_TOFF_CNT);
}

// returns number of entries it filled of buf, where each is a #def for which field is changed
static int get_delta_list(char *buf, editBrush *b1, editBrush *b2)
{
   int max=get_max_deltas(b1), i;
   int sz, cnt=0, offset;
   for (i=0; i<max; i++)
   {
      offset=get_delta_info(i,&sz,b1);
      if (memcmp(((uchar *)b1)+offset,((uchar *)b2)+offset,sz)!=0)
         buf[cnt++]=(char)i;
   }
   return cnt;
}

// called from within undo/redo to make decisions about what to do
BOOL gedundo_check_brush_delta(editBrush *new_ver, editBrush *old_ver, editBrush *base_ver)
{  
   int real_size=sizeof(editBrush)-(EDITBR_MAX_FACES*sizeof(TexInfo));
   char delta_1[MAX_DELTAS], delta_2[MAX_DELTAS];
   int delta_1_cnt, delta_2_cnt;
   editBrush tmp=*new_ver;
   
   if (brushGetType(&tmp)==brType_TERRAIN)
      real_size+=tmp.num_faces*(EDITBR_MAX_FACES*sizeof(TexInfo));
   tmp.cur_face=old_ver->cur_face;  // move across all data we dont care about diffs in
   tmp.edge=old_ver->edge; tmp.point=old_ver->point; tmp.use_flg=old_ver->use_flg;
   if (memcmp(&tmp,old_ver,real_size)==0)   // for what we care about - these two brushes are the same
      return FALSE;
   if (base_ver==NULL) return TRUE;         // w/o base, any diff is a real diff
   if (base_ver->br_id!=new_ver->br_id) return TRUE; // different brush ids, any diff is a real diff
   if (base_ver->num_faces!=new_ver->num_faces)
   {
      Warning(("Whoa! this shouldnt be different: faces %d %d %d\n",base_ver->num_faces,old_ver->num_faces,new_ver->num_faces));
      return TRUE; // something
   }
   delta_1_cnt=get_delta_list(delta_1,base_ver,old_ver); // what diffs from base->old
   delta_2_cnt=get_delta_list(delta_2,old_ver,new_ver);  // what diffs from old->new
   if (delta_1_cnt!=delta_2_cnt) return TRUE;            // different number of them, bad day
   //   if (delta_1_cnt>1) return TRUE;                     // this would be for not allowing common multi-axis undo
   return memcmp(delta_1,delta_2,delta_1_cnt)!=0;        // if the delta are the same, punt
}

static BOOL tdelta_texture_only;
BOOL gedundo_check_texture_delta(editBrush *new_ver, editBrush *old_ver)
{
   BOOL txt_diff, align_diff=FALSE;
   int i;
   
   txt_diff=(new_ver->tx_id!=old_ver->tx_id);
   for (i=0; i<min(new_ver->num_faces,old_ver->num_faces); i++)
      if (memcmp(&new_ver->txs[i].tx_rot,&old_ver->txs[i].tx_rot,sizeof(TexInfo)-sizeof(short))!=0)
         align_diff=TRUE;
      else if (new_ver->txs[i].tx_id!=old_ver->txs[i].tx_id)
         txt_diff=TRUE;
   tdelta_texture_only=!align_diff;
   return (align_diff||txt_diff);
}

void gedundo_do_texture_delta(editBrush *us)
{
   gedit_reassign_texture(us,tdelta_texture_only);
}

// utilities
static BOOL temp_safety_hack_abort(editBrush *p)
{
#ifdef NO_MBRUSH_UNDO   
   if (brSelect_Flag(p)&brSelect_VIRTUAL)
   {
      Status("Sorry, Multibrush Undo broken");
      return TRUE;
   }
#endif   
   return FALSE;
}

//#define UNDO_STACK_TRACE
#ifdef UNDO_STACK_TRACE
#define traceUndoStack() editUndoStackPrint()
#else
#define traceUndoStack()
#endif

// undo-redo UI
static void gedundo_undo(int count)
{  // 0 or 1 means 1
   bool rv=TRUE;
   if (temp_safety_hack_abort(vBrush_editBrush_Get()))
      return;
   do {
      traceUndoStack();
      rv&=editUndoDoUndo();
      traceUndoStack();      
   } while (--count>0);
   if (rv) Status("Undo!");
   else    Status("No more Undo information.");
   if (rv||(count>1))
      gedit_full_redraw();
}

static void gedundo_redo(int count)
{  // 0 or 1 means 1
   bool rv=TRUE;
   if (temp_safety_hack_abort(vBrush_editBrush_Get()))
      return;
   do {
      traceUndoStack();      
      rv&=editUndoDoRedo();
      traceUndoStack();      
   } while (--count>0);
   if (rv) Status("Redo!");
   else    Status("No more Redo information.");
   if (rv||(count>1))
      gedit_full_redraw();   
}

/////////////////////////////
// command declaration

#ifdef DBG_ON
static void gedit_undo_stack_trace(void) { editUndoStackPrint(); }
#endif

Command gedundo_keys[] =
{
   { "undo", FUNC_INT, gedundo_undo },
   { "redo", FUNC_INT, gedundo_redo },
#ifdef DBG_ON
   { "show_undo_stack", FUNC_VOID, gedit_undo_stack_trace },
#endif   
};

void gedundoCommandRegister(void)
{
   COMMANDS(gedundo_keys, HK_BRUSH_EDIT);
}
