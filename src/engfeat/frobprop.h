/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/frobprop.h,v 1.8 2000/01/29 13:19:35 adurant Exp $
#pragma once

#ifndef __FROBPROP_H
#define __FROBPROP_H

#include <property.h>
#include <propface.h> // For IBoolProperty
#include <objtype.h>
#include <frobscrt.h>
#include <relation.h>

//////////

#define PROP_FROB_INFO_NAME "FrobInfo"

#undef INTERFACE
#define INTERFACE IFrobInfoProperty
DECLARE_PROPERTY_INTERFACE(IFrobInfoProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sFrobInfo*); 
}; 


// info prop, for use by the frobctrl stuff
EXTERN IFrobInfoProperty *pFrobInfoProp;

//////////
//
// Frob Handler Property
//
// The "frob handler" for an object is the machine that interprets frobs
// on that object. There are three possible settings:
//
// -- kFrobHandlerHost (default): the frob will be dealt with by the host
//    that owns the object.
// -- kFrobHandlerLocal: the frob will happen only on the machine of the
//    player who frobbed. Usually appropriate when the frob will just
//    show something, without causing any actual world effects.
// -- kFrobHandlerGlobal: the frob will happen separately on each machine.
//    Usually appropriate when the frob's only effect is to activate
//    something that has to be local.
//
enum eFrobHandlerEnum {
   kFrobHandlerHost,
   kFrobHandlerLocal,
   kFrobHandlerGlobal
};
typedef int eFrobHandler;
#define PROP_FROB_HANDLER_NAME "FrobHandler"

EXTERN eFrobHandler FrobHandler(ObjID obj);
EXTERN void         SetFrobHandler(ObjID obj, eFrobHandler handler);

//////////

// frobresult data
EXTERN sFrobInfo *ObjGetFrobResult(ObjID obj);
EXTERN uchar      ObjFrobResultForLoc(eFrobLoc loc, ObjID obj);

// init/term fun
EXTERN BOOL       FrobPropsInit(void);
EXTERN void       FrobPropsTerm(void);

// for frob proxy creation/edit
EXTERN IRelation         *pFrobProxyRel;

#define FROB_PROXY_DOINV     (1<<0)
#define FROB_PROXY_NOWORLD   (1<<1)
#define FROB_PROXY_DOTOOLSRC (1<<2)
#define FROB_PROXY_NOTOOLDST (1<<3)
#define FROB_PROXY_ALLOW     (1<<4)

#endif // __FROBPROP_H

