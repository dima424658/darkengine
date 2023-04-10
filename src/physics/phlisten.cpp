/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phlisten.cpp,v 1.3 1999/06/16 16:48:07 JON Exp $

#include <phlistn_.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////

cPhysListeners::~cPhysListeners()
{
   for (int i=0; i<m_listeners.Size(); i++)
      delete m_listeners[i];
}

////////////////////////////////////////////

PhysListenerHandle cPhysListeners::CreateListener(const char* pName, PhysListenMsgSet set, PhysListenFunc func, PhysListenerData data) 
{
   cPhysListener* pListener = new cPhysListener(pName, m_handle++, set, func, data);
   pListener->ResetSubscriptions();
   m_listeners.Append(pListener);
   return pListener->GetHandle();
}

////////////////////////////////////////////

void cPhysListeners::DestroyListener(PhysListenerHandle handle)
{
   int i;

   if ((i = FindIndex(handle)) != -1)
   {
      cPhysListener* pListener = m_listeners[i];
      m_listeners.DeleteItem(i);
      delete pListener;
   }
}

////////////////////////////////////////////

void cPhysListeners::Listen(PhysListenerHandle handle, ObjID objID)
{
   cPhysListener* pListener = Find(handle);
   if (pListener == NULL)
   {
      Warning(("cPhysListeners::Subscribe - no such listener %d\n", handle));
      return;
   }
   if (!pListener->IsSubscribed(objID, pListener->GetType()))
      pListener->Subscribe(objID);
}

////////////////////////////////////////////
// It's OK to call this function with unsubscribed objects, but it will
// warn if no listener 
//
void cPhysListeners::Unlisten(PhysListenerHandle handle, ObjID objID)
{
   cPhysListener* pListener = Find(handle);
   if (pListener == NULL)
   {
      Warning(("cPhysListeners::Unsubscribe - no such listener %d\n", handle));
      return;
   }
   // @TODO: listeners have type, not subscriptions
   if (pListener->IsSubscribed(objID, pListener->GetType()))
      pListener->Unsubscribe(objID);
}

////////////////////////////////////////////

BOOL cPhysListeners::HasListener(ObjID objID, ePhysScriptMsgType_ type)
{
   for (int i=0; i<m_listeners.Size(); i++)
      if (m_listeners[i]->IsSubscribed(objID, type))
         return TRUE;
   return FALSE;
}

////////////////////////////////////////////

void cPhysListeners::CallCollision(ObjID objID, int submod, ePhysCollisionType type, ObjID coll_object, 
                   int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt)
{
   // @TODO: check if there are any listeners that need this type of msg
   sPhysListenMsg msg(submod, type, coll_object, coll_submod, normal, momentum, pt);
   for (int i=0; i<m_listeners.Size(); i++)
      if (m_listeners[i]->IsSubscribed(objID, kCollisionMsg))
         m_listeners[i]->Call(objID, &msg);
}

////////////////////////////////////////////

void cPhysListeners::Read(PhysReadWrite readFunc)
{
   char *pName;
   int nameLen;
   int size;
   cPhysListener* pListener;
   BOOL *pRead;
   int i;

   readFunc(&size, sizeof(int), 1);

   // setup flags to check that we read all listeners
   pRead = (BOOL*)malloc(size*sizeof(BOOL));
   for (i=0; i<size; i++)
      pRead[i] = FALSE;

   // read all saved listeners
   for (i=0; i<size; i++)
   {
      readFunc(&nameLen, sizeof(int), 1);
      pName = (char*)malloc(nameLen*sizeof(char));
      readFunc((void*)pName, sizeof(char), nameLen);
      if ((pListener = FindByName(pName)) == NULL)
         Warning(("cPhysListeners::Read - Can't find listener %s\n", pName));
      else
      {
         pListener->ReadSubscriptions(readFunc);
         pRead[i] = TRUE;
      }
      free(pName);
   }

   // for any that we didn't read, reset subscriptions
   // this happens if we saved with a version where some listeners weren't registered
   for (i=0; i<size; i++)
   {
      if (!pRead[i])
      {
         Warning(("cPhysListeners:: Read - Didn't read data for listener %s\n", pName));
         m_listeners[i]->ResetSubscriptions();
      }
   }
   free(pRead);
}

////////////////////////////////////////////

void cPhysListeners::Write(PhysReadWrite writeFunc)
{
   int size = m_listeners.Size();
   int nameLen;
   const char *pName;

   writeFunc((void *)&size, sizeof(int), 1);
   for (int i=0; i<m_listeners.Size(); i++)
   {
      pName = m_listeners[i]->GetName();
      nameLen = strlen(pName)+1;
      writeFunc((void*)&nameLen, sizeof(int), 1);
      writeFunc((void*)pName, sizeof(char), nameLen);
      m_listeners[i]->WriteSubscriptions(writeFunc);
   }
}

////////////////////////////////////////////

void cPhysListeners::Reset(void)
{
   for (int i=0; i<m_listeners.Size(); i++)
      m_listeners[i]->ResetSubscriptions();
}

////////////////////////////////////////////

cPhysListener* cPhysListeners::Find(PhysListenerHandle handle)
{
   for (int i=0; i<m_listeners.Size(); i++)
   {
      cPhysListener* pListener = m_listeners[i];
      if (pListener->GetHandle() == handle)
         return pListener;
   }
   return NULL;
}

////////////////////////////////////////////

int cPhysListeners::FindIndex(PhysListenerHandle handle)
{
   for (int i=0; i<m_listeners.Size(); i++)
   {
      cPhysListener* pListener = m_listeners[i];
      if (pListener->GetHandle() == handle)
         return i;
   }
   return -1;
}

////////////////////////////////////////////

cPhysListener* cPhysListeners::FindByName(const char* pName)
{
   for (int i=0; i<m_listeners.Size(); i++)
   {
      cPhysListener* pListener = m_listeners[i];
      if (!strcmp(pListener->GetName(), pName))
         return pListener;
   }
   return NULL;
}

