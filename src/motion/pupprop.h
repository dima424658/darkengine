/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/pupprop.h,v 1.3 2000/01/31 09:49:58 adurant Exp $
#pragma once

#ifndef __PUPPROP_H
#define __PUPPROP_H

#include <property.h>
#include <puppet.h>

////////////////////////////////////////////////////////////
// PUPPET PROPERTY
//

#undef INTERFACE
#define INTERFACE IPuppetProperty
DECLARE_PROPERTY_INTERFACE(IPuppetProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(cPuppet*); 
}; 


#define PROP_PUPPET_NAME "Puppet"

//
// Instantiate the puppet property. 
//
EXTERN IPuppetProperty *PuppetPropertyInit(void);

#endif // __PUPPROP_H




