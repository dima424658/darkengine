/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schmsg.cpp,v 1.6 1998/10/05 17:27:45 mahk Exp $

#include <lgassert.h>
extern "C"
{
#include <matrix.h>
}

#include <appsfx.h>
#include <schmsg.h>
#if AI_TODO
//include <aipool.h>
//include <aiext.h>
#endif
#include <schbase.h>


// Must be last header 
#include <dbmem.h>


// ignore all messages at volume less than this (at recipient)
// we probably want to decrease this
#define SCHEMA_MSG_MIN_VOLUME -5000

void SchemaMsgPropagate(sSchemaMsg *pMsg)
{
#if AI_TODO
   cAI *pAI;
   int volume;
   float dist;
   mxs_vector aiLoc;

   Assert_(pMsg != NULL);

   if (g_pAIPool->GetFirstAI(pAI))
   {
      do
      {
         AIGetAILocation(pAI, &aiLoc);
         dist = mx_dist_vec(&aiLoc, (mxs_vector*)(&pMsg->sourceLoc));
         if ((volume = SFX_Attenuate(pMsg->volume, dist))>SCHEMA_MSG_MIN_VOLUME)
            AINotifyAboutSound(pAI, pMsg, volume);
      }
      while (g_pAIPool->GetNextAI(pAI));
   }
#endif
}




