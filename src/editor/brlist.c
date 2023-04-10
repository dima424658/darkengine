/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brlist.c,v 1.18 2000/02/19 12:27:43 toml Exp $

//
// Brush List (blist.c)
//   stores the actual sequential brush list.  has calls to insert a brush
// into the list, to delete a brush from the list, to set the brush list
// cursor position, return current brush, and other low level brush needs. 
//     - define seek_types - SEEK_SET, SEEK_CUR, SEEK_END
//     * brush_list *blistAlloc(int size_guess), blistFree(brush_list *bl)
//         allocate and free the master brush list
//     * bool blistSeek(int pos, int seek_type)
//         set blist cursor to pos, uses fseek style SEEK types
//     * int blistInsert(editBrush *new), editBrush *blistDelete()
//         works on the current cursor position
//     * editBrush *blistGet()
//         retrieves pointer to the current brush
//     * int blistCheck(editBrush *chk)
//         returns where the brush is in the blist, or -1 if not there
//     * int blistSetPostoBrush(editBrush *targ)
//         sets the blist cursor to point at targ, returns pos or -1
//         (ie. this really just calls check then seek_set)
// 
// note this basically uses the dstruct lib llist 
// 

#include <stdlib.h>

#include <mprintf.h>
#include <lg.h>
#include <llist.h>
#include <hheap.h>

#include <editbr.h>
#include <brlist.h>

#include <config.h>
#include <command.h>
#include <status.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

extern BOOL brushIsLocked (editBrush* brush);

typedef struct _brushElem {
   struct _llist;       // the list data
   editBrush *data;     // the data in the list
} brushElem;            // note how brutally memory inefficient this is

// this is typedef'ed to brush_list to brlist.h
struct _brush_list {
   llist_head list;     // the actual list
   int flags;           // status flags and such
   int count;           // how big is the list
   brushElem *cursor;   // the actual cursor object
   int cursor_pos;      // the actual "index" of the cursor
   HheapHead brushHeap; // the data storage for brush elements
};

// the current master brush list
static brush_list *cur_bl;

static void set_cursor(brush_list* bl, brushElem* curs, int pos)
{
   bl->cursor= curs;
   bl->cursor_pos = pos;
   if (blist_new_cursor_func)
      blist_new_cursor_func((curs != NULL) ? curs->data : NULL);
}


#define _blistZeroCursor(bl) set_cursor(bl,NULL,0)

// attempt to allocate the master brush list, return NULL if you fail
brush_list *blistAlloc(int size_guess)
{
   brush_list *us;      // we will try and allocate us here

   if (size_guess<256) size_guess=256;
   us=(brush_list *)Malloc(sizeof(brush_list)); // allocate brush
   if (us!=NULL) 
   {
      llist_init(&us->list);  // set up the dstruct llist type
      us->flags=0;
      us->count=0;
      _blistZeroCursor(us);   // now the cursor setup... initially point at head
      HheapInit(&us->brushHeap, sizeof(brushElem), 128, TRUE);
   }                    // we use an Hheap to store the brushElems
   return us;           // return us, which is NULL if we failed
}

// free a brush list and all associated brushes
bool blistFree(brush_list *bl)
{
#ifdef DBG_ON
   int cnt=HheapGetNumItems(&bl->brushHeap);
   if (cnt!=0)
      Warning(("Freeing brushElems with %d alloced brushes\n",cnt));
#endif   
   HheapFreeAll(&bl->brushHeap);
   Free(bl);
#ifdef DBG_ON
   if (bl==cur_bl)
      cur_bl=NULL;
#endif
   return TRUE;
}

// set the current brush list (implicit arg to all blist functions
void blistSetCurrent(brush_list *bl)
{
   cur_bl=bl;
   if (bl != NULL && blist_new_cursor_func != NULL)
      blist_new_cursor_func((bl->cursor != NULL) ? bl->cursor->data : NULL);
}

// set blist cursor to pos, uses fseek style SEEK types, false if out of range
bool blistSeek(int pos, int seek_type)
{
   int target_pos, ptr_pos; // where we want the cursor, where the ptr is
   brushElem *ptr;          // ptr is used to walk through the list

   switch (seek_type)   // first find the target position
   {
   case SEEK_CUR: target_pos=cur_bl->cursor_pos+pos; break;
   case SEEK_SET: target_pos=pos; break;
   case SEEK_END: target_pos=cur_bl->count+pos; break;      // -1
   default: return FALSE;
   }

   if ((target_pos<0)||(target_pos>cur_bl->count))          // >=
      return FALSE;             // out of range, go home
   if ((seek_type==SEEK_CUR)&&(target_pos==cur_bl->count))
      return FALSE;             // HACK: to avoid double stepping on last one

   if (target_pos<=(cur_bl->count/2))
      if ((cur_bl->cursor==NULL)|| 
          (abs(target_pos-cur_bl->cursor_pos)>target_pos))
      {
         ptr_pos=0;             // start at the head
         ptr=llist_head(&cur_bl->list);
      }
      else
      {                         // go from the cursor
         ptr_pos=cur_bl->cursor_pos;
         ptr=cur_bl->cursor;
      }
   else
      if ((cur_bl->cursor==NULL)||
          (abs(target_pos-cur_bl->cursor_pos)>cur_bl->count-target_pos))
      {                         // start at the tail
         ptr_pos=cur_bl->count-1;
         ptr=llist_tail(&cur_bl->list);
      }
      else 
      {                         // go from the cursor
         ptr_pos=cur_bl->cursor_pos;
         ptr=cur_bl->cursor;
      }
   if (target_pos<ptr_pos)
   {
      if (ptr_pos==cur_bl->count)   // woo-woo, wacky secret end of list
         ptr_pos--;                 // hack compensation
      while (ptr_pos!=target_pos)
      {                         // target is before us, so go back
         ptr=llist_prev(ptr);
         ptr_pos--;
      }
   }
   else 
      while (ptr_pos!=target_pos)
      {                         // target after us, cruise up the list
         ptr_pos++;                 // unless secret past end is insert point
         if (ptr_pos<cur_bl->count)    // hack to do the secret off list
            ptr=llist_next(ptr);       // idx for the end of the list
      }
   if (ptr==llist_end(&cur_bl->list))
      ptr=NULL;
   set_cursor(cur_bl,ptr,ptr_pos);
   return TRUE;                 // go home
}

// insert to the current brush list position
int blistInsert(editBrush *new)
{
   brushElem *ours;
   int ins_point=cur_bl->cursor_pos;

   ours=HheapAlloc(&cur_bl->brushHeap);
   ours->data=new;
   if (cur_bl->cursor==NULL)    // inserting at top of the list
   {
      llist_add_head(&cur_bl->list,ours);       // this didnt work
   }                                            // w/o curly braces
   else
   {
      llist_insert_after(ours,cur_bl->cursor);
   }
   if (cur_bl->count==cur_bl->cursor_pos)       // we are at end of list
   {
      brushElem* curs;
      if (cur_bl->count==0)                     // if list was empty
         curs=llist_head(&cur_bl->list);
      else                                      // we go after ourselves
         curs=llist_next(cur_bl->cursor);
      set_cursor(cur_bl,curs,cur_bl->cursor_pos+1);      
   }
   cur_bl->count++;
   return ins_point;
}

// removes from the current brush list position
editBrush *blistDelete(void)
{
   editBrush *us=NULL;
   brushElem *toToast=NULL;

   if (cur_bl->count==0)
      return NULL;
   if (cur_bl->cursor==NULL)            // we are pointing at head
   {
      toToast=llist_head(&cur_bl->list);
   }
   else                                 // otherwise in list
   {
      toToast=cur_bl->cursor;           // so kill us and then move the

      do
      {
          if (cur_bl->cursor==llist_tail(&cur_bl->list))
          {
             if (cur_bl->cursor==llist_head(&cur_bl->list))
             {
                _blistZeroCursor(cur_bl);
                break;
             }
             else
             {
                set_cursor(cur_bl,llist_prev(cur_bl->cursor),cur_bl->cursor_pos-1);
             }
          }
          else
          {
              set_cursor(cur_bl,llist_next(cur_bl->cursor),cur_bl->cursor_pos);
          }

      } while (brushIsLocked (cur_bl->cursor->data) && cur_bl->cursor != toToast);

   }            // dont know what this does when at end (what next does)
   
   // if this is the last unlocked brush left...
   if (cur_bl->cursor == toToast)
       _blistZeroCursor(cur_bl);
   
   if (toToast)
   {
      us=toToast->data;
      if (brushIsLocked(us))
      {
          Status("* LOCKED *");
          return NULL;
      }

      llist_remove(toToast);
      HheapFree(&cur_bl->brushHeap,toToast);
#ifdef DBG_ON
      if (us==NULL)
         Warning(("Toasted fine, but left us a NULL brush somehow\n"));
#endif
   }
   else
      Warning(("Somehow couldnt blistDelete\n"));
   cur_bl->count--;
   return us;   // inited to NULL, so return NULL if cant delete
}

// retrieves pointer to brush structure for the current brush list pos
editBrush *blistGet(void)
{
   if (cur_bl->cursor!=NULL)
      return cur_bl->cursor->data;
   else 
      return NULL;
}

// sets the blist cursor to point at targ, returns pos or -1
//         (ie. this really just calls check then seek_set)
int blistSetPostoBrush(editBrush *targ)
{
   int pos;
   
   pos=blistCheck(targ);
   if (pos!=-1)
   {
      if ((cur_bl->cursor_pos==cur_bl->count)&&
          (cur_bl->cursor->data==targ))
         return cur_bl->cursor_pos;    // ??? or maybe -1 ???
      if (!blistSeek(pos,SEEK_SET))
         return -1;
   }
   return pos;
}

typedef struct _sBlistIter {
   brushElem   *ptr;
   int          idx;     // 0 at start, -1 if freed from beneath the top
} sBlistIter;

#define iterPtr(hnd)     (iter_stack[hnd].ptr)
#define iterBrush(hnd)  ((iter_stack[hnd].ptr)->data)
#define iterIdx(hnd)     (iter_stack[hnd].idx)

#define ITER_STACK_DEPTH 8
static sBlistIter iter_stack[ITER_STACK_DEPTH];
static int        iter_stack_ptr=-1;
static int        iter_last_used=-1;

#ifdef DBG_ON
#define blistIterDebugHeader(hnd) \
   if (((hnd)>iter_stack_ptr)||((hnd)<0)) Warning(("blistIter: bad handle %d - gonna break\n",hnd))
#else
#define blistIterDebugHeader(hnd)
#endif

editBrush *_blistIterFinish(int hnd)
{
   iter_last_used=hnd;
   if ((iterPtr(hnd)==NULL)||(iterBrush(hnd)==NULL))
   {
      blistIterDone(hnd);
      return NULL;
   }
   else
      return iterBrush(hnd);
}

editBrush *blistIterStart(int *iter_handle)
{
   if (iter_stack_ptr>=ITER_STACK_DEPTH-1)  // have we overrun our iter stack
    { Warning(("blistIter stack depth exceeded\n")); return NULL; }
   if (llist_empty(&cur_bl->list))
      return NULL;     // if empty
   *iter_handle=++iter_stack_ptr;
   iterPtr(*iter_handle)=llist_head(&cur_bl->list);   
   iterIdx(*iter_handle)=0;
   return _blistIterFinish(*iter_handle);   
}

// returns next in list
// dont screw with the list while calling this, or panic
editBrush *blistIterNext(int iter_handle)
{
   blistIterDebugHeader(iter_handle);
   if (iterPtr(iter_handle)==llist_end(&cur_bl->list))
      iterPtr(iter_handle)=NULL;
   else
   {
      iterPtr(iter_handle)=llist_next(iterPtr(iter_handle));
      iterIdx(iter_handle)++;
   }
   return _blistIterFinish(iter_handle);
}

int blistIterDone(int iter_handle)
{
   blistIterDebugHeader(iter_handle);
   if (iter_handle==iter_stack_ptr)
   {  // walk down looking for next used
      while (--iter_handle>=0)
         if (iterIdx(iter_handle)!=-1)
            break;
      iter_stack_ptr=iter_handle;
   }
   else
      iterIdx(iter_handle)=-1;
   iter_last_used=iter_stack_ptr;
   return iter_stack_ptr;
}

// returns where the brush is in the blist, or -1 if not there
// has secret gnosis of blistNext
// in particular, if you Check the current brush from Next, it is fast
int blistCheck(editBrush *chk)
{
   brushElem *loopvar;
   int i=0;

#ifdef OLD_WAY   
   if ((curIter!=NULL)&&(chk==curIter->data)) // shortcut as mentioned above
      return curIdx;                          // look, mom, we just know!
#endif
   if ((iter_last_used!=-1)&&(chk==iterBrush(iter_last_used)))
      return iterIdx(iter_last_used);
   forallinlist(brushElem,&cur_bl->list, loopvar)
   {
      if (loopvar->data==chk)
         return i;
      i++;
   }
   return -1;
}

void blistDump(void)
{
   brushElem *iter=llist_head(&cur_bl->list);
   int i=0;

   while (iter!=llist_end(&cur_bl->list))
   {
      mprintf("Elem %d> %x (%x)\n",i++,iter,iter->data);
      iter=llist_next(iter);   
   }
   mprintf("Curs %d> %x (%x)\n",
           cur_bl->cursor_pos,cur_bl->cursor,
           cur_bl->cursor?cur_bl->cursor->data:0);
   mprintf("Count is %d, hheap size %d, list %d\n",
           cur_bl->count,
           HheapGetNumItems(&cur_bl->brushHeap),
           llist_num_nodes(&cur_bl->list));
}

// get the current brush count
int blistCount(void)
{
   return cur_bl->count;
}

// where are we in time
int blistGetPos(void)
{
   return cur_bl->cursor_pos;
}

#ifndef HORRIBLE_DEBUG_EVERY_BRUSH_CURSOR_CHANGE
// call me whenever a new brush happens
void (*blist_new_cursor_func)(struct _editBrush* ) = NULL;
#else
static void hack_func(editBrush *moo)
{
   mprintf("Yep, new cursor %x\n",moo);
}

void (*blist_new_cursor_func)(struct _editBrush* ) = hack_func;
#endif
