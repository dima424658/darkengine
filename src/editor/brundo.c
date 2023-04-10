/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brundo.c,v 1.14 2000/02/19 12:27:48 toml Exp $
// brush level undo code

#include <stdlib.h>     // all the S's
#include <string.h>
#include <stdio.h>

#include <lg.h>
#include <mprintf.h>
#include <command.h>

#include <editbr.h>
#include <editbr_.h>
#include <brlist.h>
#include <vbrush.h>
//#include <gedit.h>
#include <ged_undo.h>
#include <undoredo.h>
#include <brundo.h>
#include <brquery.h>
#include <brinfo.h>
#include <editobj.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static BOOL editUndoInProgress=FALSE;
static BOOL editUndoSuspended=FALSE;
static editBrush start_brush;

extern bool set_brush_to_time_n(editBrush *br, int n);

//#define UNDO_STACK_TRACE
#ifdef UNDO_STACK_TRACE
#define editUndoStoreAndTraceAction(action) do { undoStoreAction(action); editUndoStackPrint(); } while (0)
#else
#define editUndoStoreAndTraceAction(action) undoStoreAction(action)
#endif

#define noUndoRedoActions (editUndoInProgress||editUndoSuspended)

typedef struct {
   int type;
   char data[];
} undoGeneric;

typedef struct {
   int type;
   int time;
   editBrush total;  // has the brush id inside it, of course
} undoBrushState;

typedef struct {
   int type;
   int data1, data2;
} undoData;

#define UNDO_BRUSH     1
#define UNDO_DELETE    2
#define UNDO_CREATE    3
#define MAX_UNDO_BRUSH_OP (UNDO_CREATE)

#define UNDO_BLOCK_ST  4
#define UNDO_BLOCK_END 5
#define UNDO_GROUP     6

#ifdef FANCY_DELTA_STUFF

// a delta structure looks like
// typedef struct {
//    uchar len;
//    uchar offset;
//    uchar old[len];
//    uchar new[len];
// } delta;
//
// the deltalist_header starts with the current total length

#define DELTA_HEADER_SIZE        (sizeof(uchar)*2)
#define DELTALIST_HEADER_SIZE    (sizeof(short)*1)
#define delta_header(len,offset)
#define delta_size(len)          (DELTA_HEADER_SIZE+sizeof(uchar)*len)
#define deltalist_get_len(dl)    (*((short *)dl))

static void *_storeDelta(void **dlist, uchar *o, uchar *n, int offset, int len)
{
   uchar *ptr;
   int deltalist_len=0;
   if (dlist==NULL)
      *dlist=(void *)Malloc(DELTALIST_HEADER_SIZE+delta_size(len));
   else
   {
      deltalist_len=(int)deltalist_get_len(dlist);
      *dlist=(void *)Realloc(dlist,deltalist_len+delta_size(len));
   }
   ptr=((uchar *)*dlist)+deltalist_len;
   *ptr++=(uchar)len;
   *ptr++=(uchar)offset;
   memcpy(ptr,o+offset,len);
   memcpy(ptr+len,n+offset,len);
   *((short *)*dlist)=(short)deltalist_len+delta_size(len);
   return *dlist;
}

// returns a deltalist that it has malloced
static void *_buildBrushDelta(editBrush *old_br, editBrush *new_br)
{
   uchar *o=(uchar *)old_br, *n=(uchar *)new_br;
   void  *delta=NULL;
   int   i, missmatch=-1;

   for (i=0; i<sizeof(editBrush); i++)
   {
      bool diff=(*(o+i)!=*(n+i));
      if (diff&&(missmatch==-1))
         missmatch=i;
      if (!diff&&(missmatch!=-1))
      {
         delta=_storeDelta(&delta,o,n,missmatch,i-missmatch);
         missmatch=-1;
      }
   }
   return delta;
}
#endif

// this is to be called before performing an action which will modify a brush
// for now it saves the whole brush
// TODO: eventually, it should check to see if top of undo stack is this
//  brush, and know not to bother if it is.  unless your last actions was an
//  undo, in which case it should.  of course, really really it should scan
//  the brush and record just the delta.  hmmm, maybe ill do that now.
// so for now, full is ignored, since we always do full
static void *_buildBrushState(int type, editBrush *us, bool full)
{
   undoBrushState *newU;
   newU=(undoBrushState *)Malloc(sizeof(undoBrushState));
   newU->type=type;
   newU->time=blistCheck(us);
   newU->total=*us;
   return newU;
}

static void *_buildData(int type, int data1, int data2)
{
   undoData *newU;
   newU=(undoData *)Malloc(sizeof(undoData));
   newU->type=type;
   newU->data1=data1;
   newU->data2=data2;
   return newU;
}

static void *_restoreBrushState(undoGeneric *undoRec, bool create)
{
   undoBrushState *lastU=(undoBrushState *)undoRec;
   editBrush *us;
   void *rv=NULL;
   
   if (create)
   {
      us=brushCopy(&lastU->total);
      us->br_id=lastU->total.br_id; // preseve brush id

      //  Undelete objects properly
      //
      if (brushGetType(us) == brType_OBJECT)
      {
          editObjCreateObjFromBrush(OBJ_NULL, us);
      }
      blistInsert(us);              // and insert, so if we reverse it brush id will still be valid
      blistSetPostoBrush(us);
      set_brush_to_time_n(us, lastU->time);
      we_switched_brush_focus();
   }
   else
   {  // for now, this gross hack to try and make undo/redo for texture changes more useful

      if (lastU->total.br_id==VIRTUAL_BRUSH_ID)
      {
         if ((us=vBrush_UndoVBrush(&lastU->total))!=NULL)
         {
            rv=_buildBrushState(UNDO_BRUSH,us,FALSE);
            *us=lastU->total;
         }
         else
            Warning(("MultiBrush Undo failed\n"));
      }
      else if ((us=brFind(lastU->total.br_id))!=NULL)
      {
         BOOL tdelta=gedundo_check_texture_delta(&lastU->total,us);
         rv=_buildBrushState(UNDO_BRUSH,us,FALSE);
         *us=lastU->total;
         if (tdelta)
            gedundo_do_texture_delta(us);
      }
      else
         Warning(("Cant find undo brush %d!!\n",lastU->total.br_id));
   }
   return rv;
}

// for now, just saves off the current brush, for use or not
void editUndoStoreStart(editBrush *us)
{
   if (!noUndoRedoActions && (us!=NULL))
      start_brush=*us;
}

// this is the guts of the undo thinking - which will someday be smart and sensible...
// till then, this hack

// ok, lastU->total is the brush in the current undo (ie. state we will currently undo to)
//     start_brush is where we started this frame (often != lastU)
//     us is the brush as it currently exists
// we hand them to gedit, it tells us existing thing is ok, or to save it
void editUndoStoreBrush(editBrush *us)
{
   undoBrushState *lastU=(undoBrushState *)undoPeek();
   editBrush *base=NULL;

   if (noUndoRedoActions) return;  // well, the easy case
   if (lastU&&lastU->type==UNDO_BRUSH) base=&lastU->total;
   if (gedundo_check_brush_delta(us,&start_brush,base))
   {
      void *newAction=_buildBrushState(UNDO_BRUSH,&start_brush,FALSE);
      editUndoStoreAndTraceAction(newAction);
//      mprintf("new action for brush %d record (tx %d - old %d)\n",us->br_id,us->tx_id,start_brush.tx_id);
   }
//   else
//      mprintf("punt store of brush, we think it is the same\n");
}

// these both store full, so that neither needs to change for redo or anything
// ie. the del and create records are just backwards versions of eachother
//   which have the same packet, and direction+type indicates which to do
void editUndoStoreDelete(editBrush *us)
{
   if (!noUndoRedoActions)
      editUndoStoreAndTraceAction(_buildBrushState(UNDO_DELETE,us,TRUE));
}

void editUndoStoreCreate(editBrush *us)
{
   if (!noUndoRedoActions)
      editUndoStoreAndTraceAction(_buildBrushState(UNDO_CREATE,us,TRUE));
}

// this is used to store off block starts and stops
// so that you can treat a set of undo's as a block
// and a single call to undo/redo will do all of them
void editUndoStoreBlock(BOOL start_of_block)
{
   undoBrushState *lastU=(undoBrushState *)undoPeek();

   if (noUndoRedoActions) return;
   if (!start_of_block && (lastU!=NULL) && (lastU->type==UNDO_BLOCK_ST))
      undoKillStackTop();
   else
      editUndoStoreAndTraceAction(_buildData(start_of_block?UNDO_BLOCK_ST:UNDO_BLOCK_END,-1,-1));
}

void editUndoStoreGroup(int old_group, int new_group)
{
   if (!noUndoRedoActions)
      editUndoStoreAndTraceAction(_buildData(UNDO_GROUP,old_group,new_group));
}

void editUndoDoGroupOp(undoData *groupOp, BOOL is_undo)
{
   if (is_undo)
      vBrush_GoToGroup(groupOp->data1);
   else
      vBrush_GoToGroup(groupOp->data2);
}

void editUndoSuspend(BOOL suspend)
{
   editUndoSuspended=suspend;   
}

static int getBrushIdfromUndo(undoGeneric *und)
{
   undoBrushState *lastU=(undoBrushState *)und;
   return lastU->total.br_id;
}

bool editUndoDoUndo(void)
{
   int in_block=0;
   editBrush *us;
   BOOL rv=TRUE;

   editUndoInProgress=TRUE;
   do {
      undoGeneric *lastUndo=(undoGeneric *)undoDoUndo();
      if (lastUndo==NULL)
         rv=FALSE;
      else
         switch (lastUndo->type)
         {
            case UNDO_GROUP:
               editUndoDoGroupOp((undoData *)lastUndo,TRUE);
               break;
            case UNDO_BLOCK_ST:
               in_block--;
               break;
            case UNDO_BLOCK_END:
               in_block++;
               break;
            case UNDO_BRUSH:
               undoUndoReplace(_restoreBrushState(lastUndo,FALSE));
               break;
            case UNDO_CREATE:
               us=brFind(getBrushIdfromUndo(lastUndo));
               vBrush_DeletePtr(us);
               break;
            case UNDO_DELETE:
               _restoreBrushState(lastUndo,TRUE);
               break;
         }
   } while (in_block&&rv);
   editUndoInProgress=FALSE;
   return rv;
}

bool editUndoDoRedo(void)
{
   int in_block=0;   
   editBrush *us;
   BOOL rv=TRUE;

   editUndoInProgress=TRUE;   
   do {
      undoGeneric *lastUndo=(undoGeneric *)undoDoRedo();
      if (lastUndo==NULL)
         rv=FALSE;
      else
         switch (lastUndo->type)
         {
            case UNDO_GROUP:
               editUndoDoGroupOp((undoData *)lastUndo,FALSE);
               break;
            case UNDO_BLOCK_ST:
               in_block++;
               break;
            case UNDO_BLOCK_END:
               in_block--;
               break;
            case UNDO_BRUSH:
               undoRedoReplace(_restoreBrushState(lastUndo,FALSE));
               break;
            case UNDO_CREATE:
               _restoreBrushState(lastUndo,TRUE);
               break;
            case UNDO_DELETE:
               us=brFind(getBrushIdfromUndo(lastUndo));
               vBrush_DeletePtr(us);
               break;
         }
   } while (in_block&&rv);
   editUndoInProgress=FALSE;   
   return rv;
}

#ifdef DBG_ON
void _undostackwalkcallback(void *action, bool next)
{
   undoGeneric *tmpUndo=(undoGeneric *)action;
   mprintf("%s",next?"[":"(");
   if (tmpUndo==NULL)
      mprintf("NULL");
   else if (tmpUndo->type<=MAX_UNDO_BRUSH_OP)
      mprintf("%d %d",
              ((undoGeneric *)action)->type,
              ((undoBrushState *)action)->total.br_id);
   else
      mprintf("%d",((undoGeneric *)action)->type);
   mprintf("%s",next?"]":")");
}

void editUndoStackPrint(void)
{
   undoDumpStack(_undostackwalkcallback);
}
#endif // DBG_ON