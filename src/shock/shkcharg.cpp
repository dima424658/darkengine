/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcharg.cpp,v 1.28 2000/02/19 12:36:39 toml Exp $

// NOTE: This whole file is out of date, and slated for total destruction
#if 0

#include <storeapi.h>
#include <resapi.h>
#include <respaths.h>

#include <stdlib.h>

#include <loopapi.h>
#include <comtools.h>
#include <appagg.h>
#include <timer.h>
#include <mprintf.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#include <kbcook.h>
#include <keydefs.h>
#include <editor.h> 

#include <shkcharg.h>
#include <shkplayr.h>
#include <shkmusic.h>

#include <appsfx.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define INTERFACE_PATH "iface\\"
static IRes *gHndBackBmp = NULL;

/* ------------------------------------------------------------ */
// should this be service dependant or something wacky like that?
#define NUM_TRAINING_YEARS	3
#define NUM_AVAIL_JOBS  5

enum EJobType { KJobMission, KJobPosting};

typedef struct 
{
	EJobType jobtype; // mission or posting
   char *shortdesc; // on-button job description
   char *longdesc; // longer description of job when seleced
	char *debrief; // name of debriefing portrait
	char *results; // description of what happens on the job (for debrief teletype)

	// stuff on how it affects your stats
} JobInfo;

JobInfo job_db[] = {
   { KJobMission, "Mission: Raiders", "Disaffected mercenaries are raiding Earth colonies near the Opihuichi system.  Naval units \
will engage enemy vessels and board them to quell anarchy and lawlessness.", "cgend002.pcx" },
   { KJobMission, "Mission: Ground Invasion", "Naval shock units must be deployed to clear out a landing zone for establishing an \
HQ unit on Orion IV."},
   { KJobMission, "Mission: Urban Pacification", "Economic distress has caused several colonies to threaten independence.  A strong \
military presence is being established to maintain order.", "cgend001.pcx", \
"Fighting is intense as rebellion spreads among the local populace during your assignment on the colony of New Haven.  On several \
occasions you are required to go into the streets to quell rioting and keep the peace.  Your squad is pinned down by heavy \
resistance forces, and suffers heavy casualties.\nYou gain 3 points of heavy weapons skill, and 1 point of weapons repair.  Injuries \
suffered during the fighting cost you 1 point of END."},
   { KJobPosting, "Posting: Energy Weapons", "High tech energy weapons require a lengthy training period before soldiers are \
qualified to use them in the field.","cgend002.pcx", 
"Although not the most exciting duty in the Navy, your weapons training proves useful.  Endless hours on the practice range \
pay off, and your AGI increases by 2, and you gain 5 points of energy weapons skill."},
   { KJobPosting, "Posting: Administration", "A large organization like the Navy requires a complex bureaucracy, and talented \
administrators to run it." },
   // 5
   { KJobPosting, "Posting: Psi Testing", "Naval scientists are experimenting with the development of a comprehensive course \
to stimulate latent psionic abilities, and need test subjects with high psionic potential.", "cgend000.pcx",
"After months of being poked, prodded, and probed, you blossom in psionic training!  Although not all of the experimental \
regimen is without side effects, your budding mental abilities will serve you well during your career.  Your ESP, Telepathy, \
and Projection skills go up by 3!"},
   { KJobPosting, "Posting: Officer Training", "The Navy's officers are the finest in the military, and they get that way through \
an intensive training program." },
   { KJobMission, "Mission: Deep Space Patrol", "A deep space tour of duty can be filled with dangerous hazards and exciting opportunities.", "cgend004.pcx",
"Months pass in deep space aboard the UNN Lexington, before you are called in to combat a band of spacefaring raiders!  Fortunately superior \
technology carries the day, and you rout the raiders after a few weeks of skirmishing.  You gain 3 points of energy weapons skill, \
2 points of PER, and 2 points of END." },
   { KJobMission, "Mission: Infiltrate the Von Braun", "Naval High Command has a critical need for your skills to defuse a \
tricky situation on board the deep space craft \"Von Braun\".  The ship's commander has gone renegade and you are authorized \
to use any and all means to eliminate Captain Kurtz.  A squadron of marines will assist you in overcoming any defenses that the \
Captain may have established on board the ship.", "cgend003.pcx", "You set out for the most eventful mission of your career..." },
};

int job_avail[NUM_TRAINING_YEARS][NUM_AVAIL_JOBS] = {
   { 0, 1, 3, 7, 5 }, // pick mission 7 (space patrol)
   { 0, 1, 3, 5, 2 }, // pick mission 5 (psi training)
   { 8, -1, -1, -1, -1 }, // forced pick of 8
};
   /*
   { 0, 1, 2, 3, 5 }, // pick mission 2 (urban pacification)
   { 0, 1, 3, 5, 6 }, // pick mission 5 (psi training)
   //{ 0, 1, 3, 7, 6 }, // pick mission 3 (gunnery drill)
   { 0, 1, 3, 7, 6 }, // pick mission 7 (space patrol)
   { 8, -1, -1, -1, -1 }, // forced pick of 8
   */
/* ------------------------------------------------------------ */
CCharGenerator::CCharGenerator(CCharacter *who)
{
	m_player = who;
	m_year = 0;
	m_state = KGenNone;
	m_service = KServiceBlackOps; // our default
   m_trait = 0;
   m_summary_focus = -1;
   m_color = -1;
   strcpy(m_name,"");
   
   int i;
   for (i=0; i < 2; i++)
      m_subskills[i] = -1;

   m_font = FALSE;

	// build the basic interface 
	// (should this happen here in the constructor, or be a subsequent call?)
	//BuildInterface(m_state);
}
/* ------------------------------------------------------------ */
CCharGenerator::~CCharGenerator()
{
   ResUnlock(RES_EditorFont);
}
/* ------------------------------------------------------------ */

// fake hack hack hack
int ChooseRandom(int num)
{
	return (rand() % num);
}

/* ------------------------------------------------------------ */
// shouldn't these be in some common utilities file?
// This leaves the art Locked; callers should be sure to Unlock before
// they release the handle.
IRes *LoadArt(char *artname, grs_bitmap **bmp)
{
   IRes *handle;

   // @TBD: We really ought to have a path for INTERFACE_PATH, which is
   // long-lived, instead of using this relative path each time:
   char buf[MAX_STOREPATH];
   strcpy(buf, INTERFACE_PATH);
   strcat(buf, artname);
   
   AutoAppIPtr(ResMan);
   handle = pResMan->PrepTypedResource(buf,
                                       "Image",
                                       gContextPath);
   if (handle != NULL)
   {
      *bmp = (grs_bitmap *) handle->Lock();
   }
   return(handle);
}

/* ------------------------------------------------------------ */
void CCharGenerator::DrawBackground(char *artname)
{
   grs_bitmap *bmp;
   IRes *hnd;
   hnd = LoadArt(artname,&bmp);
   gr_bitmap(bmp,0,0);
   hnd->Unlock();
   SafeRelease(hnd);
}

/* ------------------------------------------------------------ */

void CCharGenerator::DrawBackground(EGenState state)
{
   switch(state)
   {
      case KGenBirth:   
         DrawBackground("cgenbrth.pcx");
         break;
      case KGenSelectService:
         DrawBackground("cgenserv.pcx");
         break;
      case KGenChooseMission:
         DrawBackground("cgenchoo.pcx");
         break;
      case KGenDebriefing:
         if (job_db[m_mission].debrief != NULL)
            DrawBackground(job_db[m_mission].debrief);
         else
            DrawBackground("cgendebr.pcx");
         break;
      case KGenCharSummary:
      case KGenGameSummary:
         DrawBackground("cgensumm.pcx");
         break;
      case KGenSplash:
         DrawBackground("splash.pcx");
         break;
      case KGenMain:
         DrawBackground("splash2.pcx");
         break;
   }
}

/* ------------------------------------------------------------ */

static Rect service_rects[3] = { 
   {{13,246}, {166,479}},
   {{117,30}, {264,251}}, 
   {{194,247},{372,459}},
};

static Rect job_rects[NUM_AVAIL_JOBS] = { 
   {{410,230}, {430, 250}}, 
   {{410,270}, {430, 290}}, 
   {{410,310}, {430, 330}}, 
   {{410,350}, {430, 370}}, 
   {{410,390}, {430, 410}}, 
};

static Rect birth_rects[3] = { 
   {{100,300}, {150, 350}}, 
   {{100,350}, {150, 400}}, 
   {{100,400}, {150, 450}}, 
};

#define SUMMARY_X1   9
#define SUMMARY_X2   99
#define SUMMARY_X3   236
#define SUMMARY_X4   377
/*
static Rect summary_rects[NUM_CHAR_INFO] = {

   {{SUMMARY_X1,64},{SUMMARY_X2,83}}, // STR
   {{SUMMARY_X1,84},{SUMMARY_X2,103}}, // DEX
   {{SUMMARY_X1,104},{SUMMARY_X2,123}}, // CON
   {{SUMMARY_X1,124},{SUMMARY_X2,143}}, // INT
   {{SUMMARY_X1,144},{SUMMARY_X2,163}}, // PER

   {{SUMMARY_X3,253},{SUMMARY_X4,268}}, // weapons
   {{SUMMARY_X3,269},{SUMMARY_X4,284}}, 
   {{SUMMARY_X3,285},{SUMMARY_X4,300}}, 
   {{SUMMARY_X3,301},{SUMMARY_X4,316}}, 
   {{SUMMARY_X3,317},{SUMMARY_X4,332}}, 

   {{SUMMARY_X3,63},{SUMMARY_X4,78}}, // psi
   {{SUMMARY_X3,79},{SUMMARY_X4,94}},
   {{SUMMARY_X3,95},{SUMMARY_X4,110}},
   {{SUMMARY_X3,111},{SUMMARY_X4,126}},
   {{SUMMARY_X3,127},{SUMMARY_X4,142}},
   {{SUMMARY_X3,143},{SUMMARY_X4,158}},
   {{SUMMARY_X3,159},{SUMMARY_X4,174}},
   {{SUMMARY_X3,175},{SUMMARY_X4,190}},
   {{SUMMARY_X3,191},{SUMMARY_X4,205}},
   
   {{SUMMARY_X3,377},{SUMMARY_X4,392}},  // tech
   {{SUMMARY_X3,393},{SUMMARY_X4,408}},
   {{SUMMARY_X3,409},{SUMMARY_X4,424}},
   {{SUMMARY_X3,425},{SUMMARY_X4,440}},
   
};
*/
//static Rect summary_plusminus_rects[NUM_CHAR_INFO * 2];

char *summary_names[] = { "","","","","", // stats have bitmap descriptors
   "Guns", "Rifles", "Energy Weapons", "Grenades", "Heavy Weapons",
   "Psi Dampening", "Projection", "Kinesis", "ESP", "Physic", "Psi Parasitism", "Telekinesis", "Telepathy", "Serenity",
   "Computer Hacking", "Weapons Repair", "Weapons Modification", "Electronics Repair",
};

char *summary_descs[] = { "Strength is a measurement of physical prowess.", "Perception determines your awareness and \
likelihood of noticing small, important details and avoiding unseen dangers.", "Endurance measures how much punishment you \
can take and how long you can keep going under adverse conditions.","Intelligence determines your overall mental ability, and \
governs the effectiveness of Psionic powers.", "Agility is quickness and dexterity, being both fleet of foot and deft of hand.", 
"Guns skill covers all normal slug-throwing one handed weapons.", "Rifles skill covers proper usage of heavier, two-handed \
projectile weapons", "The Energy Weapons skill is for the proper use of energy or plasma based weapons like the fusion welder.", 
"Grenades skill covers safe and effective use of short-range explosives.", "Heavy Weapons skill is vital for the proper use of \
advanced, experimental or high-tech weaponry.",
   "Psi Dampening is the discipline that covers neutralization of enemy psionic abilities.", "The Projection discipline is for out-of-body \
psionic abilities", "Kinesis is the art of altering molecular structures.", "The ESP displine allows the psionicist to extend their \
senses beyond the physical realm.", "Physic allows one to influence biological energies.", "Psi Parasitism allows you to drain psionic or physical \
energies from others.", "Telekinesis is the art of manipulating matter at a distance.", "Telepathy allows one to send and receive thoughts.", 
"Serenity governs overall psi power usage and efficiency.",
   "The Computer Hacking skill is used for many varieties of electronic larceny.", "Weapons Repair is a key field skill for\
fixing damaged or jammed weapons.", "Weapons Modification lets you jury-rig weapons for an extra punch.", "Electronics Repair is essential \
for fixing damaged high-tech equipment.",
};

static Rect cont_rect = {{526,425},{604,446}};
static Rect splash_rect = {{0,0}, {640,480}};

/*
static LGadButtonList service_blist;
static LGadButtonListDesc service_blistdesc;
static LGadButtonList job_blist;
static LGadButtonListDesc job_blistdesc;
static LGadButtonList birth_blist;
static LGadButtonListDesc birth_blistdesc;
static LGadButtonList summary_blist;
static LGadButtonListDesc summary_blistdesc;
static LGadButtonList summary_plusminus_blist;
static LGadButtonListDesc summary_plusminus_blistdesc;
static LGadButton cont_button;
static LGadButton splash_button;
static DrawElement service_blist_elems[3];
static DrawElement job_blist_elems[NUM_AVAIL_JOBS];
static DrawElement birth_blist_elems[3];
static DrawElement summary_blist_elems[NUM_CHAR_INFO];
static DrawElement summary_plusminus_elems[NUM_CHAR_INFO * 2];
static DrawElement cont_elem;
static DrawElement splash_elem;

static grs_bitmap *service_blist_bitmaps[3][4];
static grs_bitmap *job_bitmaps[4];
static grs_bitmap *summary_stat_icons[5];
static grs_bitmap *cont_bitmap;
static grs_bitmap *summary_plusminus_icons[2];
static IRes *service_blist_handles[3][2];
static IRes *job_handles[2];
static IRes *cont_handle;
static IRes *summary_stat_handles[5];
static IRes *summary_plusminus_handles[2];
*/
/* ------------------------------------------------------------ */
void CCharGenerator::LoadBitmaps(void)
{
#if 0
   char temp[40];
   int i,s;

   cont_handle= LoadArt("cgencont.pcx",&cont_bitmap);
   for (s = 0; s < 3; s++)
   {
      for (i = 0; i < 2; i++)
      {
         sprintf(temp,"cgen%ds%d.pcx",s+1,i+1);
         service_blist_handles[s][i] = LoadArt(temp,&service_blist_bitmaps[s][i]);
      }
      for (i = 2; i < 4; i++)
      {
         service_blist_bitmaps[s][i] = service_blist_bitmaps[s][0];
      }
   }
   for (i = 0; i < 2; i++)
   {
      sprintf(temp,"cgenj%d.pcx",i+1);
      job_handles[i] = LoadArt(temp,&job_bitmaps[i]);
   }
   for (i = 2; i < 4; i++)
   {
      job_bitmaps[i] = job_bitmaps[0];
   }

   for (i=0;i < 5; i++)
   {
      sprintf(temp,"cgenicn%d.pcx",i+1);
      summary_stat_handles[i] = LoadArt(temp,&summary_stat_icons[i]);
   }

   summary_plusminus_handles[0] = LoadArt("cgenplus.pcx",&summary_plusminus_icons[0]);
   summary_plusminus_handles[1] = LoadArt("cgenminu.pcx",&summary_plusminus_icons[1]);

   // set up the continue button
   cont_elem.draw_type = DRAWTYPE_BITMAP;
   cont_elem.draw_data = cont_bitmap;
#endif
}

#define UnlockAndRelease(res) { res->Unlock(); SafeRelease(res); }

/* ------------------------------------------------------------ */
void CCharGenerator::FreeBitmaps(void)
{
#if 0
   int s,i;

   UnlockAndRelease(cont_handle);
   for (s = 0; s < 3; s++)
   {
      for (i = 0; i < 2; i++)
      {
         UnlockAndRelease(service_blist_handles[s][i]);
      }
   }
   for (i = 0; i < 2; i++)
   {
      UnlockAndRelease(job_handles[i]);
   }
   for (i=0; i < 5; i++)
   {
      UnlockAndRelease(summary_stat_handles[i]);
   }
   for (i = 0; i < 2; i++)
   {
      UnlockAndRelease(summary_plusminus_handles[i]);
   }
#endif
}

#pragma off(unreferenced)

/* ------------------------------------------------------------ */
bool CCharGenerator::KeyHandler(uiEvent *ev, Region *reg, void *data)
{
   uiCookedKeyEvent *key = (uiCookedKeyEvent *)ev;
   int code = key->code;
   return(gCharGenerator->ProcessKey(code));
}

/* ------------------------------------------------------------ */
bool CCharGenerator::ProcessKey(int code)
{
   if (m_state != KGenCharSummary) // Birth)
      return (FALSE);

   int n = strlen(m_name);

   if (isalpha(code) || isspace(code))
   {
      m_name[n] = code;
      m_name[n+1] = '\0';
   }
   else
   {
      switch (code)
      {
      case KEY_BS:
         if (n > 0)
            m_name[n-1] = '\0';
         break;
      }
   }
   DrawBirthName();
   return(TRUE);
}
/* ------------------------------------------------------------ */
static bool cont_cb(short action, void* data, LGadBox* vb)
{
   if (action == MOUSE_LUP)
   {
      uiDefer(CCharGenerator::ContinueFunc,NULL);
      // do continue code
      return(TRUE);
   }
   else
      return(FALSE);
}
/* ------------------------------------------------------------ */
static bool service_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   gCharGenerator->ChooseService(button);

   return TRUE;
}
/* ------------------------------------------------------------ */
static bool job_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   gCharGenerator->ChooseMission(button);

   return TRUE;
}
/* ------------------------------------------------------------ */
static bool birth_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   gCharGenerator->ChooseTrait(button);

   return TRUE;
}
/* ------------------------------------------------------------ */
static bool summary_select_cb(ushort action, int button, void* data, LGadBox* vb)
{
   gCharGenerator->ChooseSummary(button);

   return TRUE;
}
/* ------------------------------------------------------------ */
static bool summary_plusminus_cb(ushort action, int button, void* data, LGadBox* vb)
{
   if (action == MOUSE_LUP)
   {
      gCharGenerator->PlusMinus(button);
      return(TRUE);
   }
   else
      return(FALSE);
}
#pragma on(unreferenced)
/* ------------------------------------------------------------ */
// note fullwise the stupid hackery
char *service_texts[3] = {
   "You won't find the OSA in any traditional military texts.  This \"black ops\" group specializes in covert activities and \
plausible deniability.  The main focus here is on subversion and psionics rather than weapons training and combat.",
   "The Marines handle the roughest, toughest fighting in the modern military.  Although subtlety is not their forte, the quality \
of weapons training in the Marines is the best around.  A very small amount of technical and psionic training can be found, for specialty \
jobs.",
   "A tour of duty in the Navy isn't just hot-dogging around in the latest fighter jets.  Naval ground forces are deployed \
to a variety of extra-planetary targets and have to perform in a wide variety of military operations.  Training in the navy \
is the most well-rounded of all the services, with an even mix of weapons, technical, and psionic operations.",
};

#define TEXT_SIZE       1024
#define SERVICE_TEXT_X  410
#define SERVICE_TEXT_Y  38
#define SERVICE_TEXT_WIDTH    200
#define SERVICE_TEXT_HEIGHT   280

void CCharGenerator::DrawServiceText(EService serv)
{
   Rect r = {{ SERVICE_TEXT_X, SERVICE_TEXT_Y}, {SERVICE_TEXT_X + SERVICE_TEXT_WIDTH, SERVICE_TEXT_Y + SERVICE_TEXT_HEIGHT}};
   DrawUnwrappedText(service_texts[serv],&r,FALSE);
}
/* ------------------------------------------------------------ */
#define JOB_TEXT_X  430
#define JOB_TEXT_Y  80
#define JOB_TEXT_WIDTH  200
#define JOB_TEXT_HEIGHT 140

void CCharGenerator::DrawJobShortDesc()
{
   int i;
   Rect r;
   for (i=0; i < NUM_AVAIL_JOBS; i++)
   {
      r.ul.x = job_rects[i].lr.x + 10;
      r.ul.y = job_rects[i].ul.y;
      r.lr.x = JOB_TEXT_X + JOB_TEXT_WIDTH;
      r.lr.y = job_rects[i].lr.y;
      if (job_avail[m_year][i] != -1)
         DrawUnwrappedText(job_db[job_avail[m_year][i]].shortdesc,&r, FALSE);
   }
}
/* ------------------------------------------------------------ */
void CCharGenerator::DrawJobLongDesc()
{
   Rect r = {{ JOB_TEXT_X, JOB_TEXT_Y}, {JOB_TEXT_X + JOB_TEXT_WIDTH, JOB_TEXT_Y + JOB_TEXT_HEIGHT}};
   DrawUnwrappedText(job_db[m_mission].longdesc,&r, FALSE);
}
/* ------------------------------------------------------------ */
void CCharGenerator::DrawYearInfo()
{
   char temp[255];
   sprintf(temp,"YEAR: %d",m_year+1);
   gr_font_string(m_font,temp, 8, 8);
}
/* ------------------------------------------------------------ */
char *trait_descs[] = { "You are a big, stupid lug.  Duh.  Need I say more?",
   "Your metabolism is super speedy, ya drug addicted freak!",
   "You are a creepy mutant.  You'll be at home in LA, but no where else.",
};
void CCharGenerator::DrawTraitDesc()
{
   Rect r = {{ JOB_TEXT_X, JOB_TEXT_Y}, {JOB_TEXT_X + JOB_TEXT_WIDTH, JOB_TEXT_Y + JOB_TEXT_HEIGHT}};
   DrawUnwrappedText(trait_descs[m_trait],&r);
}
/* ------------------------------------------------------------ */
char *trait_text[] = { "Big Lug", "Fast Metabolism", "Freaky Personality" };
void CCharGenerator::DrawBirthTraits()
{
   int i;
   Rect r;
   for (i=0; i < 3; i++)
   {
      r.ul.x = birth_rects[i].lr.x + 10;
      r.ul.y = birth_rects[i].ul.y;
      r.lr.x = JOB_TEXT_X - 25;
      r.lr.y = birth_rects[i].lr.y;
      DrawUnwrappedText(trait_text[i],&r);
   }
}
/* ------------------------------------------------------------ */
void CCharGenerator::DrawBirthName()
{
   char temp[255];
   Rect r1 = {{17,12},{230,26}}; //{{177,46}, {377,96}};
   Rect r2 = {{239,12},{296,26}};
   Rect r3 = {{307,12},{383,26}};
   Rect r4 = {{395,12},{450,26}};
   Rect r5 = {{19,209},{103,223}};
   sprintf(temp,"%s",m_name);
   DrawUnwrappedText(temp,&r1,FALSE);

   DrawUnwrappedText("Male",&r2,FALSE);

   DrawUnwrappedText("Sergeant",&r3,FALSE);

   sprintf(temp,"Age: %d",m_year + 22);
   DrawUnwrappedText(temp,&r4,FALSE);

   DrawUnwrappedText("Crack Shot",&r5, FALSE);
}
/* ------------------------------------------------------------ */
#define TELETYPE_TEXT_X  450
#define TELETYPE_TEXT_Y  150
#define TELETYPE_TEXT_WIDTH 170
#define TELETYPE_TEXT_HEIGHT 200
void CCharGenerator::DrawTeletypeText(void)
{
   // Only do teletype in Debriefing
   if (m_state != KGenDebriefing)
      return;

   // Are we done teletyping?
   if (m_teletime == 0)
      return;

   // Don't bother if we have no results text (should never happen in structured demonstration)
   if (job_db[m_mission].results == NULL)
      return;

   Rect r = {{ TELETYPE_TEXT_X, TELETYPE_TEXT_Y}, {TELETYPE_TEXT_X + TELETYPE_TEXT_WIDTH, TELETYPE_TEXT_Y + TELETYPE_TEXT_HEIGHT}};
   int num_chars;
   char tele_text[TEXT_SIZE];
   char temp_text[TEXT_SIZE];

   if (m_teletime == -1)
      num_chars = strlen(job_db[m_mission].results) + 1;
   else
      // compute how many characters to draw
      num_chars = (tm_get_millisec() - m_teletime) / (1000 / 25);

   // copy just enough of the mission result text
   strcpy(temp_text, job_db[m_mission].results);
   gr_font_string_wrap(m_font, temp_text, RectWidth(&r));
   strncpy(tele_text, temp_text, num_chars);
   tele_text[num_chars] = '\0';
   if (num_chars > strlen(temp_text))
      m_teletime = 0; // stop teletyping

   // draw it
   DrawWrappedText(tele_text, &r, FALSE);
}
/* ------------------------------------------------------------ */
#define PLUSMINUS_BITMAP_SIZE 15
#define PLUSMINUS_X1 138 
#define PLUSMINUS_X2 156
#define PLUSMINUS_X3 416
#define PLUSMINUS_X4 436
//#define SUMMARY_STATTEXT_WIDTH 139
#define SUMMARY_STAT_X1 104
#define SUMMARY_STAT_X2 383 
#define SUMMARY_STATVAL_WIDTH 28
//#define SUMMARY_PLUSMINUS_WIDTH  ((PLUSMINUS_BITMAP_SIZE * 2) + 4)
#define SUMMARY_COST_X1 187
#define SUMMARY_COST_X2 459
#define SUMMARY_COST_WIDTH 26
#define SUMMARY_SUBSKILL_X 489
#define SUMMARY_SUBSKILL_WIDTH 127
#define SUMMARY_POOL_X  592
#define SUMMARY_POOL_Y  14

#define NUM_SKILL_COLORS   20
int skill_colors[NUM_SKILL_COLORS] = { 
   21, 41, 39, 37, 35, 33, 248, 246, 244, 242, 
   240, 109, 87, 85, 83, 81, 79, 78, 253, 231,
};
void CCharGenerator::DrawSummaryStats()
{
   /*
   int i;
   Rect r;
   char temp[255];
   for (i=0; i < NUM_CHAR_INFO; i++)
   {
      if (i < WPN_1) // column 1
      {
         r.ul.x = SUMMARY_STAT_X1;
         m_color = skill_colors[m_player->m_info[i] * 4];
      }
      else
      {
         r.ul.x = SUMMARY_STAT_X2;
         m_color = skill_colors[m_player->m_info[i]];
      }
      r.ul.y = summary_rects[i].ul.y;
      r.lr.x = r.ul.x + SUMMARY_STATVAL_WIDTH;
      r.lr.y = summary_rects[i].lr.y;
      sprintf(temp,"%d",m_player->m_info[i]); // ,m_player->StatCost(i));
      DrawWrappedText(temp,&r);
      m_color = -1;

      if (i < WPN_1)
         r.ul.x = SUMMARY_COST_X1;
      else
         r.ul.x = SUMMARY_COST_X2;
      r.ul.y = summary_rects[i].ul.y;
      r.lr.x = r.ul.x + SUMMARY_COST_WIDTH;
      r.lr.y = summary_rects[i].lr.y;
      sprintf(temp,"(%d)",m_player->StatCost(i));
      DrawWrappedText(temp,&r);
   }

   sprintf(temp, "%d",m_player->m_pool);
   r.ul.x = SUMMARY_POOL_X;
   r.ul.y = SUMMARY_POOL_Y;
   r.lr.x = r.ul.x + 50;
   r.lr.y = r.ul.y + 10;
   DrawWrappedText(temp,&r, FALSE);
   */
}
/* ------------------------------------------------------------ */
#define SUMMARY_TEXT_X  14
#define SUMMARY_TEXT_Y  324
#define SUMMARY_TEXT_WIDTH 200
#define SUMMARY_TEXT_HEIGHT 123
void CCharGenerator::DrawSummaryDesc()
{
   Rect r = {{ SUMMARY_TEXT_X, SUMMARY_TEXT_Y}, {SUMMARY_TEXT_X + SUMMARY_TEXT_WIDTH, SUMMARY_TEXT_Y + SUMMARY_TEXT_HEIGHT}};
   if (m_summary_focus != -1)
      DrawUnwrappedText(summary_descs[m_summary_focus],&r,FALSE);
}
/* ------------------------------------------------------------ */
/*
char *summary_subskill_text[NUM_CHAR_INFO][NUM_SUBSKILLS] = {
   { "", "", "", "" ,"" },
   { "", "", "", "" ,"" },
   { "", "", "", "" ,"" },
   { "", "", "", "" ,"" },
   { "", "", "", "" ,"" },
   {"Viper G9", "Kord 5.9", "Muerte 800 XL", "Diablo 666", "\"Hawk\" Auto." },
   {"Sniper Rifle","DoomBringer","Assault Rifle", "Xygar A9","SmartShooter"},
   {"HS-1138 \"Falcon\"","Neural Agg.","Plasma Gun","EMP Pulse Rifle","AK-4700 Laser"},
   {"Conc. Grenade","Timer Grenade","Starflare Gren.","Smoke Grenade","EMP Grenade", },
   {"Port. Launcher","Quantum Decel.","Field Gen.","Plasma Cannon",""},
   {"Psi Deaden", "Psi Chaff","","",""}, //0 
   {"Astral Projection", "Time Travel", "","",""}, //1
   {"Cyber-Psi","Dampen","Energy Shield", "Shock","" },//2
   {"Clairvoyance", "Clairaudience", "Precognition","",""},//3
   {"Healing","Cure Poison","Cure Radiation","Sense Aura",""},//4
   {"Psi Drain","Soma Drain","Psyche Drain", "",""},//5
   {"Levitate","Cryokinesis","Pyrokinesis","Push","Pull",},//6
   {"Suggestion","Confuse","Decoy","Chameleon","Mind Terror",},//7
   {"Control","Focus","Respect","Perspective","Balance",},//8
};
*/

void CCharGenerator::DrawSummarySubskills()
{
   SubskillGuts(0);
   SubskillGuts(1);
}

void CCharGenerator::SubskillGuts(int skill_base)
{
   /*
   Rect r;
   int i;
   int skill_type;
   char temp[32];

   if (m_subskills[skill_base] == -1)
      return;

   if (skill_base == 0)
      skill_type = WPN_1;
   else
      skill_type = PSI_1;

   for (i=0; i < NUM_SUBSKILLS; i++)
   {
      char *textp = summary_subskill_text[m_subskills[skill_base]][i];
      if ((textp == NULL) || (strlen(textp) == 0))
         continue;

      int lvl = m_player->GetSubskillLevel(m_subskills[skill_base],i);
      if (lvl >= NUM_SKILL_COLORS)
         lvl = NUM_SKILL_COLORS - 1;
      if (lvl < 0)
         lvl = 0;
      m_color = skill_colors[lvl];

      r.ul.x = SUMMARY_SUBSKILL_X;
      r.ul.y = summary_rects[skill_type+i].ul.y + (RectHeight(&summary_rects[skill_type+i]) - 
         gr_font_string_height(m_font,textp))/2;
      r.lr.x = r.ul.x + SUMMARY_SUBSKILL_WIDTH;
      r.lr.y = summary_rects[skill_type+i].lr.y;
      DrawUnwrappedText(textp,&r,FALSE);

      r.ul.x = r.lr.x - 20;
      sprintf(temp,"%d%%",lvl*5);
      DrawUnwrappedText(temp,&r,FALSE);
   }
   m_color = -1;
   */
}
/* ------------------------------------------------------------ */
void CCharGenerator::DrawUnwrappedText(char *text, Rect *r, bool center)
{
   char display_text[TEXT_SIZE];
   // load up the text
   strcpy(display_text,text);
   // wrap
   gr_font_string_wrap(m_font, display_text, RectWidth(r));
   DrawWrappedText(display_text,r,center);
}
/* ------------------------------------------------------------ */
void CCharGenerator::DrawWrappedText(char *text, Rect *r, bool center)
{
   int s1,s2,s3,s4;
   int w,h;
   int dx,dy;
   if (center)
   {
      w = gr_font_string_width(m_font,text);
      h = gr_font_string_height(m_font,text);
      dx = r->ul.x + (RectWidth(r)-w)/2;
      dy = r->ul.y + (RectHeight(r)-h)/2;
   }
   else
   {
      w = RectWidth(r);
      h = RectHeight(r);
      dx = r->ul.x;
      dy = r->ul.y;
   }

   // clear out area
   gr_get_cliprect(&s1,&s2,&s3,&s4);
   gr_set_cliprect(dx,dy,dx+w,dy+h);
   DrawBackground();
   gr_set_cliprect(s1,s2,s3,s4);

   // draw it
   if (m_color == -1)
      gr_set_fcolor(guiStyleGetColor(NULL,StyleColorWhite));
   else
      gr_set_fcolor(m_color); 
   gr_font_string(m_font, text, dx, dy);
}
/* ------------------------------------------------------------ */
void CCharGenerator::BuildInterface(EGenState state)
{
#if 0
   int n,i;
   int count;
   //char temp[255];

	// build the relevant buttons, display the relevant art
	// install appropriate callbacks, and so forth
   if (m_font == NULL)
   {
      m_font = (grs_font *)ResLock (RES_EditorFont);
   }

   DrawBackground(state);

	switch (state)
	{
   case KGenSplash:
   case KGenMain:
      splash_elem.draw_type = DRAWTYPE_NONE;
      splash_elem.draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP);
      LGadCreateButtonArgs(&splash_button, LGadCurrentRoot(), splash_rect.ul.x, splash_rect.ul.y,
         RectWidth(&splash_rect), RectHeight(&splash_rect), &splash_elem, cont_cb, 0);
      break;
   case KGenBirth:
      for (n=0; n < 3; n++)
      {
         DrawElement *elem = &birth_blist_elems[n];
         elem->draw_type = DRAWTYPE_BITMAPOFFSET;
         elem->draw_data = job_bitmaps;
         elem->draw_data2 = (void *)4; // should be 2 but hackery required
         //elem->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
      }
      birth_blistdesc.num_buttons = 3;
      birth_blistdesc.button_rects = birth_rects;
      birth_blistdesc.button_elems = birth_blist_elems;
      birth_blistdesc.cb = birth_select_cb;
      birth_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&birth_blist, LGadCurrentRoot(), &birth_blistdesc);

      LGadCreateButtonArgs(&cont_button, LGadCurrentRoot(), cont_rect.ul.x, cont_rect.ul.y,
         RectWidth(&cont_rect), RectHeight(&cont_rect), &cont_elem, cont_cb, 0);

      DrawBirthTraits();
      DrawTraitDesc();
      
      break;
	case KGenSelectService:

		// make a button for each service, ie a button list
      for (n=0; n < 3; n++)
      {
         DrawElement *elem = &service_blist_elems[n];
         elem->draw_type = DRAWTYPE_NONE;
         elem->draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP);
         /*
         elem->draw_type = DRAWTYPE_BITMAPOFFSET;
         elem->draw_data = service_blist_bitmaps[n];
         elem->draw_data2 = (void *)4; // should be 2 but hackery required
         */
      }
      // and a confirm button
      LGadCreateButtonArgs(&cont_button, LGadCurrentRoot(), cont_rect.ul.x, cont_rect.ul.y,
         RectWidth(&cont_rect), RectHeight(&cont_rect), &cont_elem, cont_cb, 0);
      
      service_blistdesc.num_buttons = 3;
      service_blistdesc.button_rects = service_rects;
      service_blistdesc.button_elems = service_blist_elems;
      service_blistdesc.cb = service_select_cb;
      service_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&service_blist, LGadCurrentRoot(), &service_blistdesc);

		// and the text/art area
      DrawServiceText();

		break;

	case KGenChooseMission:
		// figure out what missions are available 
		// create the selection buttons
		// the continue button is still around from the service selection, wahoo
		// make a button for each service, ie a button list
      count = 0;
      for (n=0; n < NUM_AVAIL_JOBS; n++)
      {
         if (job_avail[m_year][n] != -1)
         {
            count++;
            DrawElement *elem = &job_blist_elems[n];
            elem->draw_type = DRAWTYPE_BITMAPOFFSET;
            elem->draw_data = job_bitmaps;
            elem->draw_data2 = (void *)4; // should be 2 but hackery required
            elem->draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP);
            //elem->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
         }
      }
      job_blistdesc.num_buttons = count; // NUM_AVAIL_JOBS;
      job_blistdesc.button_rects = job_rects;
      job_blistdesc.button_elems = job_blist_elems;
      job_blistdesc.cb = job_select_cb;
      job_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&job_blist, LGadCurrentRoot(), &job_blistdesc);

      LGadCreateButtonArgs(&cont_button, LGadCurrentRoot(), cont_rect.ul.x, cont_rect.ul.y,
         RectWidth(&cont_rect), RectHeight(&cont_rect), &cont_elem, cont_cb, 0);
 
      // Draw in short descs
      DrawJobShortDesc();

      // Draw the long desc for the default selected obj
      m_mission = job_avail[m_year][0];
      DrawJobLongDesc();

      DrawYearInfo();

		break;

	case KGenDebriefing:
		// the gorgeous art in the background, of course, already drawn up top

		// a "go on" button, could say two things based on whether exit or mission select is next?
      LGadCreateButtonArgs(&cont_button, LGadCurrentRoot(), cont_rect.ul.x, cont_rect.ul.y,
         RectWidth(&cont_rect), RectHeight(&cont_rect), &cont_elem, cont_cb, 0);

      // debriefing tele-type is handled on a frame by frame basis
      m_teletime = tm_get_millisec(); //GetSimTime();
		break;

	case KGenCharSummary:
	case KGenGameSummary:
		// detailed character summary 
      // button for each line
      for (n=0; n < NUM_CHAR_INFO; n++)
      {
         DrawElement *elem = &summary_blist_elems[n];
         if (n >= WPN_BASE) // ie, not a stat
         {
            elem->draw_type = DRAWTYPE_TEXT;
            elem->draw_data = summary_names[n];
            elem->fcolor = guiStyleGetColor(NULL,StyleColorWhite); 
            elem->draw_flags = FORMAT(DRAWFLAG_FORMAT_LEFT)|INTERNAL(DRAWFLAG_INT_TRANSP);
         }
         else
         {
            elem->draw_type = DRAWTYPE_NONE; //DRAWTYPE_BITMAP;
            elem->draw_data = summary_stat_icons[n];
            elem->draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP);
         }
      }

      summary_blistdesc.num_buttons = NUM_CHAR_INFO;
      summary_blistdesc.button_rects = summary_rects;
      summary_blistdesc.button_elems = summary_blist_elems;
      summary_blistdesc.cb = summary_select_cb;
      summary_blistdesc.flags = BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&summary_blist, LGadCurrentRoot(), &summary_blistdesc);

      // now create the GIANT HORDE of plus/minus buttons
      for (n=0; n < NUM_CHAR_INFO; n++)
      {
         for (i=0; i< 2; i++)
         {
            DrawElement *elem = &summary_plusminus_elems[(n*2)+i];
            Rect r;
            elem->draw_type = DRAWTYPE_NONE; // DRAWTYPE_BITMAP;
            elem->draw_data = summary_plusminus_icons[i];
            elem->draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP);
            if (n < WPN_1)
            {
               r.ul.x = PLUSMINUS_X1+(i*25);
               r.lr.x = PLUSMINUS_X2+(i*25);
            }
            else
            {
               r.ul.x = PLUSMINUS_X3+(i*20);
               r.lr.x = PLUSMINUS_X4+(i*20);
            }
            r.ul.y = summary_rects[n].ul.y;
            r.lr.y = summary_rects[n].lr.y;
            summary_plusminus_rects[(n*2)+i] = r;
         }
      }

      summary_plusminus_blistdesc.num_buttons = NUM_CHAR_INFO * 2;
      summary_plusminus_blistdesc.button_rects = summary_plusminus_rects;
      summary_plusminus_blistdesc.button_elems = summary_plusminus_elems;
      summary_plusminus_blistdesc.cb = summary_plusminus_cb;

      LGadCreateButtonListDesc(&summary_plusminus_blist, LGadCurrentRoot(), &summary_plusminus_blistdesc);

      // the omnipresent continue button
      LGadCreateButtonArgs(&cont_button, LGadCurrentRoot(), cont_rect.ul.x, cont_rect.ul.y,
         RectWidth(&cont_rect), RectHeight(&cont_rect), &cont_elem, cont_cb, 0);

      DrawSummaryStats();
      DrawSummaryDesc();
      DrawSummarySubskills();
      DrawBirthName();
		break;
	}
   LGadDrawBox(VB(LGadCurrentRoot()),NULL);
#endif
}

/* ------------------------------------------------------------ */
void CCharGenerator::DestroyInterface(void)
{
#if 0
	// eliminate all those pesky buttons we just built
   LGadHideBox(VB(LGadCurrentRoot()),TRUE);
	switch (m_state)
	{
   case KGenSplash:
   case KGenMain:
      LGadDestroyBox(VB(&splash_button),FALSE);
      break;
   case KGenBirth:
      LGadDestroyButtonList(&birth_blist);
      LGadDestroyBox(VB(&cont_button),FALSE);
      break;
	case KGenSelectService:
      LGadDestroyButtonList(&service_blist);
      LGadDestroyBox(VB(&cont_button),FALSE);
      break;
	case KGenChooseMission:
      LGadDestroyButtonList(&job_blist);
      LGadDestroyBox(VB(&cont_button),FALSE);
      break;
	case KGenDebriefing:
      LGadDestroyBox(VB(&cont_button),FALSE);
		break;
	case KGenCharSummary:
	case KGenGameSummary:
      LGadDestroyButtonList(&summary_blist);
      LGadDestroyButtonList(&summary_plusminus_blist);
      LGadDestroyBox(VB(&cont_button),FALSE);
		break;
	}
   LGadHideBox(VB(LGadCurrentRoot()),FALSE);
#endif
}

/* ------------------------------------------------------------ */
void CCharGenerator::ChangeState(EGenState newstate)
{
	// WARNING: Don't call this function from inside an interface callback!  It is likely to
	// destroy the very interface elements that triggered it if you do!

	// just take down the old
	DestroyInterface();
	m_state = newstate;
	// and bring in the new
	BuildInterface();
}

/* ------------------------------------------------------------ */
// only call this once for each mission!
void CCharGenerator::ApplyMissionStats(int mission_id)
{
	// poke around in the character class and modify lots of data 
	// based on the data in the mission database
	//printf("Applying mission num %d (type %d)\n",mission_id,jobs_list[m_service][mission_id].jobtype);
	//printf("%s\n",jobs_list[m_service][mission_id].results);
}

/* ------------------------------------------------------------ */
// Interface callbacks
/* ------------------------------------------------------------ */
// this is static
void CCharGenerator::ContinueFunc(void *arg)
{
   gCharGenerator->ContinueGuts();
}

/* ------------------------------------------------------------ */
// note this does all sorts of interface destruction, so if we can't do that from within
// a button callback we will have to trigger this indirectly via a message or somesuch,
// thus the purpose of ContinueFunc, which can be uiDefered
void CCharGenerator::ContinueGuts(void)
{
#if 0
   SFX_Play_Raw(SFX_STATIC, NULL, "BEEP1");
	switch (m_state)
	{
   case KGenSplash:
      //ChangeState(KGenBirth);
      ChangeState(KGenMain);
      break;

   case KGenMain:
      ChangeState(KGenCharSummary);
      break;

   case KGenBirth:
      ChangeState(KGenSelectService);
      break;

	case KGenSelectService:
		// move along to mission selection
		ChangeState(KGenChooseMission);
		break;

	case KGenChooseMission:

		// move along to debriefing
		ChangeState(KGenDebriefing); // hack
		break;

	case KGenDebriefing:
      // apply results
#if 0
      switch (m_year)
      {
      case 0:
         // +3 hvy +1 repair -1 CON
         gChar->m_info[WPN_5] += 3;
         gChar->m_info[TECH_2] += 1;
         gChar->m_info[STAT_3] -= 1;
         gChar->m_pool += 10;
         break;
      case 1:
         // ESP, telepath, proj +3
         gChar->m_info[PSI_4] += 3;
         gChar->m_info[PSI_2] += 3;
         gChar->m_info[PSI_8] += 3;
         gChar->m_pool += 10;
         break;
      case 2:
         // DEX +2, energy + 5
         gChar->m_info[STAT_5] += 2;
         gChar->m_info[WPN_3] += 3;
         gChar->m_pool += 10;
         break;
      case 0:
         // +3 energy, +2 PER, +2 CON
         gChar->m_info[WPN_3] += 3;
         gChar->m_info[STAT_2] += 2;
         gChar->m_info[STAT_3] += 2;
         gChar->m_pool += 10;
         break;
      }
#endif

		// increment our age
		m_year++;
		
      if (m_year == 3) //5)
      {
         ILoop* looper = AppGetObj(ILoop);
         ILoop_EndMode(looper, 0);  // This is to "pop" the loopmode
         SafeRelease(looper);    
         // cd music hack
         CDStopPlay();
      }
      else
         ChangeState(KGenCharSummary);
		break;

   case KGenCharSummary:
		// either go on to exit, or back to mission selection, based on years 
		if (m_year >= NUM_TRAINING_YEARS)
      {
	      // then get on with our lives (ie send a message to go to normal game mode, or pop mode stack)
         ILoop* looper = AppGetObj(ILoop);
         ILoop_EndMode(looper, 0);  // This is to "pop" the loopmode
         SafeRelease(looper);    
      }
		else
      {
         // should be a character summary screen
			ChangeState(KGenChooseMission);
      }
      
      break;
	case KGenGameSummary:
      ILoop* looper = AppGetObj(ILoop);
      ILoop_EndMode(looper, 0);  // This is to "pop" the loopmode
      SafeRelease(looper);    
   }
#endif
}
/* ------------------------------------------------------------ */
void CCharGenerator::ChooseService(int button_num)
{
	m_service = (EService)(button_num);

	// update art area
   DrawServiceText();
}
/* ------------------------------------------------------------ */
void CCharGenerator::ChooseMission(int button_num)
{
	m_mission = job_avail[m_year][button_num];

	// update interface
   DrawJobLongDesc();
}
/* ------------------------------------------------------------ */
void CCharGenerator::ChooseTrait(int button_num)
{
	m_trait = button_num;

	// update interface
   DrawTraitDesc();
}
/* ------------------------------------------------------------ */
void CCharGenerator::ChooseSummary(int button_num, bool redraw)
{
#if 0 
	m_summary_focus = button_num;
   if ((button_num >= WPN_1) && (button_num < PSI_1))
      m_subskills[0] = button_num;
   else if ((button_num >= PSI_1) && (button_num < TECH_1))
      m_subskills[1] = button_num;

   if (redraw)
   {
      ChangeState(m_state);
	   // update interface
      DrawSummaryDesc();

      // if it's a skill, also update the subskill area
      DrawSummarySubskills();
   }
#endif 
}
/* ------------------------------------------------------------ */
void CCharGenerator::PlusMinus(int button)
{
   //m_player->SpendPool(button/2,button%2);
   //ChooseSummary(button/2);

}
/* ------------------------------------------------------------ */
// When the player picks the "start over" button from the exit screen.
// Note that due to state changing we need to trigger this "safely" ie from within a message
// NOT USED IN PROTOTYPE
void CCharGenerator::Restart(void)
{
}
/* ------------------------------------------------------------ */

#endif