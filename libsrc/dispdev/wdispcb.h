// $Header: x:/prj/tech/libsrc/dispdev/RCS/wdispcb.h 1.1 1997/10/21 16:16:10 KEVIN Exp $

#ifndef __WDISPCB_H
#define __WDISPCB_H

#include <rect.h>
#include <cbchain.h>

typedef struct sWinDispDevCallbackInfo {
   callback_chain_info chain_info;
   int message;
   Rect *r;       // region affected, if applicable, or NULL for whole screen
} sWinDispDevCallbackInfo;

typedef enum eWinDispDevCallback {
   // Update chain
   kWinDispDevCallbackUpdateChain,

   // App coming active
   kWinDispDevCallbackActive,

   // App going Inactive
   kWinDispDevCallbackInactive,

   // App must repaint 
   kWinDispDevCallbackRepaint,
} eWinDispDevCallback;

#endif
