/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostmsg.cpp,v 1.7 1999/08/24 17:42:15 dc Exp $
// message types and declarations

#include <ghost.h>
#include <ghostmsg.h>
#include <ghostrcv.h>
#include <ghostlst.h>

#include <netmsg.h>

#include <dbmem.h>

// misc defines/setup
#define NET_GHOST_SPEW "ghost_spew"

////////////////////////
// the messages themselves
// g_ since shared by all ghost systems

// @TBD: is it worth having a packet w/no pitch or mode or flags
//       which only supports having tz, but no dtz, for size?
//       i.e. for AIs, basically... add a _NO_MODE_ define things can use...

cNetMsg *g_pGhostHeartbeatFullMsg = NULL;
cNetMsg *g_pGhostHeartbeatNormMsg = NULL;
cNetMsg *g_pGhostHeartbeatTerseNGMsg = NULL;
cNetMsg *g_pGhostMiniHeartbeatNGMsg = NULL;
cNetMsg *g_pGhostObjHeartbeatNGMsg = NULL;
cNetMsg *g_pGhostAIHeartbeatNGMsg = NULL;
cNetMsg *g_pGhostRotHeartbeatNGMsg = NULL;

////////////////////////
// actual packet definitions/descs/callbacks

static void handleGhostFullHB(ObjID ghost, ushort seq_id, ObjID relobj, sGhostHeartbeat *pGH, sGhostMoCap *pGMC)
{
   GhostRecvPacket(ghost,(int)seq_id,relobj,pGH,pGMC);
}

// if you are G, you get a mocap too...
static sNetMsgDesc gGhostHeartbeatFullDesc =
{
   kNMF_Broadcast, "GhostFullHB", "Ghost Full Heartbeat", NET_GHOST_SPEW,
   handleGhostFullHB,
   {{kNMPT_SenderObjID, kNMPF_None, "GhostObj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_GlobalObjID, kNMPF_None, "RelObj" },
    {kNMPT_Block, kNMPF_None, "HBData", sizeof(sGhostHeartbeat)},
    {kNMPT_Block, kNMPF_None, "MCData", sizeof(sGhostMoCap)},
    {kNMPT_End}}
};

// for non-schema stuff
static void handleGhostHeartbeatNorm(ObjID ghost, ushort seq_id, sGhostHeartbeat *pGH)
{
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,pGH,NULL);
}

// this is idiotic
static sNetMsgDesc gGhostHeartbeatNormDesc =
{
   kNMF_Broadcast, "GhostNormHB", "Ghost Normal Heartbeat", NET_GHOST_SPEW,
   handleGhostHeartbeatNorm,
   {{kNMPT_SenderObjID, kNMPF_None, "GhostObj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostHeartbeat)},
    {kNMPT_End}}
};

static void handleGhostHeartbeatTerse(ObjID ghost, ushort seq_id, sGhostHeartbeat *pGH)
{
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,pGH,NULL);
}

static sNetMsgDesc gGhostHeartbeatTerseNGDesc =
{
   kNMF_Broadcast|kNMF_Nonguaranteed,
   "GhostHBTerse", "Ghost Heartbeat Terse NonG", NET_GHOST_SPEW,
   handleGhostHeartbeatTerse,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostHeartbeat)},
    {kNMPT_End}}
};

// @TBD: do we support mini's?  more data copying in and out, but less bandwidth, eh?
static void handleGhostMiniHeartbeat(ObjID ghost, ushort seq_id, sGhostMiniHeartbeat *pGMH)
{
   sGhostRemote *pGR=GhostGetRemote(ghost);  // so we can get the old data...
   if (pGR==NULL) return;
   sGhostHeartbeat GH=pGR->info.last.pos;
   GH.pos=pGMH->pos; GH.vel.x=pGMH->velx; GH.vel.y=pGMH->vely;
   GH.angle_info.tz=pGMH->tz; GH.flags=pGMH->flags;
   GH.vel.z=0; GH.angle_info.dtz=0; // since we are checking for Mag here, not delta
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,&GH,NULL);  // just call on upto normal one
}

static sNetMsgDesc gGhostMiniHeartbeatNGDesc =
{
   kNMF_Broadcast|kNMF_Nonguaranteed,
   "GhostMiniHB", "Ghost Mini Heartbeat NonG", NET_GHOST_SPEW,
   handleGhostMiniHeartbeat,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostMiniHeartbeat)},
    {kNMPT_End}}
};

static void handleGhostObjHeartbeat(ObjID ghost, ushort seq_id, sGhostObjMiniHB *pGMH)
{
   sGhostRemote *pGR=GhostGetRemote(ghost);  // so we can get the old data...
   if (pGR==NULL) return;
   sGhostHeartbeat GH=pGR->info.last.pos;
   GH.pos=pGMH->pos; GH.vel=pGMH->vel;
   // set default flags for this type, eh?
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,&GH,NULL);  // just call on upto normal one
}

static sNetMsgDesc gGhostObjHeartbeatNGDesc =
{
   kNMF_Broadcast|kNMF_Nonguaranteed,
   "GhostObjHB", "Ghost Obj Heartbeat NonG", NET_GHOST_SPEW,
   handleGhostObjHeartbeat,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostObjMiniHB)},
    {kNMPT_End}}
};

static void handleGhostRotHeartbeat(ObjID ghost, ushort seq_id, sGhostRotMiniHB *pGMH)
{
   sGhostRemote *pGR=GhostGetRemote(ghost);  // so we can get the old data...
   if (pGR==NULL) return;
   sGhostHeartbeat GH=pGR->info.last.pos;
   GH.angle_info.fac=pGMH->angle_info.fac;
   GH.flags|=(kGhostHBAngOnly|kGhostHBNoZPos);
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,&GH,NULL);  // just call on upto normal one
}

static sNetMsgDesc gGhostRotHeartbeatNGDesc =
{
   kNMF_Broadcast|kNMF_Nonguaranteed,
   "GhostRotHB", "Ghost Rot Heartbeat NonG", NET_GHOST_SPEW,
   handleGhostRotHeartbeat,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostRotMiniHB)},
    {kNMPT_End}}
};

static void handleGhostAIHeartbeat(ObjID ghost, ushort seq_id, sGhostAIMiniHB *pGMH)
{
   sGhostRemote *pGR=GhostGetRemote(ghost);  // so we can get the old data...
   if (pGR==NULL) return;   
   sGhostHeartbeat GH=pGR->info.last.pos;

   // AI checks for velz and posz w/in epsilon of last packet, so we just get it from last...
   GH.pos.x=pGMH->posx; GH.pos.y=pGMH->posy; GH.vel.x=pGMH->velx; GH.vel.y=pGMH->vely;
   GH.angle_info.tz=pGMH->tz; GH.angle_info.dtz=pGMH->dtz;
   GH.flags|=kGhostHBNoZPos;
   handleGhostFullHB(ghost,seq_id,OBJ_NULL,&GH,NULL);  // just call on upto normal one
}

static sNetMsgDesc gGhostAIHeartbeatNGDesc =
{
   kNMF_Broadcast|kNMF_Nonguaranteed,
   "GhostAIHB", "Ghost AI Heartbeat NonG", NET_GHOST_SPEW,
   handleGhostAIHeartbeat,
   {{kNMPT_SenderObjID, kNMPF_None, "Obj" },
    {kNMPT_UShort, kNMPF_None, "SeqID" },
    {kNMPT_Block, kNMPF_None, "Data", sizeof(sGhostAIMiniHB)},
    {kNMPT_End}}
};

/////////////////////////////////
// actually do init/term

void GhostMsgsInit(void)
{
   g_pGhostHeartbeatFullMsg    = new cNetMsg(&gGhostHeartbeatFullDesc);
   g_pGhostHeartbeatNormMsg    = new cNetMsg(&gGhostHeartbeatNormDesc);
   g_pGhostHeartbeatTerseNGMsg = new cNetMsg(&gGhostHeartbeatTerseNGDesc);
   g_pGhostMiniHeartbeatNGMsg  = new cNetMsg(&gGhostMiniHeartbeatNGDesc);
   g_pGhostObjHeartbeatNGMsg   = new cNetMsg(&gGhostObjHeartbeatNGDesc);
   g_pGhostAIHeartbeatNGMsg    = new cNetMsg(&gGhostAIHeartbeatNGDesc);
   g_pGhostRotHeartbeatNGMsg   = new cNetMsg(&gGhostRotHeartbeatNGDesc);
}

void GhostMsgsTerm(void)
{
   delete g_pGhostHeartbeatFullMsg;
   delete g_pGhostHeartbeatNormMsg;
   delete g_pGhostHeartbeatTerseNGMsg;
   delete g_pGhostMiniHeartbeatNGMsg;
   delete g_pGhostObjHeartbeatNGMsg;
   delete g_pGhostAIHeartbeatNGMsg;
   delete g_pGhostRotHeartbeatNGMsg;
}
