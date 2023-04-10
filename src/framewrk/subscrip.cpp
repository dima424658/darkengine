/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/framewrk/subscrip.cpp,v 1.3 1998/10/09 17:17:34 mahk Exp $
//

#include <lg.h>

#include <subscrip.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

cSubscribeService::cSubscribeService(int numBits)
{
   if (numBits > MAX_NUM_BITS)
   {
      Warning(("Too many bits in SubscribeServce!  Reducing to %d\n", MAX_NUM_BITS));
      numBits = MAX_NUM_BITS;
   }

   m_numBits = numBits;
}

////////////////////////////////////////

cSubscribeService::~cSubscribeService()
{
   m_subscriptionTable.DestroyAll();
}

////////////////////////////////////////

void cSubscribeService::Reset()
{
   m_subscriptionTable.DestroyAll();
}

////////////////////////////////////////////////////////////////////////////////

void cSubscribeService::Subscribe(ObjID objID, int bitField)
{
   cSubscription *pSubscription;
   BOOL new_node = FALSE;

   // Look up the node, or create it if necessary
   pSubscription = m_subscriptionTable.Search(&objID);
   if (pSubscription == NULL)
   {
      pSubscription = new cSubscription(objID);
      new_node = TRUE;
   }

   Assert_(pSubscription->objID == objID);

   // Increment each requested bit
   for (int i=0; i<m_numBits; i++)
   {
      if (bitField & (1 << i))
         pSubscription->bitCount[i]++;
   }

   // Need to insert if it's a new node
   if (new_node)
      m_subscriptionTable.Insert(pSubscription);
}

////////////////////////////////////////

void cSubscribeService::Unsubscribe(ObjID objID, int bitField)
{
   cSubscription *pSubscription;
   BOOL old_node = TRUE;

   // Look up node
   pSubscription = m_subscriptionTable.Search(&objID);

   if (pSubscription == NULL)
      Warning(("Attempt to unsubscribe non-subscribed obj %d\n", objID));

   if (!pSubscription)
      return;

   Assert_(pSubscription->objID == objID);

   // Decrement each requested bit
   for (int i=0; i<m_numBits; i++)
   {
      if (bitField & (1 << i))
      {
         Assert_(pSubscription->bitCount[i] > 0);
         pSubscription->bitCount[i]--;
      }

      // Check if any bits are non-zero
      if (pSubscription->bitCount[i] > 0)
         old_node = FALSE;
   }

   // Remove node if all bits are zero
   if (old_node)
   {
      m_subscriptionTable.Remove(pSubscription);
      delete pSubscription;
   }
}

////////////////////////////////////////////////////////////////////////////////

BOOL cSubscribeService::IsSubscribed(ObjID objID, int bitField)
{
   cSubscription *pSubscription;

   // Look up node
   pSubscription = m_subscriptionTable.Search(&objID);
   if (pSubscription == NULL)
      return FALSE;

   // If any of our bits are not subscribed to, return false
   for (int i=0; i<m_numBits; i++)
   {
      if (bitField & (1 << i))
      {
         if (pSubscription->bitCount[i] == 0)
            return FALSE;
      }
   }

   // All requested bits must be subscribed to
   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

void cSubscribeService::Read(ReadWriteFunc read)
{
   cSubscription *pSubscription;
   int counter;
   ObjID objID;

   read(&m_numBits, sizeof(int), 1);
   read(&counter, sizeof(int), 1);

   for (int i=0; i<counter; i++)
   {
      read(&objID, sizeof(ObjID), 1);
      pSubscription = new cSubscription(objID);

      read(pSubscription->bitCount, sizeof(short), MAX_NUM_BITS);

      m_subscriptionTable.Insert(pSubscription);
   }
}

////////////////////////////////////////

void cSubscribeService::Write(ReadWriteFunc write)
{
   cSubscription *pSubscription;
   tHashSetHandle handle;
   int counter;

   write(&m_numBits, sizeof(int), 1);
   
   // Loop through, counting number of items
   counter = 0;
   pSubscription = m_subscriptionTable.GetFirst(handle);
   while (pSubscription != NULL)
   {
      counter++;
      pSubscription = m_subscriptionTable.GetNext(handle);
   }

   // Write count
   write(&counter, sizeof(int), 1);

   // Loop through, actually writing data
   pSubscription = m_subscriptionTable.GetFirst(handle);
   while (pSubscription != NULL)
   {
      write(&pSubscription->objID, sizeof(ObjID), 1);
      write(&pSubscription->bitCount, sizeof(short), MAX_NUM_BITS);

      pSubscription = m_subscriptionTable.GetNext(handle);
   }
}

////////////////////////////////////////////////////////////////////////////////









