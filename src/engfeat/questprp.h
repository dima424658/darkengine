/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/questprp.h,v 1.2 1999/06/02 16:55:28 XEMU Exp $
#pragma once  
#ifndef __QUESTPRP_H
#define __QUESTPRP_H

#include <property.h>
#include <propdef.h>
////////////////////////////////////////////////////////////
// QUEST-VARIABLE FILTER PROPERTY 
//
// Each instance of this property is tied to a quest variable. 
// Its value is a bitmask of which quest variable values (0-31) it 
// "matches."  There are tools for iterating over all matching objects,
// or all unmatching objects.
//

F_DECLARE_INTERFACE(IQuestVarProperty); 
F_DECLARE_INTERFACE(IObjectQuery); 
typedef int tQVarVal; 

enum eQVarMatch
{
   kQVarNonMatches,
   kQVarMatches,

   kQVarLargeDummyVal = 0xFFFFFFFF
}; 
typedef enum eQVarMatch eQVarMatch; 

#undef INTERFACE 
#define INTERFACE IQuestVarProperty
DECLARE_PROPERTY_INTERFACE(IQuestVarProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(tQVarVal); 

   //
   // Return TRUE iff the object's value matches the qvar
   // Defaults to FALSE in the case where the property isn't relevant,
   // or if the Quest Var isn't defined 
   //
   STDMETHOD_(BOOL,Matches)(THIS_ ObjID obj) PURE; 

   // just like the normal Matches function, but lets you provide
   // a hypothetical qval
   STDMETHOD_(BOOL,Matches)(THIS_ ObjID obj, int val) PURE; 

   //
   // Return a query of all objects that match or don't match 
   //
   STDMETHOD_(IObjectQuery*,QueryAllMatches)(THIS_ eQVarMatch match) PURE; 


}; 

//
// Factory
//
EXTERN IQuestVarProperty* CreateQuestVarProperty(const char* qvar, const sPropertyDesc* desc, ePropertyImpl impl); 




#endif // __QUESTPRP_H





