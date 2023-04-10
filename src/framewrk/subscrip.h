/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/framewrk/subscrip.h,v 1.2 2000/01/31 09:48:42 adurant Exp $
//
#pragma once

#ifndef __SUBSCRIP_H
#define __SUBSCRIP_H

#include <hashset.h>
#include <hshsttem.h>

#include <objtype.h>

typedef void (*ReadWriteFunc) (void *buf, size_t elsize, size_t nelem);

////////////////////////////////////////////////////////////////////////////////

#define MAX_NUM_BITS (14)

class cSubscription
{
public:

   cSubscription(ObjID objID_)
   {
      objID = objID_;
      memset(bitCount, 0, sizeof(short) * MAX_NUM_BITS);
   }

   ObjID objID;
   short bitCount[MAX_NUM_BITS];
};

class cSubscriptionTable : public cHashSet<cSubscription *, const ObjID *, cHashFunctions>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)&((cSubscription *)p)->objID;
   }
};

////////////////////////////////////////////////////////////////////////////////

class cSubscribeService
{
public:
   
   // Constructor
   cSubscribeService(int numBits = 1);
   ~cSubscribeService();

   void Reset();

   // Accessors
   void Subscribe(ObjID objID, int bitField = 0x0001);
   void Unsubscribe(ObjID objID, int bitField = 0x0001);

   BOOL IsSubscribed(ObjID objID, int bitField = 0x0001);

   // Save/load
   void Read(ReadWriteFunc read);
   void Write(ReadWriteFunc write);

private:

   int m_numBits;

   cSubscriptionTable m_subscriptionTable;    

};

////////////////////////////////////////////////////////////////////////////////

#endif // __SUBSCRIP_H










