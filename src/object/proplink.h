/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/proplink.h,v 1.3 2000/01/29 13:24:35 adurant Exp $
#pragma once
#ifndef __PROPLINK_H
#define __PROPLINK_H

#include <property.h>
#include <proptype.h>
#include <linktype.h>


////////////////////////////////////////////////////////////
// LINKED PROPERTY 
// 
// A Property whose value is an objID, that also presents itself as a 
// link store
//
EXTERN IProperty* CreateLinkedProperty(const sPropertyDesc* pdesc, 
                                       const sRelationDesc* rdesc,
                                       const sRelationDataDesc* rddesc,
                                       ILinkQueryDatabase* DestKnown,
                                       ePropertyImpl impl);


EXTERN IProperty* CreateLinkedPropertyFromStore(const sPropertyDesc* pdesc, 
                                       const sRelationDesc* rdesc,
                                       const sRelationDataDesc* rddesc,
                                       ILinkQueryDatabase* DestKnown,
                                       IPropertyStore* store);

#endif // __PROPLINK_H


