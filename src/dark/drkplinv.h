/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkplinv.h,v 1.1 1998/05/22 00:36:20 mahk Exp $
#pragma once  
#ifndef __DRKPLINV_H
#define __DRKPLINV_H

#include <objtype.h>
#include <comtools.h>
#include <invtype.h>

////////////////////////////////////////////////////////////
// DARK PLAYER INVENTORY
//
// This system manages the objects possessed by the single player 
// sitting at the console.  It uses the contains system. 
//


F_DECLARE_INTERFACE(IInventory);
F_DECLARE_INTERFACE(ITagFile); 



#undef INTERFACE
#define INTERFACE IInventory
DECLARE_INTERFACE_(IInventory,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Determine the inventory type of an object
   //
   STDMETHOD_(eInventoryType,GetType)(THIS_ ObjID obj) PURE; 

   //
   // Add an object to inventory.  If the object is junk, selects it and 
   // puts it in hand.  If we already have a junk object, fails. 
   //
   STDMETHOD(Add)(THIS_ ObjID obj) PURE; 

   // 
   // Remove an object from inventory
   //
   STDMETHOD(Remove)(THIS_ ObjID obj) PURE; 

   // 
   // Select an object, making it the current weapon or item. 
   // Fails if the object is not in the inventory, or if 
   // we currently have junk selected
   //
   STDMETHOD(Select)(THIS_ ObjID obj) PURE; 

   // Get current selection
   STDMETHOD_(ObjID, Selection)(THIS_ eWhichInvObj which) PURE; 

   // Clear current selection
   STDMETHOD(ClearSelection)(THIS_ eWhichInvObj which) PURE; 

   // Cycle the current selection, returning the new one
   // If the current selection is a junk object, does not cycle 
   STDMETHOD_(ObjID, CycleSelection)(THIS_ eWhichInvObj which,eCycleDirection dir) PURE; 
   
   //
   // Put one of your selections in hand.  In order to wield something,
   // it must be selected.  This will fail if we try to wield a weapon 
   // while junk is selected. 
   // 
   STDMETHOD(Wield)(THIS_ eWhichInvObj which) PURE; 
   // return currently weilded object
   STDMETHOD_(ObjID,WieldedObj)(THIS) PURE; 

   // 
   // Am I wielding a piece of junk right now.  
   // You'd be surprised how useful this is.
   // 
   STDMETHOD_(BOOL,WieldingJunk)(THIS) PURE; 
   
   //
   // Handle database messages
   //
   STDMETHOD(DatabaseNotify)(THIS_ ulong msg, ITagFile* file) PURE; 

};

#undef INTERFACE

EXTERN void InventoryCreate(void); 


#endif // __DRKPLINV_H






