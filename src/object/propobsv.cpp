/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/object/propobsv.cpp,v 1.3 1998/10/06 15:04:36 TOML Exp $
//
//
//

#include <propbase.h>
#include <propobsv.h>

// Must be last header 
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPropertyObserver
//

void cPropertyObserver::Init(IProperty * pProperty, unsigned flags)
{
   if (flags & kPOF_DirtyRebuildConcrete)
      pProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad | kListenPropRebuildConcrete, 
                        PropListener, 
                        (PropListenerData)this);
   else
      pProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad, 
                        PropListener, 
                        (PropListenerData)this);
}

///////////////////////////////////////

void LGAPI cPropertyObserver::PropListener(sPropertyListenMsg* msg, PropListenerData data)
{
   ((cPropertyObserver *)data)->m_touched = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
