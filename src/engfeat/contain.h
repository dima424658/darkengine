/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/contain.h,v 1.15 2000/01/29 13:19:22 adurant Exp $
#pragma once

#ifndef __CONTAIN_H
#define __CONTAIN_H

#include <comtools.h>
#include <objtype.h>
#include <linktype.h>
#include <propface.h>

// containtype is basically an arbitrary sorting field that the app can
// use however it sees fit, if it has sorting-oriented needs
typedef int eContainType;
#define ECONTAIN_NULL (0x7FFFFFFF)

#define IGNORE_TYPES  ECONTAIN_NULL

// eventually add level-change events?
typedef enum eContainsEvent {

   // Query messages get sent before the actual event occurs.
   // If you want to veto the event, this is your chance.  
   kContainQueryAdd,
   kContainQueryCombine, 

   // there is no query remove.  

   // These messages are sent upon COMPLETION of the event in question.  
   // No sense cryin' about it now. 
   kContainAdd,
   kContainRemove,
   kContainCombine, 
   // to force 4 byte compiler independant storage
   kContainWatcomMSVCSilliness=0xffffffff
} eContainsEvent;

#define CTF_NONE        0x0000
#define CTF_COMBINE     0x0001

// user callback data
typedef void* ContainCBData; 
// returns FALSE to mean "DONT ALLOW THIS"
typedef BOOL (*pContainCallback)(eContainsEvent event, ObjID container, ObjID containee, eContainType ctype, ContainCBData data);


// name of the sdesc for the contain relation name
#define CONTAIN_SDESC_NAME "ContainRelType"

///////////////////////////
// Iterators for Containment

typedef struct {
   BOOL         finished;   // is this iter finished
   eContainType type;       // what is my ordinal
   LinkID       link_id;    // linkID of this link
   ObjID        containee;  // ObjID of this object
   short        index;      // this is the index-th element of iter
   // internal to iter rep
   void        *dont_use;   // internal rep data
   eContainType min, max;   // contain type range
} sContainIter;             // 28 bytes

///////////////////////////
// Flags for stack remove
//
enum eStackRemoveFlags
{
   kStackRemoveNoDecrement = 1 << 0, // don't actually decrement the stack count
   kStackRemoveAllowInfinite = 1 << 1, // treat "no stack count" as infinite
   kStackRemoveLeaveEmpty = 1 << 2, // leave a zero stack count behind if needed
}; 

enum eStackIncFlags
{
   kStackAddDestroyIfZero      = 1 << 0,  // if stack count goes to zero, delete 
   kStackAddAllowInfinite      = 1 << 1,  // if no stack count, count as infinite
}; 


////////////////////////////////////////////////////////////
// Contain System Interface
// 
// 

F_DECLARE_INTERFACE(IContainSys); 

#undef INTERFACE
#define INTERFACE IContainSys

DECLARE_INTERFACE_(IContainSys,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //////////////////////////////
   // Callback management

   // adds this callback as a listener to contains changes on the container obj 
   // listen to OBJ_NULL in order to listen on every object always
   STDMETHOD(Listen)(ObjID obj, pContainCallback ContListen, ContainCBData data) PURE;

   // try out the callback.  returns the value returned by the callback
   STDMETHOD_(BOOL,CheckCallback)(ObjID cbobj, eContainsEvent event, ObjID container, ObjID containee, 
      eContainType type) PURE;

   //////////////////////////////
   // Basic Add/Remove sort of stuff

   // the add to container call, if combine false, will not try to
   STDMETHOD(Add)(ObjID container, ObjID containee, eContainType type, uint flags) PURE; 

   // Remove containee from container
   // returns whether successful (ie whether containee actually was in container)
   // note that doesn't care about stack count -- if you want to remove just one of a stack use CombineAdd
   STDMETHOD(Remove)(ObjID container, ObjID containee) PURE;

   // Mutates the containment type for an object
   STDMETHOD(SetContainType)(ObjID container, ObjID containee, eContainType newctype) PURE;

   // is containee in container 
   // returns type of containment or ECONTAIN_NULL if not contained
   STDMETHOD_(eContainType,IsHeld)(ObjID container, ObjID containee) PURE;

   // is containee in container, or in a containee of container, recursively
   STDMETHOD_(BOOL,Contains)(ObjID container, ObjID containee) PURE; 

   // Get an object's container, OBJ_NULL if not contained
   STDMETHOD_(ObjID,GetContainer)(ObjID containee) PURE; 

   // Move all contents 
   STDMETHOD(MoveAllContents)(ObjID targ, ObjID src, int addflags) PURE; 

   //////////////////////////////
   // Combine-related functions

   // for attempting to merge an object only (and going and doing it)
   // will merge new_obj into combinee -- if succesfull, new_obj will cease to exist
   STDMETHOD(CombineTry)(ObjID combinee, ObjID new_obj, int type) PURE;

   // How many items of combine type foo do I contain, in total?
   // ctype is the archetype of the obj you want to check into
   // (maybe should be string name of the CombineType?)
   // does this want a flags option for specifying a contain type value to search within?
   STDMETHOD_(int,CombineCount)(ObjID container, ObjID ctype) PURE;

   // accumulate / deduct to combineable
   // ctype is archetype of combineable obj
   // use negative quantity to deduct
   // returns whether or not it succeeded -- failure cases include nonexistant object of
   // that type (hmm, maybe should just instantiate one in inv? ick) or insufficient amount
   // to remove
   // Flags are "stack" add flags 
   STDMETHOD(CombineAdd)(ObjID container, ObjID ctype, int quantity, ulong flags DEFAULT_TO(kStackAddAllowInfinite)) PURE;

   // Can these two objects be combined?  Checks properties and callbacks
   // Takes no actions, though.
   STDMETHOD_(BOOL, CanCombine)(ObjID combinee, ObjID new_obj, eContainType type) PURE;

   // Scan through the contents of container, and find whether new_obj would
   // hypothetically combine with any of the elements within.
   // Takes no actions.
   STDMETHOD_(BOOL, CanCombineContainer)(ObjID container, ObjID new_obj, eContainType type) PURE;

   //
   // Remove some objects from a stack, possibly making a new object
   // that is a clone 
   //
   STDMETHOD_(ObjID,RemoveFromStack)(THIS_ ObjID stack, ulong flags DEFAULT_TO(0), ulong how_many DEFAULT_TO(1)) PURE; 

   //
   // Add a quantity to a stack, analagous to combine add, but with 
   // no intervening container or combine type.  Returns amount actually 
   // added
   // 
   STDMETHOD_(int,StackAdd)(THIS_ ObjID stack,  int how_many DEFAULT_TO(1), ulong flags DEFAULT_TO(kStackAddAllowInfinite)) PURE; 
   
   // Return the "stack count" property
   STDMETHOD_(IIntProperty*,StackCountProp)(THIS) PURE; 

   ///////////////////////////
   // contain iter functions
   // simple iteration over all elements
   STDMETHOD_(sContainIter *,IterStart)(ObjID container) PURE;
   // setup an iteration from min->max on container
   STDMETHOD_(sContainIter *,IterStartType)(ObjID container, eContainType min_type, eContainType max_type) PURE;
   // iteration over elements including all ancestors
   STDMETHOD_(sContainIter *,IterStartInherits)(ObjID container) PURE;
   // iteration over ancestores from min->max on container
   STDMETHOD_(sContainIter *,IterStartInheritsType)(ObjID container, eContainType min_Type, eContainType max_type) PURE;
   // get next iter element
   STDMETHOD_(BOOL,IterNext)(sContainIter *iter) PURE;
   // finish query process
   STDMETHOD_(void,IterEnd)(sContainIter *iter) PURE;

   // Handle database messages
   STDMETHOD(DatabaseMessage)(ulong msg, IUnknown* file) PURE; 
};

#undef INTERFACE

EXTERN void ContainSysCreate(void); 


#endif  // __CONTAIN_H


