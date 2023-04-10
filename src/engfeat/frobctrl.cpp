/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/frobctrl.cpp,v 1.42 2000/01/06 13:57:13 BFarquha Exp $
// frob control system

#include <lg.h>
#include <mprintf.h>
#include <cfgdbg.h>  // For ConfigSpew

#include <appagg.h>
#include <lazyagg.h>

// frob property stuff
#include <frobprop.h>
#include <frobscrt.h>
#include <frobscrm.h>
#include <frobctrl.h>

// for frob propagator
#include <contag8r.h>
#include <ctactbas.h>

#include <esnd.h>

// for ammo
#include <appagg.h>
#include <contain.h>

// for the frobproxy horror
#include <lnkquery.h>
#include <linkbase.h>

// for script service/persistance
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <iobjsys.h>
#include <objdef.h>
#include <objedit.h>

#include <iobjnet.h>
#include <netmsg.h>

// #include <initguid.h>

#include <playrobj.h> // testing

// must be last header
#include <dbmem.h>

IMPLEMENT_SCRMSG_PERSISTENT(sFrobMsg)
{
   PersistenceHeader(sScrMsg,kFrobMsgVer);
   Persistent(SrcObjId);
   Persistent(DstObjId);
   Persistent(Frobber);
   PersistentEnum(SrcLoc);
   PersistentEnum(DstLoc);
   Persistent(Sec);
   Persistent(Abort);
   return TRUE;
}

//  kFrobLocNone, kFrobLocInv, kFrobLocWorld
static char *loc_names[]={"World","Inv","Tool"};
//  kFrobLevFocus, kFrobLevSelect
static char *lev_names[]={"Focus","Select"};

static void hostFrobExecute(sFrobActivate *situation, eFrobLoc type);

//////////
//
// NETWORK CODE
//

// We have been instructed to frob something. Remarshal the arguments,
// and go do it.
static void handleFrobRequest(int ms_down, ObjID src_obj, ObjID dst_obj,
                              uchar src_loc, uchar dst_loc, ObjID frobber,
                              uchar flags)
{
   sFrobActivate situation;
   eFrobLoc type;

   // Transcribe the message into the situation, and keep going:
   situation.ms_down = ms_down;
   situation.src_obj = src_obj;
   situation.dst_obj = dst_obj;
   situation.src_loc = (eFrobLoc) src_loc;
   situation.dst_loc = (eFrobLoc) dst_loc;
   situation.frobber = frobber;
   situation.flags = flags;

   if (situation.dst_obj!=OBJ_NULL)
      type=kFrobLocTool;           // hey, im a tool
   else
      type=situation.src_loc;     // am i world or inv

   hostFrobExecute(&situation, type);
}

//
// Normal case: a message to the object's owner, asking them to frob
// this thing.
//
static cNetMsg *g_pFrobRequestMsg = NULL;

static sNetMsgDesc sFrobRequestDesc =
{
   kNMF_SendToObjOwner,
   "FrobReq",
   "Frob Request",
   NULL,
   handleFrobRequest,
   {{kNMPT_Int, kNMPF_None, "MS Down"},
    {kNMPT_ReceiverObjID, kNMPF_AllowNonExistentObj, "Source Obj"},
    {kNMPT_GlobalObjID, kNMPF_None, "Dest Obj"},
    {kNMPT_UByte, kNMPF_None, "Src Loc"},
    {kNMPT_UByte, kNMPF_None, "Dest Loc"},
    // We allow the sender to be local-only, and put some checks on
    // that ourselves:
    {kNMPT_SenderObjID, kNMPF_NoAssertHostedHere, "Frobber"},
    {kNMPT_UByte, kNMPF_None, "Flags"},
    {kNMPT_End}}
};

//
// For the unusual case where we want each machine to frob something
// individually. Rarely used, but occasionally useful.
//
static cNetMsg *g_pGlobalFrobMsg = NULL;

static sNetMsgDesc sGlobalFrobDesc =
{
   kNMF_Broadcast,
   "GlobalFrob",
   "Global Frob",
   NULL,
   handleFrobRequest,
   {{kNMPT_Int, kNMPF_None, "MS Down"},
    {kNMPT_ReceiverObjID, kNMPF_None, "Source Obj"},
    {kNMPT_GlobalObjID, kNMPF_None, "Dest Obj"},
    {kNMPT_UByte, kNMPF_None, "Src Loc"},
    {kNMPT_UByte, kNMPF_None, "Dest Loc"},
    {kNMPT_SenderObjID, kNMPF_None, "Frobber"},
    {kNMPT_UByte, kNMPF_None, "Flags"},
    {kNMPT_End}}
};

//
// For objects that have requested a return receipt
//
static cNetMsg *g_pFrobAcknowledgeMsg = NULL;

static void handleFrobAcknowledge(ObjID o)
{
   frobAcknowledge(o);
}

static sNetMsgDesc sFrobAcknowledgeDesc =
{
   kNMF_None,
   "FrobAck",
   "Frob Acknowledge",
   NULL,
   handleFrobAcknowledge,
   // We can't assert here, because the object might not exist by the
   // time we need to acknowledge it:
   {{kNMPT_SenderObjID, kNMPF_NoAssertHostedHere | kNMPF_AllowNonExistentObj, "Object"},
    {kNMPT_End}}
};

//////////

// actually do the send...
BOOL SendFrobMsg(sFrobActivate *info, char *base_name, BOOL abort)
{
   char buf[120];
   strcpy(buf,"Frob");
   strcat(buf,base_name);
   strcat(buf,info->ms_down?"End":"Begin");
//   mprintf("Sending %s(%s) to %x %x (tm %d)\n",buf,abort?"Abort":"",info->src_obj,info->dst_obj,info->ms_down);
   AutoAppIPtr(ScriptMan);
   sFrobMsg frobmsg(info->src_obj,info->dst_obj,info->frobber,info->src_loc,info->dst_loc,info->ms_down,buf,abort);
   cMultiParm result = 1;
   // If we've gotten this far, allow scripts to run locally:
   frobmsg.flags |= kSMF_MsgSendToProxy;
   pScriptMan->SendMessage(&frobmsg,&result);
   return (BOOL)result;
}

//////////////
// @BUG: you currently cant save during a Frob! - due to auto remember
// maybe we cant punt that...

static sFrobActivate last_frob_begin;
static eFrobLoc      last_frob_loc=kFrobLocNone;

// need a stored frob for each type, really
// ie. one for world and one for inv, at least
// but hey, later, i guess

// cancel the started frob! - NULL will cancel if there is a current one
void FrobAbort(eFrobLoc loc, sFrobActivate *abort_situation)
{
   sFrobInfo *info_ptr;
   eFrobLoc type;
   if (abort_situation==NULL)
      if ((last_frob_loc!=kFrobLocNone)&&((loc==kFrobLocNone)||(loc==last_frob_loc)))
         abort_situation=&last_frob_begin;
   if (abort_situation==NULL)
   {
//      Warning(("No frob to abort loc %d...",last_frob_loc));
      return;
   }
   AssertMsg(pFrobInfoProp,"Cannot Frob w/o FrobInfoProp");
   if (!pFrobInfoProp->Get(abort_situation->src_obj,&info_ptr))
   {
      Warning(("%s has no FrobInfo prop, cant be frobbed\n",ObjWarnName(abort_situation->src_obj)));
      return;
   }
   if (abort_situation->dst_obj!=OBJ_NULL)
      type=kFrobLocTool;           // hey, im a tool
   else
      type=abort_situation->src_loc;     // am i world or inv
   if (info_ptr->actions[type]&kFrobIgnore) return;
   if ((info_ptr->actions[type]&kFrobScript)==0) return;
   SendFrobMsg(abort_situation,loc_names[type],TRUE); // send the script messages
   last_frob_loc=kFrobLocNone;
}

// get the useful value of a prox element
static BOOL prox_bit_eval(int *data, int mask, BOOL bias)
{
   if (data==NULL) return bias;
   else if (bias)  return ((*data)&mask)==0;
   else            return ((*data)&mask)!=0;
}

static BOOL get_prox_info(int *data, eFrobLoc loc, BOOL im_the_src)
{
   if (loc==kFrobLocTool)
      if (im_the_src)           return prox_bit_eval(data,FROB_PROXY_DOTOOLSRC,FALSE);
      else                      return prox_bit_eval(data,FROB_PROXY_NOTOOLDST,TRUE);
   else if (loc==kFrobLocWorld) return prox_bit_eval(data,FROB_PROXY_NOWORLD,TRUE);
   else                         return prox_bit_eval(data,FROB_PROXY_DOINV,FALSE);
}

// go resolve any frobproxy links - return FALSE to abort the frob
static BOOL resolveFrobProxy(ObjID *objp, eFrobLoc loc, BOOL im_the_src)
{
   ILinkQuery *prox_query;
   ObjID new_obj=OBJ_NULL;
   sLink link;
   int cnt=0;

   // if i am the source of proxy links, change me to that object
   prox_query=pFrobProxyRel->Query(*objp,LINKOBJ_WILDCARD);
   for (; !prox_query->Done(); prox_query->Next())
      if (new_obj==OBJ_NULL)
      {
         if (get_prox_info((int *)prox_query->Data(),loc,im_the_src))
         {
            prox_query->Link(&link);
            new_obj=link.dest;
         }
      }
      else
         Warning(("We dont support multiple valid FrobProxy Link Destinations\n"));
   SafeRelease(prox_query);
   if (new_obj)
   {
      *objp=new_obj;
      return TRUE;
   }
   // if i am the target of proxy links, if !allow, dont allow me to frob
   prox_query=pFrobProxyRel->Query(LINKOBJ_WILDCARD,*objp);
   for (; !prox_query->Done(); prox_query->Next())
   {
      int *data=(int *)prox_query->Data();
      if ((data!=NULL)&&((*data)&FROB_PROXY_ALLOW))
         continue;
      if (!get_prox_info(data,loc,im_the_src))
         continue;    // this proxies to us for this, so we cant be directly frobbed!
      SafeRelease(prox_query);
      return FALSE;
   }
   SafeRelease(prox_query);
   return TRUE;
}

// installable inventory function pointers
EXTERN BOOL (*frobInvRemObj)(ObjID o) = NULL;
EXTERN BOOL (*frobInvTakeObjFromWorld)(ObjID o, ObjID owner) = NULL;
EXTERN BOOL (*frobInvThrowBackToWorld)(ObjID o, ObjID src) = NULL;
EXTERN BOOL (*frobInvToolCursor)(ObjID o) = NULL;
EXTERN BOOL (*frobOnSetCurrent)(eFrobLoc loc, eFrobLev lev, ObjID newobj, ObjID oldobj) = NULL;
EXTERN BOOL (*frobInvDeselect)(eFrobLoc loc, ObjID obj, ObjID frobber) = NULL;
EXTERN void (*frobAcknowledge)(ObjID o) = NULL;

void frob_propagate(sFrobActivate* frob, BOOL reverse);

static LazyAggMember(IObjectSystem) gpObjSys;
static LazyAggMember(IContainSys)    gpContainSys;

// Returns TRUE iff this frob will try to pick up the target object. It
// is essential that, if this returns TRUE, FrobInstantExecute will be
// sure to begin the actual frob transaction, not just return.
BOOL FrobWouldPickup(sFrobActivate *situation)
{
   sFrobInfo *info_ptr;

   if ((situation->dst_obj != OBJ_NULL) ||
       (situation->src_obj == OBJ_NULL) ||
       (situation->src_loc == kFrobLocTool))
   {
      return FALSE;
   }

   eFrobLoc type = situation->src_loc;
   if (type != kFrobLocWorld)
      return FALSE;

   ObjID src_o = situation->src_obj;
   if (!resolveFrobProxy(&src_o, type, TRUE))
      return FALSE;
   if (!pFrobInfoProp->Get(src_o, &info_ptr))
   {
      return FALSE;
   }
   int action=info_ptr->actions[type];
   if (action & kFrobMove)
      // Okay, it's a single-object frob in the world, and the
      // appropriate action is to take it:
      return TRUE;

   return FALSE;
}

// Tell the owner of the target object that it is being frobbed:
void FrobExecute(sFrobActivate *situation)
{
   ObjID src_o=situation->src_obj;
   ObjID dst_o=situation->dst_obj;
   eFrobLoc type;

   AssertMsg(pFrobInfoProp,"Cannot Frob w/o FrobInfoProp");
   if (dst_o!=OBJ_NULL)
      type=kFrobLocTool;           // hey, im a tool
   else
      type=situation->src_loc;     // am i world or inv
   if ((src_o!=OBJ_NULL)&&(!resolveFrobProxy(&src_o,type,TRUE)))
      return;
   if ((dst_o!=OBJ_NULL)&&(!resolveFrobProxy(&dst_o,type,FALSE)))
      return;
   situation->src_obj=src_o;  // @TODO: make sure we move the correct objIDs around
   situation->dst_obj=dst_o;  // for now, situation can be modified, somewhat scary, maybe wrong!
#ifdef NEW_NETWORK_ENABLED
   // There are several possible cases we have to deal with, depending on
   // who is in charge of handling this frob:
   // -- kFrobHandlerLocal: just do it here
   // -- kFrobHandlerHost: let the host deal with it
   // -- kFrobGlobally: send it to everyone else, *and* do it here
   eFrobHandler handler = FrobHandler(src_o);
   // Check for the rare but nasty case of a local-only object (such as
   // a Shock apparition) trying to frob a "real" object. In this case,
   // we only allow it to happen if the frobbee is owned here. The
   // rationale for this is that these things are probably happening
   // more or less in parallel, and we shouldn't be triggering the
   // real effect every time something local-only happens...
   AutoAppIPtr(ObjectNetworking);
   // Since we aren't doing the automatic assertion in cNetMsg on the
   // frobber, do a more-appropriate one here:
   AssertMsg1(!pObjectNetworking->ObjIsProxy(situation->frobber),
              "Proxy object %d trying to send a frob message!",
              situation->frobber);
   if (pObjectNetworking->ObjLocalOnly(situation->frobber) &&
       !pObjectNetworking->ObjLocalOnly(src_o))
   {
      if (!pObjectNetworking->ObjHostedHere(src_o))
         // None of our business messing with this object...
         return;
      else
      {
         // Okay, so we now know that the target is hosted here. Send
         // the request directly, instead of through networking, which
         // will have heartburn at the local-only frobber. We don't
         // currently allow global frobs from local-only objects, since
         // that's combining two somewhat questionable activities into
         // one horribly strange one.
         AssertMsg1(handler != kFrobHandlerGlobal,
                    "Local-only object %d trying a global frob!",
                    situation->frobber);
         hostFrobExecute(situation, type);
      }
   } else if (handler == kFrobHandlerHost) {
      // Usual case -- just send to the owning host
      // This goes through handleFrobRequest, above, and thence to
      // hostFrobExecute.
      g_pFrobRequestMsg->Send(OBJ_NULL,
                              situation->ms_down,
                              situation->src_obj,
                              dst_o,
                              (uchar) situation->src_loc,
                              (uchar) situation->dst_loc,
                              situation->frobber,
                              (uchar) situation->flags);
   } else if (handler == kFrobHandlerGlobal) {
      // handler == kFrobHandlerGlobal
      // send it to everyone
      // This goes through handleFrobRequest, above, and thence to
      // hostFrobExecute.
      g_pGlobalFrobMsg->Send(OBJ_NULL,
                             situation->ms_down,
                             situation->src_obj,
                             dst_o,
                             (uchar) situation->src_loc,
                             (uchar) situation->dst_loc,
                             situation->frobber,
                             (uchar) situation->flags);
   } else {
      AssertMsg(handler == kFrobHandlerLocal,
                "Unknown Frob handler type!");

      // We need to deal with the frob ourselves
      hostFrobExecute(situation, type);
   }
#else
   hostFrobExecute(situation, type);
#endif
}

// Send a return receipt, if one was requested. This runs on the host.
static void returnReceipt(sFrobActivate *situation)
{
   if (frobAcknowledge == NULL)
      // This app isn't dealing with acknowledgements...
      return;

   if (situation->flags & FROBFLG_RETURN_RECEIPT)
   {
#ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(ObjectNetworking);
      ObjID receiver = pObjectNetworking->ObjHostPlayer(situation->frobber);
      if (receiver == OBJ_NULL)
         receiver = PlayerObject();
      g_pFrobAcknowledgeMsg->Send(receiver, situation->src_obj);
#else
      frobAcknowledge(situation->src_obj);
#endif
   }
}

// The portion of the frob process that gets executed on the host that
// owns src_o, instead of the client that started the frob.
static void hostFrobExecute(sFrobActivate *situation, eFrobLoc type)
{
   sFrobInfo *info_ptr;
   int action;
   ObjID src_o=situation->src_obj;

   if (!gpObjSys->Exists(src_o))
   {
      // Probably some sort of network raceway condition
      returnReceipt(situation);
      return;
   }

   if (!pFrobInfoProp->Get(src_o,&info_ptr))
   {
      Warning(("%s has no FrobInfo prop, how can you be frobbed\n",ObjWarnName(src_o)));
      returnReceipt(situation);
      return;
   }
   action=info_ptr->actions[type];
   if (action & kFrobIgnore)
   {
      returnReceipt(situation);
      return;
   }

   gpObjSys->Lock();

   BOOL success = TRUE;
   if (action & kFrobScript)
      success = SendFrobMsg(situation,loc_names[type],FALSE); // send the script messages
   if (situation->ms_down==0)
   {  // store it off for an abort...
      last_frob_begin=*situation;
      last_frob_loc=type;
      goto out;     // delete and move happen only on End
   }
   else
      last_frob_loc=kFrobLocNone;  // let us know we have nothing pending

   if (success)
   {

      // Do act/react propagation, in each direction
      frob_propagate(situation,FALSE);  // forward
      frob_propagate(situation,TRUE);  // reverse

      if (action & kFrobDelete)
      {
         gpObjSys->Destroy(src_o);
         goto destroyed;
      }

      if ((action & kFrobMove) && (type!=kFrobLocTool))
      {
         ObjID moveme = src_o;

         // If this is an ammo kinda thing, make a clone to ove
         if (action & kFrobUseAmmo)
            moveme = gpContainSys->RemoveFromStack(src_o,kStackRemoveAllowInfinite);

         if (type==kFrobLocWorld)
            frobInvTakeObjFromWorld(moveme,situation->frobber);
         else // for now, always throw back
            frobInvThrowBackToWorld(moveme,situation->frobber);
      }

      if (action & kFrobToolCursor)
      {
         // if game has defined tool cursor usage, call it
         if (frobInvToolCursor != NULL)
            frobInvToolCursor(src_o);
      }


      if ((action & kFrobUseAmmo) && !(action & (kFrobDelete|kFrobMove)))
      {
         cAutoIPtr<IIntProperty> pCount = gpContainSys->StackCountProp();
         int cnt = 0;
         if (pCount->Get(src_o, &cnt))
         {
            cnt--;
            pCount->Set(src_o,cnt);
            if (cnt <= 0)
            {
               gpObjSys->Destroy(src_o);
               goto destroyed;
            }
         }
      }


      if (action & kFrobDeselect)
      {
         if (frobInvDeselect)
            frobInvDeselect(situation->src_loc, src_o, situation->frobber);
      }
   }

destroyed:
out:

   gpObjSys->Unlock();
   returnReceipt(situation);

}

void FrobInstantExecute(sFrobActivate *situation)
{
   int ms=situation->ms_down;
   situation->ms_down=0;
   FrobExecute(situation);
   situation->ms_down=ms;
   FrobExecute(situation);
}

#define FROB_PAIR(x,y) (((ulong)(x) << 16) | (y))

static void frob_propagate(sFrobActivate* frob, BOOL reverse)
{
   ulong mask = 0;
   if (reverse && frob->dst_obj == OBJ_NULL)
      return;

   ObjID src = (!reverse) ? frob->src_obj : frob->dst_obj;
   ObjID dst = (!reverse) ? frob->dst_obj : frob->src_obj;
   eFrobLoc src_loc = (!reverse) ? frob->src_loc : frob->dst_loc;
   eFrobLoc dst_loc = (!reverse) ? frob->dst_loc : frob->src_loc;

   if (dst == OBJ_NULL)
   {
      dst = frob->frobber;
      mask = (src_loc == kFrobLocInv) ? kPropagateInInv : kPropagateInWorld;
   }
   // this is hateful, but hard to avoid
   else switch (FROB_PAIR(src_loc,dst_loc))
   {
      case FROB_PAIR(kFrobLocInv,kFrobLocWorld) :  mask = kPropagateInv2World;   break;
      case FROB_PAIR(kFrobLocInv,kFrobLocInv)   :  mask = kPropagateInv2Inv;     break;
      case FROB_PAIR(kFrobLocWorld,kFrobLocInv)   :  mask = kPropagateWorld2Inv;     break;
      case FROB_PAIR(kFrobLocWorld,kFrobLocWorld)   :  mask = kPropagateWorld2World;     break;
   }

   cContactPropagator* pGator = cContactPropagator::gpGator;
   pGator->DoPropagationEvent(mask,src,dst,frob->ms_down*1.0);

}

/////////////
// these are to be used by the app when it builds the FrobExecutes...

// selected Objs are the current selection
ObjID frobWorldSelectObj=OBJ_NULL;
ObjID frobInvSelectObj  =OBJ_NULL;

// focused objects are for things like cursors and such
ObjID frobWorldFocusObj =OBJ_NULL;
ObjID frobInvFocusObj   =OBJ_NULL;

void FrobSetCurrent(eFrobLoc loc, eFrobLev lev, ObjID obj)
{
   ObjID *objp, old_obj;
   sFrobInfo *info_ptr;
   char buf[120], *p;  // for script messages
   int action_de=0, action_on=0;

   AssertMsg(pFrobInfoProp,"Cannot SetFrob w/o FrobInfoProp");
   if (loc==kFrobLocWorld)
      if (lev==kFrobLevFocus) objp=&frobWorldFocusObj; else objp=&frobWorldSelectObj;
   else
      if (lev==kFrobLevFocus) objp=&frobInvFocusObj; else objp=&frobInvSelectObj;
   old_obj=*objp;

   if (obj == old_obj)
      return;

   if (frobOnSetCurrent)
      frobOnSetCurrent(loc,lev,obj,old_obj);

   if (old_obj!=OBJ_NULL)
   {
      if (pFrobInfoProp->Get(old_obj,&info_ptr))
         action_de=info_ptr->actions[loc];
   }
   if (obj!=OBJ_NULL)
   {
      if (pFrobInfoProp->Get(obj,&info_ptr))
         action_on=info_ptr->actions[loc];
   }
   if ((action_on & kFrobFocusScript)||(action_de & kFrobFocusScript))
   {
      AutoAppIPtr_(ScriptMan,pScriptMan);
      strcpy(buf,loc_names[loc]);
      p=buf+strlen(buf);
      if (action_de & kFrobFocusScript)
      {
         *p='\0';
         strcat(p,"De");
         strcat(p,lev_names[lev]);
         sScrMsg focusmsg(old_obj,buf);
         pScriptMan->SendMessage(&focusmsg);
      }
      if (action_on& kFrobFocusScript)
      {
         *p='\0';
         strcat(p,lev_names[lev]);
         sScrMsg focusmsg(obj,buf);
         pScriptMan->SendMessage(&focusmsg);
      }
   }
   if (loc == kFrobLocInv) // send a "select" event
   {
      cTagSet Event("Event Select");
      ESndPlay(&Event,obj,OBJ_NULL);
   }
   *objp=obj;
}


void FrobSendInvSelectionMsg(eFrobLoc loc, eFrobLev lev, ObjID old_obj, ObjID obj)
{
   char buf[120], *p;  // for script messages

   AutoAppIPtr_(ScriptMan,pScriptMan);
   strcpy(buf,loc_names[loc]);
   p=buf+strlen(buf);
   if (old_obj)
   {
      *p='\0';
      strcat(p,"De");
      strcat(p,lev_names[lev]);
      sScrMsg focusmsg(old_obj,buf);
      pScriptMan->SendMessage(&focusmsg);
   }
   if (obj)
   {
      *p='\0';
      strcat(p,lev_names[lev]);
      sScrMsg focusmsg(obj,buf);
      pScriptMan->SendMessage(&focusmsg);
   }
}



////////////////////////////////////////

BOOL IsTool(ObjID tool)
{
   sFrobInfo* pInfo;
   if (!pFrobInfoProp->Get(tool,&pInfo))
      return FALSE;

   return pInfo->actions[kFrobLocTool] != 0;
}

BOOL IsTakeable(ObjID obj)
{
   sFrobInfo* pInfo;
   if (!pFrobInfoProp->Get(obj,&pInfo))
      return FALSE;

   return pInfo->actions[kFrobLocWorld] & kFrobMove;

}

BOOL IsToolFrob(ObjID tool, ObjID target  )
{
   if (tool == OBJ_NULL || target == OBJ_NULL)
      return FALSE;

#ifndef SHOCK
   // @TODO: have some game-specific way of dealing with this?
   if (IsTakeable(target))
      return FALSE;
#endif // SHOCK

   return IsTool(tool);
}

////////////////////////////////////////

void FrobInit(void)
{
#ifdef NEW_NETWORK_ENABLED
   g_pFrobRequestMsg = new cNetMsg(&sFrobRequestDesc);
   g_pGlobalFrobMsg = new cNetMsg(&sGlobalFrobDesc);
   g_pFrobAcknowledgeMsg = new cNetMsg(&sFrobAcknowledgeDesc);
#endif
}
void FrobTerm(void)
{
#ifdef NEW_NETWORK_ENABLED
   delete g_pFrobRequestMsg;
   delete g_pGlobalFrobMsg;
   delete g_pFrobAcknowledgeMsg;
#endif
}
