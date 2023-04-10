/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkid.h,v 1.1 1998/01/27 18:55:17 mahk Exp $
#pragma once  
#ifndef __LINKID_H
#define __LINKID_H

// Maker/breakers
#define LINK_FLAVOR_SHF 20
#define LINK_FLAVOR_MASK 0xFFF
#define LINK_PARTITION_SHF 16
#define LINK_PARTITION_MASK 0xF
#define LINK_IDX_MASK 0xFFFF
#define LINK_IDX_SHF  0

#define LINKID_PARTITION(id) (((id) >> LINK_PARTITION_SHF) & LINK_PARTITION_MASK)

#define LINK_FLAVOR_BITS (LINK_FLAVOR_MASK << LINK_FLAVOR_SHF)
#define LINK_IDX_BITS (LINK_IDX_MASK << LINK_IDX_SHF)

// Here we want to preserve the sign of the value.  Use / instead of >>.  
#define LINKID_RELATION(id)  (RelationID)((long)((id) & LINK_FLAVOR_BITS) / (1 << LINK_FLAVOR_SHF))
#define LINKID_IDX(id) ((short)((id) & LINK_IDX_BITS) / (1 << LINK_IDX_SHF))

#define LINKID_NON_RELATION(id) ((id) & ~LINK_FLAVOR_BITS)

#define LINKID_MAKE(flav,part,idx)  \
         (LinkID)( \
                     (((flav) & LINK_FLAVOR_MASK) << LINK_FLAVOR_SHF)         \
                   | (((part) & LINK_PARTITION_MASK) << LINK_PARTITION_SHF)   \
                   | (((idx) & LINK_IDX_MASK) << LINK_IDX_SHF)                \
                 )

#define LINKID_MAKE2(flav,nonflav)  \
         (LinkID)( \
                     (((flav) & LINK_FLAVOR_MASK) << LINK_FLAVOR_SHF)  \
                     | ((nonflav) & ~(LINK_FLAVOR_MASK << LINK_FLAVOR_SHF)) \
         )

// construct the ID of the "inverse link"
#define LINKID_INVERT(id) LINKID_MAKE2(RELID_INVERT(LINKID_RELATION(id)),LINKID_NON_RELATION(id))
#define LINKID_IS_INVERTED(id) RELID_IS_INVERTED(LINKID_RELATION(id))

#endif // __LINKID_H
