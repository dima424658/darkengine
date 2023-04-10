/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvpr.h,v 1.5 1999/10/20 16:36:39 Justin Exp $
#pragma once  
#ifndef __INVNTPRP_H
#define __INVNTPRP_H

#include <comtools.h>
#include <invtype.h>


F_DECLARE_INTERFACE(IIntProperty); 
F_DECLARE_INTERFACE(IStringProperty); 
F_DECLARE_INTERFACE(IBoolProperty); 

////////////////////////////////////////////////////////////
// "Inventory type" property, specifies whether an object is 
// a weapon, item, or junk 
//


#define PROP_INVENTORY_TYPE_NAME "InvType" 


EXTERN IIntProperty* CreateInvTypeProperty(); 

////////////////////////////////////////////////////////////
// "Game Name" property, used when the object is displayed in the inventory. 
// A string table property, to look it up, use IGameStrings 
// 

#define PROP_GAME_NAME_NAME "GameName" 
#define GAME_NAME_TABLE "objnames" 

EXTERN IStringProperty* CreateGameNameProperty(); 

////////////////////////////////////////////////////////////
// "Description" property, used when the object is described in loadout
// A string table property, to look it up, use IGameStrings 
// 

#define PROP_DESCRIPTION_NAME "GameDesc" 
#define DESCRIPTION_TABLE "objdescs" 

EXTERN IStringProperty* CreateDescriptionProperty(); 


////////////////////////////////////////////////////////////
// "Store" property
// Marks an object as being the "store."  
// The "store" contains the objects that can be purchased before the mission 
// 

#define PROP_STORE_NAME "ItemStore" 
EXTERN IBoolProperty* CreateStoreProperty(); 

////////////////////////////////////////////////////////////
// "Purchase Price" property
// How much does it cost to buy one of these
//
#define PROP_PRICE_NAME "SalePrice"
EXTERN IIntProperty* CreatePriceProperty(); 

////////////////////////////////////////////////////////////
// "Cycle Order" property
// An arbitrary string key, determines sort order for cycling
//
#define PROP_CYCLE_ORDER_NAME "InvCycleOrder"
EXTERN IStringProperty* CreateCycleOrderProperty(); 

////////////////////////////////////////////////////////////
// "Being Taken" property
// This property is actually only used by networking. The host uses it to
// keep track of whether an object has been "claimed" by a player who is
// in the process of picking it up. We use that to prevent anyone else
// from doing the same.
//
#define PROP_BEING_TAKEN_NAME "InvBeingTaken"
EXTERN IBoolProperty* CreateBeingTakenProperty(); 



#endif // __INVNTPRP_H



