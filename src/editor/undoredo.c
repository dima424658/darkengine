/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/undoredo.c,v 1.5 1998/08/08 16:52:11 dc Exp $
// undo system for brush/motion elements

#include <mprintf.h>
#include <lg.h>

#include <editbr.h>
#include <undoredo.h>

#include <dbmem.h>

#define UNDO_STACK_BLOCK_SIZE 1024

static void **undoStack=NULL;
static void **undoPtr, **undoMax;
static int    undoCurSize=0;

void undoStackInit(void)
{
   undoCurSize=UNDO_STACK_BLOCK_SIZE;
   undoStack=(void **)Malloc(undoCurSize*sizeof(void *));
   undoMax=undoPtr=undoStack;
}

void undoStackFree(void)
{
   if (undoStack!=NULL)
   {
      undoClearUndoStack();
      Free(undoStack);
      undoStack=NULL;
   }
}

void  _undoFreeStack(void **low, void **hi)
{
   while (low<hi)
      Free(*low++);
}

BOOL  _undoStoreStack(void *ptr)
{
   if (undoPtr-undoStack>=undoCurSize)
   {
      int new_size=undoCurSize+UNDO_STACK_BLOCK_SIZE;
      void **newStack=(void **)Realloc(undoStack,new_size*sizeof(void *));
      if (newStack==NULL)
         return FALSE;
      undoMax=(undoMax-undoStack)+newStack;
      undoPtr=(undoPtr-undoStack)+newStack;
      undoStack=newStack;
      undoCurSize=new_size;
   }
   _undoFreeStack(undoPtr,undoMax); // this free stack is here so that if you
   *undoPtr++=ptr;                  // start storing after an undo, it goes and
   undoMax=undoPtr;                 // frees the actions you are discarding
   return TRUE;
}

BOOL _undoStackReplace(void **loc, void *ptr)
{
   if (loc==undoMax)          // if you are already at end of stack
      return FALSE;           // you cant do a replace to the stack
   _undoFreeStack(loc,loc+1); // else just free this exact elem
   *loc=ptr;                  // and load over it 
   return TRUE;         
}

void  undoKillStackTop(void)
{
   if (undoPtr>undoStack)
   {
      --undoPtr;
      _undoFreeStack(undoPtr,undoMax);
      undoMax=undoPtr;
   }
}

void *_undoFetchStack(void)
{
   if (undoPtr==undoStack)
      return NULL;
   return *--undoPtr;
}

void *_undoWalkStack(void)
{
   if (undoPtr==undoMax)   // cant walk the stack, nothing to say
      return NULL;
   return *undoPtr++;      // walk to the next element
}

BOOL undoStoreAction(void *action)
{
   if (!_undoStoreStack(action))
   {
      Warning(("Hey, undo stack full - couldnt Realloc size %d block %d",undoCurSize,UNDO_STACK_BLOCK_SIZE));
      return FALSE;
   }
   return TRUE;
}

BOOL undoUndoReplace(void *ptr)
{
   return _undoStackReplace(undoPtr,ptr);
}

BOOL undoRedoReplace(void *ptr)
{
   return _undoStackReplace(undoPtr-1,ptr);
}

void *undoPeek(void)
{
   if (undoPtr>undoStack)
      return *(undoPtr-1);
   else
      return NULL;
}

void *undoDoUndo(void)
{
   return _undoFetchStack();
}

void *undoDoRedo(void)
{
   return _undoWalkStack();
}

void undoClearUndoStack(void)
{
   _undoFreeStack(undoStack,undoMax);
   undoPtr=undoMax=undoStack;    // set us back at the beginning
}

// rain is wonderful

#ifdef DBG_ON
void undoDumpStack(void (*cback)(void *action,bool next))
{
   void **ptr=undoStack;
   mprintf("undoStack @ %dof%d..",undoPtr-undoStack,undoMax-undoStack);
   while (ptr<undoMax)
   {
      (*cback)(*ptr,ptr==undoPtr-1);
      ptr++;
   }
   mprintf("\n");
}
#endif
