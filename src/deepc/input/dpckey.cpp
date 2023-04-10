/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <appagg.h>

#include <iobjsys.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <keyprop.h>
#include <contain.h>

#include <dpckey.h>

#include <playrobj.h>
#include <dpcplcst.h>
#include <dpcplayr.h>

#include <netmsg.h>

// structure descriptor fun
char *access_bit_names[] = { 
   "MED", "SCI", "R&D", "CREW", "CARGO",
   "REC","HYD A","HYD B","HYD D","SHUTTLE", 
   "CRYO", "MED ANNEX", "SECURITY", "BRIDGE", "CARGO2",
   "CREW2", "RICKENBACHER", "RICK ROOM","OPS OVERRIDE",
};

static sFieldDesc keyinfo_fields [] =
{
   { "MasterBit", kFieldTypeBool, FieldLocation(sKeyInfo,master_bit) },
   { "RegionID",  kFieldTypeBits,  FieldLocation(sKeyInfo,region_mask), FullFieldNames(access_bit_names) },
   { "LockID",    kFieldTypeInt,  FieldLocation(sKeyInfo,lock_id),     kFieldFlagUnsigned },
};

static sStructDesc keyinfo_struct = StructDescBuild(sKeyInfo,kStructFlagNone,keyinfo_fields);

////////////////////
//
// NETWORK CODE
//

// The message that gets broadcast whenever a player gets a new key:
static cNetMsg *g_pAddKeyMsg = NULL;

// Some other player has gotten a key; I get it too...
void handleDPCAddKey(uint region_mask)
{
   sKeyInfo *skip;
   AutoAppIPtr(DPCPlayer);
   ObjID keyring = pDPCPlayer->GetEquip(PlayerObject(), kEquipFakeKeys);

   if (!KeySrcProp->IsRelevant(keyring)) {
      Warning(("Object %d doesn't seem to be a key ring!\n", keyring));
      return;
   }
   KeySrcProp->Get(keyring, &skip);
   skip->region_mask |= region_mask;
   KeySrcProp->Set(keyring, skip);
}

static sNetMsgDesc sAddKeyDesc =
{
   kNMF_Broadcast,
   "DPCAddKey",
   "DPC Add Key",
   NULL,
   handleDPCAddKey,
   {{kNMPT_UInt},
    {kNMPT_End}}
};

////////////////////

BOOL DPCKeyContainsListener(eContainsEvent event, ObjID containee, ObjID new_obj, eContainType, ContainCBData)
{
   bool retval = TRUE;
   sKeyInfo *skip1, *skip2;

   // we only care about combining
   if ((event != kContainQueryCombine) && (event != kContainCombine))
      return(TRUE);

   // we only care about keys
   // is this better done after we filter on event?
   if (!KeySrcProp->IsRelevant(containee)  || !KeySrcProp->IsRelevant(new_obj))
      return(TRUE);

   KeySrcProp->Get(containee, &skip1);
   KeySrcProp->Get(new_obj, &skip2);

   switch (event)
   {
   case kContainQueryCombine:
      // only let them merge if their lock_id's are non zero.
      if ((skip1->lock_id != 0) || (skip2->lock_id != 0))
         retval = FALSE;
      break;

   case kContainCombine:
      // just binary OR in the bits of the doomed key
      skip1->region_mask = skip2->region_mask | skip1->region_mask;
      KeySrcProp->Set(containee, skip1);
      g_pAddKeyMsg->Send(OBJ_NULL, skip2->region_mask);
      break;
   }
   return(retval);
}

void DPCKeyInit(void)
{
   AutoAppIPtr(ContainSys);
   StructDescRegister(&keyinfo_struct);

   pContainSys->Listen(OBJ_NULL,DPCKeyContainsListener,NULL);
   g_pAddKeyMsg = new cNetMsg(&sAddKeyDesc);
}

void DPCKeyTerm(void)
{
   delete g_pAddKeyMsg;
}
