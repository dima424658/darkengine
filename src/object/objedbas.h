/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objedbas.h,v 1.3 1997/12/04 15:03:29 mahk Exp $
#pragma once  
#ifndef __OBJEDBAS_H
#define __OBJEDBAS_H

#include <objedtyp.h>
//------------------------------------------------------------
// OBJECT EDITING BASE TYPES
//

//
// Editor Flags
// 

enum eObjEditorFlags
{
   kObjEdShowAll     = 1 << 0,   // Show all traits, including invisible ones

};

//
// Editor descriptor structure
//

struct sObjEditorDesc 
{
   char title[32]; // window title
   ulong flags;
};

//
// Editor event kind
//

enum eObjEditEvent_
{
   kObjEdDone,
   kObjEdCancel,
};

typedef ulong eObjEditEvent;

//
// Event structure
//

struct sObjEditorEvent
{
   eObjEditEvent kind; 
   IObjEditor* editor;    
};

//
// Trait iterator
//
struct sEditTraitIter 
{
   int idx; 
}; 


#endif // __OBJEDBAS_H



