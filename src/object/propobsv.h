/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/propobsv.h,v 1.2 1998/10/06 15:04:30 TOML Exp $
//
//
//

#include <property.h>

#ifndef __PROPOBSV_H
#define __PROPOBSV_H

#pragma once

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPropertyObserver
//
// Special case property listener used only to indicate a property has
// changed since some previous time.
//
// Until properties allow unlistening, all instances of this class
// must be static.
//

enum ePropObsvFlags
{
   kPOF_DirtyRebuildConcrete = 0x01 // mark dirty on any rebuild concrete
};

///////////////////

class cPropertyObserver
{
public:
   cPropertyObserver();

   // Begin observing a property
   void Init(IProperty *, unsigned flags = 0);

   // Check if any object has acquired or lost the property
   BOOL Touched();
   
   // Clear the "touched" flag
   void SetClean();

private:
   BOOL m_touched;
   static void LGAPI PropListener(sPropertyListenMsg* msg, PropListenerData data);
   
};

///////////////////////////////////////

inline cPropertyObserver::cPropertyObserver()
{
   m_touched = TRUE; // Safest to assume dirty
}

///////////////////////////////////////

inline BOOL cPropertyObserver::Touched()
{
   return m_touched;
}

///////////////////////////////////////

inline void cPropertyObserver::SetClean()
{
   m_touched = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PROPOBSV_H */
