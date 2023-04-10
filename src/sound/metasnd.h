/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/metasnd.h,v 1.3 1998/11/03 04:22:51 dc Exp $
//

#pragma once

#ifndef __METASND_H
#define __METASND_H

#include <guistyle.h>

#define kMetaSndPanelLast     (-1)
#define kMetaSndPanelDefault   (0)
#define kMetaSndPanelMain      (1)
#define kMetaSndPanelSaveLoad  (2)
#define kMetaSndPanelOptions   (3)
#define kMetaSndPanelLoadout   (4)
#define kMetaSndPanelBook      (5)
#define kMetaSndPanelInGame    (6)
#define kMetaSndPanelCount     (7)

///////////
// actual play callback
EXTERN bool metaSndPlayGUICallback(StyleSound sound);

// call as we transit panels
EXTERN void metaSndEnterPanel(int panel_type);
EXTERN void metaSndExitPanel(BOOL kill_snds);

// call on enter/exit game mode
EXTERN void metaSndEnterGame(void);
EXTERN void metaSndExitGame(void);

/////////
// yea yea
EXTERN void metaSndInit(char *snd_prefix);
EXTERN void metaSndTerm(void);

#endif  // __METASND_H
