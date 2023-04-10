/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcharg.h,v 1.14 2000/01/29 13:40:33 adurant Exp $
#pragma once

#ifndef __SHKCHARG_H
#define __SHKCHARG_H

#include "shktypes.h"

#include "event.h"

enum EService { KServiceBlackOps, KServiceMarines, KServiceNavy, KNumServices };
enum EGenState { KGenSelectService, KGenChooseMission, KGenDebriefing, KGenCharSummary, 
   KGenBirth, KGenSplash, KGenNone, KGenGameSummary, KGenMain };

class CCharGenerator {
protected:
	int m_year; // how many years of character progress have we gone through?
	EService m_service; // which service are you in
	EGenState m_state; // what state is the interface in
   CCharacter *m_player;

	int m_mission; // the mission ID code for the current mission (in KGenDebriefing state)
   int m_trait; // the selected trait hack
   int m_summary_focus; // hack for giving feedback on stats
   int m_subskills[2]; // subskill hacks

   char m_name[64]; 

   grs_font *m_font; // what font to use!
   int m_color; // drawing color

public:
	// start this puppy up
	CCharGenerator(CCharacter *who);
   ~CCharGenerator(void);

	// create the interface for the specified EGenState
	void BuildInterface(EGenState state);
	void BuildInterface(void) { BuildInterface(m_state); }

	// destroy the current interface elements (either for shutdown, or for switching to new state)
	void DestroyInterface(void);

	// switch to a new state, doing all appropriate interface frobbing
	void ChangeState(EGenState newstate);
   void SetState(EGenState newstate) { m_state = newstate; } // just force a state change (dangerous!)

	// interface "callbacks"
	void ChooseService(int button_num);
	void Restart(void);
	void ChooseMission(int button_num);
   void ChooseTrait(int button_num);
   void ChooseSummary(int button_num, bool redraw = TRUE);
   void PlusMinus(int button_num);

   // for uiDefer usage
   static void ContinueFunc(void *arg);
   void ContinueGuts(void);

   // continuous updating of the teletype in debriefing mode
   // perhaps should be just an Update function public which calls
   // this privately, but oh well
   void DrawTeletypeText(void);

   // Load/free all the internal art needed
   void LoadBitmaps(void);
   void FreeBitmaps(void);

   // grabbing input
   static bool KeyHandler(uiEvent *ev, Region *reg, void *data);
   bool ProcessKey(int code);

   uint m_teletime; // what time did we start teletyping

private:
   // Draw the background art
   void DrawBackground(char *artname);
   void DrawBackground(EGenState state);
   void DrawBackground(void) { DrawBackground(m_state); }

	// Apply the results of the specified mission ID to the character
	void ApplyMissionStats(int mission_id);

   // text drawing utility func
   void DrawUnwrappedText(char *text, Rect *r, bool center = TRUE);
   void DrawWrappedText(char *text, Rect *r, bool center = TRUE);

   // draw appropriate service-specific text
   void DrawServiceText(EService which_service);
   void DrawServiceText(void) { DrawServiceText(m_service); }

   void DrawJobLongDesc(void);
   void DrawJobShortDesc(void);
   void DrawYearInfo(void);

   void DrawTraitDesc(void);
   void DrawBirthTraits(void);
   void DrawBirthName(void);

   void DrawSummaryStats(void);
   void DrawSummaryDesc(void);
   void DrawSummarySubskills(void);
   void SubskillGuts(int skill_base);
};

extern CCharGenerator *gCharGenerator;

#endif