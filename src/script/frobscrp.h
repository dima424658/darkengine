/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/frobscrp.h,v 1.1 1998/01/14 06:18:45 dc Exp $

#pragma once

#ifndef __FROBSCRP_H
#define __FROBSCRP_H

#include <scrptmsg.h>

#include <frobscrm.h>

#ifdef SCRIPT
#define OnFrobToolBegin()          SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobToolBegin,sFrobMsg)
#define DefaultOnFrobToolBegin()   SCRIPT_CALL_BASE(FrobToolBegin)
#define OnFrobToolEnd()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobToolEnd,sFrobMsg)
#define DefaultOnFrobToolEnd()     SCRIPT_CALL_BASE(FrobToolEnd)
#define OnFrobWorldBegin()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobWorldBegin,sFrobMsg)
#define DefaultOnFrobWorldBegin()  SCRIPT_CALL_BASE(FrobWorldBegin)
#define OnFrobWorldEnd()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobWorldEnd,sFrobMsg)
#define DefaultOnFrobWorldEnd()    SCRIPT_CALL_BASE(FrobWorldEnd)
#define OnFrobInvBegin()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobInvBegin,sFrobMsg)
#define DefaultOnFrobInvBegin()    SCRIPT_CALL_BASE(FrobInvBegin)
#define OnFrobInvEnd()             SCRIPT_MESSAGE_HANDLER_SIGNATURE_(FrobInvEnd,sFrobMsg)
#define DefaultOnFrobInvEnd()      SCRIPT_CALL_BASE(FrobInvEnd)

#define OnWorldFocus()             SCRIPT_MESSAGE_HANDLER_SIGNATURE(WorldFocus)
#define DefaultOnWorldFocus()      SCRIPT_CALL_BASE(WorldFocus)
#define OnWorldDeFocus()           SCRIPT_MESSAGE_HANDLER_SIGNATURE(WorldDeFocus)
#define DefaultOnWorldDeFocus()    SCRIPT_CALL_BASE(WorldDeFocus)
#define OnWorldSelect()            SCRIPT_MESSAGE_HANDLER_SIGNATURE(WorldSelect)
#define DefaultOnWorldSelect()     SCRIPT_CALL_BASE(WorldSelect)
#define OnWorldDeSelect()          SCRIPT_MESSAGE_HANDLER_SIGNATURE(WorldDeSelect)
#define DefaultOnWorldDeSelect()   SCRIPT_CALL_BASE(WorldDeSelect)
#define OnInvFocus()               SCRIPT_MESSAGE_HANDLER_SIGNATURE(InvFocus)
#define DefaultOnInvFocus()        SCRIPT_CALL_BASE(InvFocus)
#define OnInvDeFocus()             SCRIPT_MESSAGE_HANDLER_SIGNATURE(InvDeFocus)
#define DefaultOnInvDeFocus()      SCRIPT_CALL_BASE(InvDeFocus)
#define OnInvSelect()              SCRIPT_MESSAGE_HANDLER_SIGNATURE(InvSelect)
#define DefaultOnInvSelect()       SCRIPT_CALL_BASE(InvSelect)
#define OnInvDeSelect()            SCRIPT_MESSAGE_HANDLER_SIGNATURE(InvDeSelect)
#define DefaultOnInvDeSelect()     SCRIPT_CALL_BASE(InvDeSelect)
#endif

///////////////
// the base room script

#ifdef SCRIPT

#define FROB_MESSAGE_DEFINES() \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobToolBegin  ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobToolEnd    ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobWorldBegin ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobWorldEnd   ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobInvBegin   ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(FrobInvEnd     ,sFrobMsg) \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(WorldFocus)                \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(WorldDeFocus)              \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(WorldSelect)               \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(WorldDeSelect)             \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(InvFocus)                  \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(InvDeFocus)                \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(InvSelect)                 \
      SCRIPT_DEFINE_MESSAGE_DEFAULT(InvDeSelect)
      
#define FROB_MSG_MAP() \
      SCRIPT_MSG_MAP_ENTRY(FrobToolBegin ) \
      SCRIPT_MSG_MAP_ENTRY(FrobToolEnd   ) \
      SCRIPT_MSG_MAP_ENTRY(FrobWorldBegin) \
      SCRIPT_MSG_MAP_ENTRY(FrobWorldEnd  ) \
      SCRIPT_MSG_MAP_ENTRY(FrobInvBegin  ) \
      SCRIPT_MSG_MAP_ENTRY(FrobInvEnd    ) \
      SCRIPT_MSG_MAP_ENTRY(WorldFocus    ) \
      SCRIPT_MSG_MAP_ENTRY(WorldDeFocus  ) \
      SCRIPT_MSG_MAP_ENTRY(WorldSelect   ) \
      SCRIPT_MSG_MAP_ENTRY(WorldDeSelect ) \
      SCRIPT_MSG_MAP_ENTRY(InvFocus      ) \
      SCRIPT_MSG_MAP_ENTRY(InvDeFocus    ) \
      SCRIPT_MSG_MAP_ENTRY(InvSelect     ) \
      SCRIPT_MSG_MAP_ENTRY(InvDeSelect   )

#endif  // SCRIPT

#endif  // __FROBSCRP_H
