/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmmax.cpp,v 1.2 2000/02/19 13:25:45 toml Exp $

#include <limits.h>
#include <lg.h>
#include <string.h>
#include <shkmmax.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define MAX_POSITION_SIZE 32

#define STACKSIZE    4096

static uchar FauxStackMem[STACKSIZE];

static uchar* FauxStackBase;
static uchar* FauxStackPointer;
static uint FauxStackSize = STACKSIZE;

static uint PositionSize;
static bool (*generate_position)(void* parent, int index, bool minimizer_moves);
static int (*static_evaluator)(void* position);
static bool (*extend_horizon)(void* position);

#define StackSpew(x) 

void fstack_init()
{
   //FauxStackSize=siz;
   FauxStackBase=FauxStackPointer=FauxStackMem;
}

void fstack_push(void* data, uint siz)
{
#ifdef PLAYTEST
   if(FauxStackSize-(FauxStackPointer-FauxStackBase) < siz) {
      Warning(("push would overflow stack size\n"));
      return;
   }
#endif
   memcpy(FauxStackPointer,data,siz);
   FauxStackPointer+=siz;
   StackSpew(("pushing %d bytes, top %d\n",siz,FauxStackPointer-FauxStackBase));
}

void fstack_pop(void* data, uint siz)
{
#ifdef PLAYTEST
   if(FauxStackPointer-FauxStackBase < siz) {
      Warning(("pop would underflow stack\n"));
      return;
   }
#endif
   FauxStackPointer-=siz;
   memcpy(data,FauxStackPointer,siz);
   StackSpew(("popping %d bytes, top %d\n",siz,FauxStackPointer-FauxStackBase));
}

void* fstack_create(uint siz)
{
   void* retval;
#ifdef PLAYTEST
   if(FauxStackSize-(FauxStackPointer-FauxStackBase) < siz) {
      Warning(("create would overflow stack size\n"));
      return(NULL);
   }
#endif
   retval=FauxStackPointer;
   FauxStackPointer+=siz;
   StackSpew(("moving top %d bytes, top %d\n",siz,FauxStackPointer-FauxStackBase));
   return((void*)retval);
}

void fstack_flush(uint siz)
{
#ifdef PLAYTEST
   if(FauxStackPointer-FauxStackBase < siz) {
      Warning(("ERROR: flush would underflow stack\n"));
      return;
   }
#endif
   FauxStackPointer-=siz;
   StackSpew(("flushing top %d bytes, top %d\n",siz,FauxStackPointer-FauxStackBase));
}

#define FSTACK_PUSHVAR(v) fstack_push(&v,sizeof(v))
#define FSTACK_POPVAR(v) fstack_pop(&v,sizeof(v))

// ------------
// MiniMax procedure
//   Runs MiniMax evaluation of board position, using user-supplied
// stack for recursion.
//

void minimax_get_result(int* val,char* which)
{
   fstack_pop(val,sizeof(int));
   fstack_pop(which,sizeof(char));
}

bool minimax_done(void)
{
   // done if only value (an int) and move (a char) remain on stack
   return(FauxStackPointer<=FauxStackBase+(sizeof(int)+sizeof(char)));
}

void minimax_setup(void* boardpos, uint pos_siz, uchar depth, bool minimize,
   int (*evaluator)(void*), bool (*generate)(void*,int,bool),
   bool (*horizon)(void*))
{
   char next_child=0;

   static_evaluator=evaluator;
   generate_position=generate;
   extend_horizon=horizon;
   PositionSize=pos_siz;

   // correspondence of these stack operations to the variables
   // used in minimax_step commented below, even the obvious ones.
   //
   FSTACK_PUSHVAR(next_child);      // next_child
   fstack_create(sizeof(int));      // bestval
   fstack_create(sizeof(char));     // which_child
   FSTACK_PUSHVAR(minimize);        // minimize
   FSTACK_PUSHVAR(depth);           // depth
   fstack_push(boardpos,pos_siz);   // boardpos
   fstack_create(sizeof(char)+sizeof(int));    // return values
}

void minimax_step(void)
{
   uchar boardpos[MAX_POSITION_SIZE];
   uchar copy[MAX_POSITION_SIZE];
   int value, bestval;
   char next_child, which_child;
   uchar depth;
   bool minimize;
   
   FSTACK_POPVAR(value);  // get value from previous recursive call
   fstack_flush(sizeof(which_child));  // flush which child gave best value

   fstack_pop(boardpos, PositionSize);
   FSTACK_POPVAR(depth);
   FSTACK_POPVAR(minimize);
   FSTACK_POPVAR(which_child);
   FSTACK_POPVAR(bestval);
   FSTACK_POPVAR(next_child);

   if(depth==0) {
      if(extend_horizon && extend_horizon(boardpos)) {
         depth=1;
      }
      else {
         value=static_evaluator(boardpos);
         fstack_create(sizeof(which_child));
         FSTACK_PUSHVAR(value);
         return;
      }
   }

   if(next_child<=0) {
      bestval=minimize?INT_MAX:INT_MIN;
   }
   else {
      if((!minimize) ^ (value<bestval)) {  // new value is better than previous
         bestval=value;
         which_child=next_child-1;
      }
   }

#define PRUNE_WIN
#ifdef PRUNE_WIN
   // if have already found a winning move, do not continue
   if(bestval==(minimize?INT_MIN:INT_MAX)) {
      FSTACK_PUSHVAR(which_child);
      FSTACK_PUSHVAR(bestval);
      return;
   }
#endif

   memcpy(copy,boardpos,PositionSize);
   if(!generate_position(copy,next_child,minimize)) {  // no more children to be had
      if(next_child==0) // there were no children at all!
         bestval=static_evaluator(boardpos);
      FSTACK_PUSHVAR(which_child);
      FSTACK_PUSHVAR(bestval);
      return;
   }

   // before setting up for recursion, preserve our local parameters
   next_child++;
   FSTACK_PUSHVAR(next_child);
   FSTACK_PUSHVAR(bestval);
   FSTACK_PUSHVAR(which_child);
   fstack_create(sizeof(minimize));
   FSTACK_PUSHVAR(depth);
   fstack_create(PositionSize);

   // set up for recursive call
   next_child=0; FSTACK_PUSHVAR(next_child);
   FSTACK_PUSHVAR(bestval);
   FSTACK_PUSHVAR(which_child);
   minimize=!minimize; FSTACK_PUSHVAR(minimize);
   depth--; FSTACK_PUSHVAR(depth);
   fstack_push(copy,PositionSize);

   // the actual value we push here is never looked at, but we need to
   // make space for it.
   fstack_create(sizeof(which_child)+sizeof(value));
}

