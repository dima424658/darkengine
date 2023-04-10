/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkowrld.cpp,v 1.16 1999/08/05 17:57:56 Justin Exp $

#include <math.h>

#include <2d.h>
#include <timer.h>
#include <resapi.h>
#include <appagg.h>
#include <minmax.h>
#include <mprintf.h>
#include <rand.h>
#include <random.h>
#include <config.h>

#include <playrobj.h>
#include <simtime.h>
#include <schema.h>

#include <kbcook.h>
#include <keydefs.h>
#include <uiapp.h>

#include <shkovrly.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkmfddm.h>
#include <shkobjst.h>
#include <shkprop.h>
#include <shkiftul.h>
#include <shkplayr.h>
#include <shkminig.h>
#include <shkinv.h>

#include <filevar.h>

// ui library not C++ ized properly yet 
extern "C" {
#include <event.h>
#include <gadbox.h>
#include <gadblist.h>
#include <gadbutt.h>
}

#include <dbmem.h>

// are we supporting saveload, or not
//#define NO_SAVELOAD

//--------------------------------------------------------------------------------------
// OVERWORLD
// basically, an UltimaI/Nethack crossover clone hack
//--------------------------------------------------------------------------------------

#define SCREEN_TILES_W  11
#define SCREEN_TILES_H  17
#define WORLD_TILES_W   64
#define WORLD_TILES_H   64
#define PLAYER_OFF_X    (((SCREEN_TILES_W+1)/2)-1)
#define PLAYER_OFF_Y    (((SCREEN_TILES_H+1)/2)-1)

//     138 x 274
// 11 x 12 = 132
// 17 x 12 = 204

#define SIZE_TILE_W     12
#define SIZE_TILE_H     12

////////////////
// our layout

static Rect base_rect = {{ 13, 11},{154,292}};

#define TILE_OFFSET_X     (3)
#define TILE_OFFSET_Y     (1)
#define TEXT_AREA_W       (SCREEN_TILES_W*SIZE_TILE_W)

////////////////
// misc useful game defines

#define OW_EMPTY (0xff)

///////////////////////////////
// world construction data
#define NUM_RANGES      (12)
#define NUM_FOREST      (24)
#define NUM_MISC_TREES  (80)
#define NUM_LAKES       (3)
#define NUM_VILLAGES    (8)
#define NUM_CITIES      (6)
#define NUM_TREASURE    (64)
#define NUM_SQUID       (2)

////////////////
// terrain types
typedef struct {
   uchar terrain;     // base terrain - cannot be OW_EMPTY, clearly...
   uchar treasure;    // what loot, man, what loot
   uchar monster_id;  // id (index into gMonsters) of the monster here
   uchar action;      // i think we actually want a separate action list
   uchar loc_id;      // 0-N is city name, 100->10x is monster_camp for that type
   //   uchar pad[3]; // if going to disk, lets hurt memory, not disk
} sWorldMapTile;

#ifdef NO_SAVELOAD

static sWorldMapTile gWorld[WORLD_TILES_W][WORLD_TILES_H];
#define gTile(x,y)       (gWorld[(x)][(y)])
#define gTileTerr(x,y)   (gWorld[(x)][(y)].terrain)
#define gTileMonst(x,y)  (gWorld[(x)][(y)].monster_id)

#else // otherwise, do saveload thing...

typedef struct {
   sWorldMapTile tiles[WORLD_TILES_W][WORLD_TILES_H];
} sWorldTiles;

sFileVarDesc gOWTilesDesc =
{
   kCampaignVar,
   "OW Map",
   "OverWorld World Tiles",
   FILEVAR_TYPE(sWorldTiles),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sWorldTiles,&gOWTilesDesc> gWorldMap;

#define gTile(x,y)      (gWorldMap.tiles[(x)][(y)])
#define gTileTerr(x,y)  (gWorldMap.tiles[(x)][(y)].terrain)
#define gTileMonst(x,y) (gWorldMap.tiles[(x)][(y)].monster_id)
#endif

#define gTilePt(pt)      (gTile((pt)->x,(pt)->y))
#define gTileTerrPt(pt)  (gTileTerr((pt)->x,(pt)->y))
#define gTileMonstPt(pt) (gTileMonst((pt)->x,(pt)->y))

#define TERRAIN_PLAIN       (0)
#define TERRAIN_FOREST      (1)
#define TERRAIN_MOUNTAIN    (2)
#define TERRAIN_WATER       (3)
#define TERRAIN_VILLAGE     (4)
#define TERRAIN_CITY        (5)
#define TERRAIN_ROAD        (6)
#define TERRAIN_SHRINE      (7)
#define TERRAIN_CAMP        (8)
#define TERRAIN_TEMPLE      (9)
#define TERRAIN_NUM_TYPES   (TERRAIN_TEMPLE+1)

#define VILLAGE_HEAL_COST  (15)
#define CITY_HEAL_COST      (5) 

// mobility bits
#define TERR_CAN_WALK    (1<<0)
#define TERR_CAN_CLIMB   (1<<1)
#define TERR_CAN_SWIM    (1<<2)
#define TERR_CAN_FLY     (1<<3)

// for knowing about terrain
typedef struct {
   uchar mobility_bits;
   uchar guard_chance;
   uchar speed_cost;
   uchar world_freq; // out of 255
} sTerrainStats;

static sTerrainStats gTerrStat[]=
{
   { TERR_CAN_WALK|TERR_CAN_FLY,   0,  1, 180 },
   { TERR_CAN_WALK,                0, 43,   1 },
   { TERR_CAN_CLIMB|TERR_CAN_FLY,  0,  1,  14 },
   { TERR_CAN_SWIM|TERR_CAN_FLY,   0,  1,  10 },
   { TERR_CAN_WALK|TERR_CAN_FLY,  20,  1,   2 },
   { TERR_CAN_WALK|TERR_CAN_FLY,  50,  1,   1 },
   { TERR_CAN_WALK|TERR_CAN_FLY,   0,  1,   6 },
   { TERR_CAN_WALK|TERR_CAN_FLY,  99,  1,   0 },
   { TERR_CAN_WALK|TERR_CAN_FLY,   0,  1,   0 },
   { TERR_CAN_WALK|TERR_CAN_FLY,   0,  1,   0 },
};

////////////////
// hit spangs and stuff
#define COMBAT_HIT          (0)
#define COMBAT_MISS         (1)
#define COMBAT_FIREBALL     (2)
#define COMBAT_MISSILE      (3)
#define COMBAT_NUM_TYPES    (COMBAT_MISSILE+1)
#define COMBAT_FIRST_SPELL  (COMBAT_FIREBALL)

////////////////
// loot
#define LOOT_LITTLE         (0)
#define LOOT_LOTS           (1)
#define LOOT_TONS           (2)
#define LOOT_EXOTIC         (3)  // + 10 Armor
#define LOOT_HOD            (4)  // + 10 attack, Hoe Of Destruction
#define LOOT_SHIELD         (5)  // + 4 Armor
#define LOOT_MASAMUNE       (6)  // + 5 attack
#define LOOT_POTION         (7)  // Heals 10 HP
#define LOOT_NUM_TYPES      (LOOT_POTION+1)
#define LOOT_MAX_GOLD       (LOOT_TONS)
#define LOOT_MAX_EQUIP      (LOOT_MASAMUNE)

#define NUM_TREASURE_TYPES 6
#define TT_NONE   0
#define TT_LAME   1
#define TT_MOD    2
#define TT_RICH   3
#define TT_ARTY   4   
#define TT_TEST   5
uchar gTreasureTable[NUM_TREASURE_TYPES][LOOT_NUM_TYPES] = { 
   {  0,  0,  0,  0,  0,  0,  0,  0, },  // no treasure
   { 78, 10,  0,  0,  0,  1,  1, 10, },  // lame treasure
   { 10, 40, 20,  0,  0,  5,  5, 20, },  // moderate treasure
   {  0, 30, 50,  0,  0,  5,  5, 10, },  // gold-rich treasure
   {  0,  0,  0,  0, 10, 30, 30, 30, },  // artifact treasure
   {  0,  0,  0,  0, 33, 33, 33,  1, },  // test
};

// generate location for exotics, perhaps?
#define EXOTIC_ARMOR_VAL    (10)
#define SHIELD_ARMOR_VAL    (4)
#define MASAMUNE_ATTACK     (4)
#define HOD_ATTACK          (10)
#define POTION_HEAL         (10)

////////////////
// monster definitions

typedef struct {
   uchar x, y;
} sOWMapLoc;

#define MAX_MONSTERS 128

typedef struct {
   uchar     type;
   uchar     hp;
   uchar     flags;
   uchar     gp;      
   sOWMapLoc loc;
   sOWMapLoc targ;
} sWorldMonster;

#define MONST_FLG_NONE      (0)     // none
#define MONST_FLG_ATTACK    (1<<0)  // attacking the player
#define MONST_FLG_FLEE      (1<<1)  // fleeing from player
#define MONST_FLG_WANDER    (1<<2)  // just wandering around
#define MONST_FLG_CLOUD     (1<<3)  // cloud around target point....
#define MONST_FLG_GUARD     (1<<4)  // guard a location
#define MONST_FLG_WAS_HIT   (1<<5)  // was attacked this frame...
#define MONST_FLG_DAMAGED   (1<<6)  // was i damaged
#define MONST_FLG_TALK      (1<<7)  // should we just talk

#ifdef NO_SAVELOAD

static sWorldMonster   gRawMonsters[MAX_MONSTERS];
static uchar           gRawMonsterCount;

#define gMonsterCount (gRawMonsterCount)
#define gMonst(i)     (gRawMonsters[i])
#define gMonstType(i) (gRawMonsters[i].type)
#define gMonstLoc(i)  (gRawMonsters[i].loc)

#else // no_saveload

typedef struct {
   uchar         count;
   sWorldMonster dudes[MAX_MONSTERS];
} sSaveMonsters;

sFileVarDesc gOWMonstersDesc =
{
   kCampaignVar,
   "OW Monsters",
   "OverWorld Monsters",
   FILEVAR_TYPE(sSaveMonsters),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sSaveMonsters,&gOWMonstersDesc> gSaveMonst;

#define gMonsterCount (gSaveMonst.count)
#define gMonst(i)     (gSaveMonst.dudes[i])
#define gMonstType(i) (gSaveMonst.dudes[i].type)
#define gMonstLoc(i)  (gSaveMonst.dudes[i].loc)
#endif

#define MONSTER_PLAYER      (0)
#define MONSTER_GOBLIN      (1)
#define MONSTER_HEADLESS    (2)
#define MONSTER_BAT         (3)
#define MONSTER_PIG         (4)
#define MONSTER_TROLL       (5)
#define MONSTER_GHOST       (6)
#define MONSTER_PONDSQUID   (7)
#define MONSTER_DEMON       (8)
#define MONSTER_DRAGON      (9)
#define MONSTER_WISEPERSON  (10)
#define MONSTER_GAZER       (11)
#define MONSTER_REAPER      (12)
#define MONSTER_MIMIC       (13)
#define MONSTER_LURKER      (14)
#define MONSTER_SLIME       (15)
#define MONSTER_MONGBAT     (16)
#define MONSTER_SNICKERS    (17)
#define MONSTER_LB          (18)
#define MONSTER_MERCHANT    (19)
#define MONSTER_NUM_TYPES   (MONSTER_MERCHANT+1)
                                   
#define PLAYER_MONSTER_IDX  (0)
#define DRAGON_MONSTER_IDX  (1)
#define FIRST_REAL_MONSTER_IDX (2)

#define LOC_MONSTER_BASE (100) // for monster camp hacking

// add criticals to this system somehow!!!
typedef struct {
   uchar num_dice;
   uchar die_size;
   uchar flat_add;
   uchar pad;
//   uchar rand_add;
} sDieRoll;  // num_dice die_size dice + flat_add + 1-rand_add 

// for knowing what monsters can do
typedef struct {
   sDieRoll tohit;
   sDieRoll hitpoints;
   sDieRoll damage;
   uchar armor;             // armor is sub'd from damage, and 1/2 from to hit
   uchar xp_val;            // xp to give to player for killing
   uchar level;             // for monster generation reasons, 0 means none
   uchar view_rad;          // how far this crit looks when searching
   uchar treasure_chance;   // x in 100
   uchar treasure_type;     // what to generate when monsters die
   uchar start_flags;       // what flags to set when i am created
   uchar mobility_bits;     // what terrain i can get through
} sMonsterStats;

#define NFlg(loot,mode,terr)   TT_##loot##, MONST_FLG_##mode##, TERR_CAN_##terr
#define NFlg2(loot,mode,t1,t2) TT_##loot##, MONST_FLG_##mode##, TERR_CAN_##t1##|TERR_CAN_##t2

static sMonsterStats gDefaultMonsterStat[]=
{
   { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, 0,  0, 0, 0, 0, NFlg(NONE,NONE,WALK) },   // Player
   { {1,50, 2}, {1, 6, 1}, {1, 4, 0}, 0,  3, 1, 8,25, NFlg(LAME,WANDER,WALK) }, // Goblin
   { {1,50, 8}, {2, 6, 4}, {1, 8, 1}, 1,  7, 2, 4,40, NFlg(MOD, WANDER,WALK) }, // Headless
   { {1,50, 0}, {1, 4, 0}, {1, 3, 0}, 0,  2, 1, 8, 0, NFlg(NONE,WANDER,FLY) },  // Bat
   { {1,50, 4}, {4, 5, 5}, {2, 3, 0}, 4,  9, 2, 6,20, NFlg(LAME,WANDER,WALK) }, // Pig
   { {1,50,17}, {3, 8, 4}, {2, 8, 2}, 2, 13, 3,10,50, NFlg(MOD,WANDER,WALK) },  // Troll
   { {1,50,12}, {2, 8, 8}, {3, 8, 1}, 0, 15, 4,10,25, NFlg2(MOD,WANDER,WALK,FLY) }, // Ghost
   { {1,50,20}, {2, 8, 1}, {3, 8, 1}, 1,  9, 2, 6,15, NFlg(NONE,WANDER,SWIM) }, // PondSquid
   { {1,50,25}, {5, 8, 8}, {4, 8, 1}, 3, 28, 6,12,50, NFlg2(RICH,WANDER,WALK,CLIMB) },  // Demon
   { {1,60,30}, {6,10,10}, {5, 6, 4}, 5, 75,99,18,75, NFlg2(ARTY,WANDER,WALK,FLY) },  // Dragon
   { {1,50,50}, {9,10,10}, {9,10,10}, 0,  0, 0, 0, 0, NFlg(NONE,TALK,WALK) },   // WiseMan
   { {1,50,21}, {6, 6, 1}, {4, 4, 1}, 2, 21, 5,15,15, NFlg(MOD,WANDER,FLY)  },  // Gazer
   { {1,50,24}, {5, 8, 8}, {3,12, 2}, 4, 25, 5,10,25, NFlg(RICH,WANDER,WALK) }, // Reaper
   { {1,50,17}, {4, 8, 4}, {4, 3, 0}, 3, 15, 3, 6,50, NFlg(MOD,GUARD,WALK) },   // Mimic
   { {1,50,17}, {7, 8, 5}, {5, 3, 1}, 4, 19, 4, 6,50, NFlg(MOD,WANDER,WALK) },  // Lurker
   { {1,50,15}, {1, 3, 1}, {1, 2, 1}, 0,  2, 1, 4, 0, NFlg(NONE,WANDER,WALK) }, // Slime
   { {1,50,11}, {2,10, 2}, {3, 3, 2}, 1,  9, 3,12, 5, NFlg(LAME,WANDER,FLY) },  // Mongbat
   { {1,50,50}, {9,10,10}, {9,10,10}, 0,  0, 0, 0, 0, NFlg(NONE,TALK,WALK) },   // Snickers
   { {1,50,50}, {9,10,10}, {9,10,10}, 0,  0, 0, 0, 0, NFlg(NONE,TALK,WALK) },   // LB
   { {1,50,50}, {9,10,10}, {9,10,10}, 0,  0, 0, 0, 0, NFlg(NONE,TALK,WALK) },   // Merchant
};

#ifdef NO_SAVELOAD

static sMonsterStats gRawMonsterStat[MONSTER_NUM_TYPES];
#define gMonsterStat gRawMonsterStat

#else  // do saveload

typedef struct {
   sMonsterStats stats[MONSTER_NUM_TYPES];
} sSaveStats;

sFileVarDesc gOWStatsDesc =
{
   kCampaignVar,
   "OW MStats",
   "OverWorld Monster Stats",
   FILEVAR_TYPE(sSaveStats),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sSaveStats,&gOWStatsDesc> gSaveStats;

#define gMonsterStat gSaveStats.stats
#endif

#define MAX_MONSTER_LEVEL (6) // cheating, really, since dragon is special....

// a note on to-hit rolls
// we roll, then compare for > 40
// so, basically, default d50 means you hit 1 in 5
// by adding a flat_add, one can up it
// so, if you do d50+10, then it is 2 in 5

////////////////
// action queue

typedef struct {
   sOWMapLoc loc;    // where is this action
   ushort time_out;  // when will it be over (in ms for anims, in frames for shots)
   uchar  dx,dy;     // current velocity (0,0 implies this is an anim, not a projectile)
} sAction;

#define MAX_ACTIONS 32

#ifdef NO_SAVELOAD

static sAction gRawActionQueue[MAX_ACTIONS];
#define gActionQueue gRawActionQueue

#else // not no saveload...

typedef struct {
   sAction actions[MAX_ACTIONS];
} sSaveActions;

sFileVarDesc gOWActionsDesc =
{
   kCampaignVar,
   "OW Actions",
   "OverWorld Actions",
   FILEVAR_TYPE(sSaveActions),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sSaveActions,&gOWActionsDesc> gSaveActions;
#define gActionQueue gSaveActions.actions

#endif

////////////////
// player

typedef struct {
   sOWMapLoc loc;               // where are we
   sOWMapLoc exotic;            // location of the exotic armor
   sOWMapLoc temple;            // where to put the temple
   uchar     level;             // current level
   uchar     heal_timer;        // where in the heal_timer phase we are
   uchar     converse;          // conversation state bits
   uchar     potions;           // how many potions do you have
   uchar     quest;             // target of current quest
   uchar     questcount;        // how many quests have you done
   uchar     equipment;         // bit field of loot types gotten
   uchar     drag_ids[4];       // monster_id's of the elemental dragons, in order
   uchar     dragons;           // number of dragons killed (5 means you win!)
   uchar     shrine_time;       // how long have i been praying at this shrine_time
   uchar     casting;           // am i preparing to cast (is next left click a cast?)
   uchar     spell_timer;       // spell timer, can i cast again...
   uchar     death_count;       // for testing while invuln, why not
   uchar     pad[2];
   ushort    max_hp;
   ushort    xp;
   ushort    gp;
   ushort    pad_s;
   uint      turns;
} sPlayer;

// index into the gPlayer.drag_ids[] for the elemental dragons...
#define ELEM_DRAGON_MTN    (0)
#define ELEM_DRAGON_WATER  (1)
#define ELEM_DRAGON_FOREST (2)
#define ELEM_DRAGON_ROAD   (3)
#define NUM_ELEM_DRAGONS   (4)

#define WON_THE_GAME() (gPlayer.dragons==5)

////////////////
// conversation states (from gPlayer.converse)
#define WISEPERSON_SHFT (0)
#define WISEPERSON_MASK (0x3<<WISEPERSON_SHFT)
#define MERCHANT_SHFT   (2)
#define MERCHANT_MASK   (0x3<<MERCHANT_SHFT)
#define SNICKERS_SHFT   (4)
#define SNICKERS_MASK   (0x3<<SNICKERS_SHFT)
#define LORDB_SHFT      (6)
#define LORDB_MASK      (0x3<<LORDB_SHFT)

#define GetConverseState(WHICH)   ((gPlayer.converse&##WHICH##_MASK)>>##WHICH##_SHFT)
#define SetConverseState(WHICH,x) gPlayer.converse=((gPlayer.converse&~##WHICH##_MASK)+((x)<<##WHICH##_SHFT))

////////////////
// overall game state

typedef enum eOWGameMode { kGameInit, kGamePlay, kGameDead, kGameScores };

static int                gLastModeChange = 0;
static int                gLastInputEvent = 0;

#define MAX_CITY_NAMES (NUM_CITIES + NUM_VILLAGES)

#ifdef NO_SAVELOAD

static eOWGameMode        gRawGameMode = kGameInit;
static sPlayer            gRawPlayer;
static char               gRawCityNames[MAX_CITY_NAMES][32];

#define gCurGameMode      gRawGameMode
#define gPlayer           gRawPlayer
#define gCityNames        gRawCityNames

#else // do saveload

// all we need to do is make sure mode is inited to kGameInit on reset of the game
// this is doable w/filevar, i just forgot how...

typedef struct {
   eOWGameMode mode;
   sPlayer     player;
   char        city_names[MAX_CITY_NAMES][32];   
} sSavePlayer;

sFileVarDesc gOWPlayerDesc =
{
   kCampaignVar,
   "OW Player",
   "OverWorld Player",
   FILEVAR_TYPE(sSavePlayer),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sSavePlayer,&gOWPlayerDesc> gSavePlayer;

#define gCurGameMode      gSavePlayer.mode
#define gPlayer           gSavePlayer.player
#define gCityNames        gSavePlayer.city_names

#endif

////////////////
// strings interface
static char        _form_buf[256];
static char        _msg_buf[256];
static char        _monst_buf[64];

#define GetNamedString(buf,name)           ShockStringFetch(buf,sizeof(buf),name,"minigame")
#define GetNamedStringNum(buf,name,num)    ShockStringFetch(buf,sizeof(buf),name,"minigame",num)

// quick loads into the globals to avoid lots of buffer declaration annoyance...
#define GetNamedFormatString(name)         GetNamedString(_form_buf,name)
#define GetNamedFormatStringNum(name,num)  GetNamedStringNum(_form_buf,name,num)
#define GetNamedMessageString(name)        GetNamedString(_msg_buf,name)
static char *mname="ow_monster_XX";
#define set_mstr(x)                 { mname[11]='0'+((x)/10); mname[12]='0'+((x)%10); }
#define GetMonsterName(i)           { set_mstr(i); GetNamedString(_monst_buf,mname); }

// if you use the above to load strings, here is where they are
#define StrFormat() (_form_buf)
#define StrMsg()    (_msg_buf)
#define StrMonst()  (_monst_buf)

// ARGH HATE HATE HATE - cant figure out how to avoid this forward declare
void _ow_add_message_named(char *msg_name);

//////////////////////////////
// save load - hi scores

sFileVarDesc gOverWorldScoreDesc =
{
   kCampaignVar,
   "OWScores",
   "OverWorld High Scores",
   FILEVAR_TYPE(sMiniGameScores),
   {1,0},
   {1,0},
   "shock",
};

cFileVar<sMiniGameScores,&gOverWorldScoreDesc> gOverWorldScoreTable;

// actually generate the sortable score field
static int  _build_score_val(void)
{
   if (WON_THE_GAME())
   {  // player won - score = 1,000,000,000 - (turns * 100) + (lvl * 1000) + (gp * 10)?
      int scr=1000000000-(gPlayer.turns*1000)+(500*gPlayer.level)+(gPlayer.gp*10);
      return scr;
   }
   else
   {
      int scr=(gPlayer.turns*50)+(1000*gPlayer.level)+(gPlayer.gp*10);
      return scr;
   }
}

// compute score for current player, build strings
static void _add_high_score(char *cause)
{
   int scr=_build_score_val();
   char buf[128];
#ifdef DBG_ON
   mprintf("Getting aggregate score %d, %s\n",scr,WON_THE_GAME()?"Won":"Lost");
#endif   
   if (WON_THE_GAME())
   {
      GetNamedFormatString("ow_score_long_win");
      sprintf(buf,StrFormat(),gPlayer.turns,gPlayer.level,gPlayer.gp,gPlayer.xp);
   }
   else
   {
      GetNamedFormatString("ow_score_long_lose");
      sprintf(buf,StrFormat(),cause,gPlayer.turns,gPlayer.level,gPlayer.gp,gPlayer.xp);
   }
   ShockMiniGameAddHighScore(&gOverWorldScoreTable,scr,buf);
}

static void _display_scores(Point pt)
{
   if (WON_THE_GAME())
      pt.y+=48;
   else
      for (int i=1; i<5; i++)
      {
         char buf[64];
         GetNamedStringNum(buf,"ow_tomb_",i);
         gr_font_string(gShockFont,buf,pt.x+25,pt.y);
         pt.y+=12;
      }
   pt.y+=8;  // make a little extra space
   ShockMiniGameDisplayHighScores(&gOverWorldScoreTable,pt.x,pt.y);
}

/////////////////////////
// creation of the world!

#define CLUMP_CIRCLE    (0)
#define CLUMP_LINEAR    (1)
#define CLUMP_TYPES     (CLUMP_LINEAR+1)

#define MAP_X(x)        (((x)+WORLD_TILES_W)%WORLD_TILES_W)
#define MAP_Y(y)        (((y)+WORLD_TILES_H)%WORLD_TILES_H)

#define terr_at(x,y)       (gTileTerr(MAP_X(x),MAP_Y(y)))
#define put_terr(terr,x,y) terr_at(x,y)=terr

// transfrom world point into local coordinate system (rotated by ang, in fact)
// then distort by the "shape" for CLUMP_LINEAR
// as a note the  | m0 m2 |
//   matrix is    | m1 m3 |
static void GenClump(int terr, int x_seed, int y_seed, int rad, int shape_type)
{
   const float ang=RandFloat()*MX_REAL_PI;  // 0 - pi angle
   const float m0=cos(ang), m1=sin(ang);
   const float m2=-m1, m3=m0;
   float x_scale = shape_type==CLUMP_CIRCLE ? 1 : RandFloat()/3.0;

   for (int x=-rad; x<=rad; x++)
      for (int y=-rad; y<=rad; y++)
      {
         float vx=RandFloat()+x, vy=RandFloat()+y;
         float nx=vx*m0+vy*m1, ny=vx*m2+vy*m3;
         nx/=x_scale;
         if (nx*nx+ny*ny<rad*rad)
            put_terr(terr,x_seed+x,y_seed+y);
      }
}

static BOOL IsClearTerr(int x, int y)
{
   int terr=terr_at(x,y);
   return ( terr==TERRAIN_PLAIN || terr==TERRAIN_FOREST );
}

static BOOL IsMountainTerr(int x, int y)
{
   int terr=terr_at(x,y);
   return ( terr==TERRAIN_MOUNTAIN );
}

static void FindAPlaceFor(int type, int *px = NULL, int *py = NULL)
{
   int x,y,lx,ly;
   x=Rand()%WORLD_TILES_W;
   y=Rand()%WORLD_TILES_H;
   for (lx=0; lx<WORLD_TILES_W; lx++)
      for (ly=0; ly<WORLD_TILES_H; ly++)
         if (IsClearTerr(x+lx,y+ly)) // am i a moron, or what
            break;
   x=MAP_X(x+lx);
   y=MAP_Y(y+ly);
   gTileTerr(x,y)=type;
   if (px != NULL) *px = x;
   if (py != NULL) *py = y;
}

static BOOL FindClearRad(int *x_in, int *y_in, int rad, BOOL (*ClearCheck)(int x, int y))
{
   int x, y, lx, ly;
   BOOL ok;
   x=Rand()%WORLD_TILES_W;
   y=Rand()%WORLD_TILES_H;
   for (lx=0; lx<WORLD_TILES_W; lx++)
      for (ly=0; ly<WORLD_TILES_H; ly++)
      {
         ok=TRUE;
         for (int i=-rad; i<=rad && ok; i++)
            for (int j=-rad; j<=rad && ok; j++)
               ok&=(*ClearCheck)(x+lx+i,y+ly+j);
         if (ok)
         {
            *x_in=MAP_X(x+lx);
            *y_in=MAP_Y(y+ly);
            return TRUE;
         }
      }
   Warning(("FindClearRad failed rad %d callback %x (%d)\n",rad,ClearCheck,ClearCheck==IsClearTerr));
   *x_in=x; *y_in=y; return FALSE;  // well, gotta return something, i guess
}

// place a shrine surrounded by ring_terr with a road leading in
static void PlaceShrine(int ring_terr)
{
   int x,y,dx,dy;
   if (!FindClearRad(&x,&y,2,IsClearTerr))
      if (!FindClearRad(&x,&y,1,IsClearTerr))      
      {  // oh well, no space for cool shrine ring thing...
         FindAPlaceFor(TERRAIN_SHRINE);
         return;
      }
   for (int i=-1; i<=1; i++)
      for (int j=-1; j<=1; j++)
         put_terr(ring_terr,x+i,y+j);
   dx=dy=0;
   if (Rand()&1)
      dx=((Rand()&1)*2)-1;
   else
      dy=((Rand()&1)*2)-1;
   put_terr(TERRAIN_ROAD,x+dx,y+dy);
   put_terr(TERRAIN_ROAD,x+2*dx,y+2*dy);
   put_terr(TERRAIN_SHRINE,x,y);
}

static void RandomFantasyName(char *pStr, int len)
{
   static char *consonants = "bcdfghjklmnpqrstvwxzzk";
   static char *vowels = "aeiouy'aey";
   for (int i=0; i < len; i++)
   {
      char *p=(i&0x1)?vowels:consonants; // choose cons or vowel based on parity
      pStr[i] = p[Rand()%strlen(p)];
   }
   pStr[0] = toupper(pStr[0]);
   pStr[len] = '\0';
}

static int nameCity(int len, int *namecount)
{
   int usename=(*namecount)++;
   RandomFantasyName(gCityNames[usename],len);
   return usename;
}

static void SimGenerateWorld(void)
{
   int i,x,y;
   int namecount = 0;
   
   // clear the world to all plains
   for (x=0; x < WORLD_TILES_W; x++)
      for (y=0; y < WORLD_TILES_H; y++)
      {
         gTile(x,y).terrain    = TERRAIN_PLAIN;
         gTile(x,y).treasure   = OW_EMPTY;
         gTile(x,y).monster_id = OW_EMPTY;
         gTile(x,y).action     = OW_EMPTY;
         gTile(x,y).loc_id     = OW_EMPTY;
      }

   // NOTE: this ordering is important
   // so that later steps of "creation" overwrite earlier
   // ie. we generates trees early, so they dont end up in the middle of lakes
   //     since creation of the lake will step on the tree, which is what we want

   // create some forests
   for (i=0; i<NUM_FOREST; i++)
      GenClump(TERRAIN_FOREST,Rand()%WORLD_TILES_W,Rand()%WORLD_TILES_H,2+Rand()%5,CLUMP_CIRCLE);

   // some misc trees
   for (i=0; i<NUM_MISC_TREES; i++)
      FindAPlaceFor(TERRAIN_FOREST);      
   
   // create a few mountain ranges   
   for (i=0; i<NUM_RANGES; i++)
      GenClump(TERRAIN_MOUNTAIN,Rand()%WORLD_TILES_W,Rand()%WORLD_TILES_H,4+Rand()%6,CLUMP_LINEAR);

   // create some lakes, some rivers
   for (i=0; i<NUM_LAKES; i++)
      GenClump(TERRAIN_WATER,Rand()%WORLD_TILES_W,Rand()%WORLD_TILES_H,3+Rand()%3,Rand()%CLUMP_TYPES);
   
   // create some cities, with some attached roads...
   for (i=0; i<(NUM_CITIES/2); i++)
   {
      x=Rand()%WORLD_TILES_W;
      y=Rand()%WORLD_TILES_H;
      gTileTerr(x,y)=TERRAIN_CITY;
      gTile(x,y).loc_id = nameCity((Rand()%4) + 4,&namecount);
      int dx=(Rand()%3)-1, dy=(Rand()%3)-1;
      if ((dx|dy)==0) dx=1;
      // now build a road to the next city
      for (int road_len=9+(Rand()%18); road_len>0; road_len--)
      {
         x=MAP_X(x+dx);
         y=MAP_Y(y+dy);
         gTileTerr(x,y)=TERRAIN_ROAD;
      }
      x=MAP_X(x+dx);
      y=MAP_Y(y+dy);
      gTileTerr(x,y)    = TERRAIN_CITY;
      gTile(x,y).loc_id = nameCity((Rand()%4) + 4, &namecount);
   }

   // any other roads?
   
   // and some villages peppered in
   for (i=0; i<NUM_VILLAGES; i++)
   {
      FindAPlaceFor(TERRAIN_VILLAGE,&x,&y);
      gTile(x,y).loc_id = nameCity((Rand()%2)+3, &namecount);
   }
      
   // now place treasure
   for (i=0; i<NUM_TREASURE; i++)
   {
      do {
         x=Rand()%WORLD_TILES_W;
         y=Rand()%WORLD_TILES_H;
      } while ((gTerrStat[gTileTerr(x,y)].mobility_bits&
                gMonsterStat[MONSTER_PLAYER].mobility_bits)==0);
      gTile(x,y).treasure=0;
      if ((Rand()&0x7)==0)
      {
         gTile(x,y).treasure++;     // go to size 1
         if ((Rand()&0xf)==0)
            gTile(x,y).treasure++;  // all the way up to size 2, woo woo           
      }
   }

   // and shrines - one ringed with mountain, one with water
   // note that we sorta "know" about these later in ChangeWorld_n_SummonLB or whatever it is
   PlaceShrine(TERRAIN_MOUNTAIN);
   PlaceShrine(TERRAIN_WATER);   
}

static BOOL _test_off_screen(uchar x, uchar y)
{
   return ((abs(x-gPlayer.loc.x)>PLAYER_OFF_X)||
           (abs(y-gPlayer.loc.y)>PLAYER_OFF_Y));
}

static BOOL _cant_place_monster(int x, int y, int move_bits)
{
   return ((gTileMonst(x,y)!=OW_EMPTY)||
           ((move_bits!=0)&&
            ((gTerrStat[gTileTerr(x,y)].mobility_bits&move_bits)==0)));
}

static void FindSafeSquare(BOOL off_screen, int move_bits, int terr, uchar *x, uchar *y)
{
   uchar x_try, y_try;
   do {
      x_try=Rand()%WORLD_TILES_W;
      y_try=Rand()%WORLD_TILES_H;
   } while (((terr!=OW_EMPTY)&&terr_at(x_try,y_try)!=terr)||
            _cant_place_monster(x_try,y_try,move_bits)||
            (off_screen&&!_test_off_screen(x_try,y_try)));
   *x=x_try;
   *y=y_try;
}

//////////////////////////////
// roll the dice
static int  do_die_roll(sDieRoll *roll)
{
   int res=roll->flat_add;
   for (int i=0; i<roll->num_dice; i++)
      res+=(Rand()%((int)roll->die_size))+1;
   return res;    // add Rand()%roll->rand_add if we add that back in
}

////////////////////
// monster creation code

static int  GetFreeMonsterRecord(void)
{
   for (int i=FIRST_REAL_MONSTER_IDX; i<MAX_MONSTERS; i++)
      if (gMonstType(i)==OW_EMPTY)
         return i;
   return -1;
}

static int BuildAMonster(int i, int type, uchar x, uchar y)
{
   if ((i==-1)||(i==OW_EMPTY))
      i=GetFreeMonsterRecord();
   if (i==-1)
   {
      Warning(("No monster space to build in\n"));
      return i;
   }
   gMonstType(i)=type;
   sMonsterStats *our_stats=&gMonsterStat[gMonstType(i)];
   sWorldMonster *us=&gMonst(i);
   us->hp=do_die_roll(&our_stats->hitpoints);
   us->loc.x=us->targ.x=x;  // both loc and targ at birthplace, for now
   us->loc.y=us->targ.y=y;  // because, hey, why not...
   us->flags=our_stats->start_flags;
   gTileMonstPt(&gMonstLoc(i))=i;
   gMonsterCount++;
#ifdef PLAYTEST
   if (config_is_defined("overworld_monsters"))
      mprintf("Monster %d, slot %d, a %d, built at %d %d\n",gMonsterCount,i,type,x,y);
#endif
   return i;
}

static int ChooseMonsterType(void)
{
   int targ_lvl=gPlayer.level/2;
   sDieRoll monst_gen={6,2,0};  // 6-12 - so we will -9 on the next line... 
   int roll=do_die_roll(&monst_gen)+targ_lvl-9;
   if (roll<1) roll=1; else if (roll>MAX_MONSTER_LEVEL) roll=MAX_MONSTER_LEVEL;
   // now find all monsters of this level...
   int count=0, match_monst[MONSTER_NUM_TYPES];
   for (int i=0; i<MONSTER_NUM_TYPES; i++)
      if (gMonsterStat[i].level==roll)
         match_monst[count++]=i;
   return match_monst[Rand()%count];
}

static int CreateAMonster(BOOL off_screen, int idx, int type)
{  
   if (type==OW_EMPTY)
      type=ChooseMonsterType();
   uchar x,y;
   FindSafeSquare(off_screen,gMonsterStat[type].mobility_bits,OW_EMPTY,&x,&y);
   return BuildAMonster(idx,type,x,y);
}

static void CreateTheDragon(void)
{  // look for some mountains in which to put the dragon
   int x,y;
   if (!FindClearRad(&x,&y,2,IsMountainTerr))
      if (!FindClearRad(&x,&y,1,IsMountainTerr))
         Warning(("Cannot find a place for the Dragon!\n"));
   BuildAMonster(DRAGON_MONSTER_IDX,MONSTER_DRAGON,x,y);
   gMonst(DRAGON_MONSTER_IDX).flags=MONST_FLG_GUARD;  // the dragon
}

static BOOL FindClearNearTerr(uchar *x, uchar *y, uchar terr, BOOL plain_only, BOOL off_screen)
{
   int count=0;
   do {
      FindSafeSquare(TRUE,TERR_CAN_WALK,plain_only?TERRAIN_PLAIN:OW_EMPTY,x,y);
      if (!off_screen || _test_off_screen(*x,*y))
         for (int i=-1; i<=1; i++)       // look within 1
            for (int j=-1; j<=1; j++)    // for a tile matching terr
               if ((i|j)==0)
                  continue;
               else if (terr_at((*x)+i,(*y)+j)==terr)
                  return TRUE;
   } while (count++<256);  // really, should be a loop
   Warning(("Couldnt find ClearNearTerr (%d %d)\n",terr,off_screen));
   return FALSE;
}

#ifdef DBG_ON
static BOOL SanityCheckTheWorld(char *buf)
{
   static BOOL gone_bad=FALSE;
   BOOL this_ok=TRUE;

   if (!config_is_defined("overworld_sanity"))
      return FALSE;
   
   for (int x=0; x<WORLD_TILES_W; x++)
      for (int y=0; y<WORLD_TILES_H; y++)
      {
         sWorldMapTile *us=&gTile(x,y);
         if ((us->terrain>=TERRAIN_NUM_TYPES)||
             ((us->treasure!=OW_EMPTY)&&(us->treasure>=LOOT_NUM_TYPES))||
             ((us->monster_id!=OW_EMPTY)&&
              ((us->monster_id>=MAX_MONSTERS)||(gMonstType(us->monster_id)>MONSTER_NUM_TYPES))))
         {
            if (!gone_bad)
               mprintf("BAD %s: %d %d: terr %d loot %d m_id %d type %d (ply %d %d)\n",
                       buf?buf:"Somewhere",x,y,us->terrain,us->treasure,us->monster_id,
                       gMonstType(us->monster_id),gPlayer.loc.x,gPlayer.loc.y);
            this_ok=FALSE;
            gone_bad=TRUE;
         }
      }
   if (this_ok)
      gone_bad=FALSE;
   return gone_bad;
}
#else
#define SanityCheckTheWorld(buf) (FALSE)
#endif

static void CreateTalkingMonster(int type, int terr)
{
   uchar x,y;
   int idx;
   FindClearNearTerr(&x,&y,terr,FALSE,TRUE);
   idx=BuildAMonster(-1,type,x,y);
   if (idx!=OW_EMPTY)  // if we got a valid monster, set it to talk
      gMonst(idx).flags=MONST_FLG_TALK;
}

static void SummonLB_n_DestroyShrines(void)
{
   int sc=0;
   for (int x=0; x<WORLD_TILES_W; x++)
      for (int y=0; y<WORLD_TILES_H; y++)
         if (gTileTerr(x,y)==TERRAIN_SHRINE)
         {  // since 7 of 8 around should be our element, just check 2 looking for water
            BOOL water_shrine=((gTileTerr(MAP_X(x+1),MAP_Y(y+1))==TERRAIN_WATER)||
                               (gTileTerr(MAP_X(x),MAP_Y(y+1))==TERRAIN_WATER));
            gTileTerr(x,y)=TERRAIN_PLAIN;
            // put LB at mountain shrine - Temple at water shrine
            if (water_shrine)
            {
               gPlayer.temple.x=x;
               gPlayer.temple.y=y;
            }
            else
            {
               int idx=BuildAMonster(-1,MONSTER_LB,x,y);
               if (idx!=OW_EMPTY)  // if we got a valid monster, set it to talk
                  gMonst(idx).flags=MONST_FLG_TALK;
            }
            if (++sc==2) // number of shrines
               return;
         }
}

static void CreateExotics(void)
{
   FindClearNearTerr(&gPlayer.exotic.x,&gPlayer.exotic.y,TERRAIN_WATER,FALSE,FALSE);
}

// note: player is secretly always monster 0, dragon monster 1, wiseperson monster 2
static void CreateMonsters(void)
{
   int i, j;
   gMonsterCount=0;
   for (i=FIRST_REAL_MONSTER_IDX; i<MAX_MONSTERS; i++)
      if ((Rand()&0xff)<0x53) // for now, 1 in 3 chance of creating
         CreateAMonster(FALSE,i,OW_EMPTY);
      else
         gMonstType(i)=OW_EMPTY;
   for (i=FIRST_REAL_MONSTER_IDX, j=0; i<MAX_MONSTERS && j<NUM_SQUID; i++)
      if (gMonstType(i)==OW_EMPTY)
      {  // @TBD: dont want them next to a shrine, sadly... have to deal
         CreateAMonster(FALSE,i,MONSTER_PONDSQUID);
         j++;
      }
   CreateTheDragon();
}

// dont replace a greater treasure with a lesser
void PlaceTreasure(int which, int x, int y)
{
#ifdef DBG_ON   
   if (which>=LOOT_NUM_TYPES)
   {
      mprintf("Hey!  trying to put %d at %d %d\n",which,x,y);
      return;
   }
#endif
   if ((gMonsterStat[MONSTER_PLAYER].mobility_bits&gTerrStat[gTileTerr(x,y)].mobility_bits)!=0)
      if ((gTile(x,y).treasure==OW_EMPTY)||(gTile(x,y).treasure<=which))
         gTile(x,y).treasure=which;
}

BOOL FindSurroundingGoodPlace(int move_bits, int *nx, int *ny)
{
   int x=*nx,y=*ny;
   sOWMapLoc worked[9];
   int work_cnt=0;
   for (int i=-1; i<=1; i++)
      for (int j=-1; j<=1; j++)
         if ((i|j)!=0)
            if (!_cant_place_monster(MAP_X(x+i),MAP_Y(y+j),move_bits))
            {
               worked[work_cnt].x=MAP_X(x+i);
               worked[work_cnt].y=MAP_Y(y+j);
               work_cnt++;
            }
   if (work_cnt)
   {  // only split if we are sure we can build the second...
      int use=Rand()%work_cnt;
      *nx=worked[use].x;
      *ny=worked[use].y;
      return TRUE;
   }
   return FALSE;
}

// and monster removal, too
static void RemoveAMonster(int id, BOOL gen_treasure)
{
   int type=gMonstType(id);
   sMonsterStats *us=&gMonsterStat[gMonstType(id)];
#ifdef PLAYTEST
   if (config_is_defined("overworld_monsters"))
      mprintf("Monster %d, a %d, removed at %d %d\n",id,gMonstType(id),
              gMonstLoc(id).x,gMonstLoc(id).y);
#endif   
   gMonstType(id)=OW_EMPTY;
   gTileMonstPt(&gMonstLoc(id))=OW_EMPTY;
   
   if (gen_treasure&&((Rand()%100)<us->treasure_chance))
      if (us->treasure_type > 0)
      {  // okay, pick a kind of treasure from our treasure table
         int lootkind, chance, sum=0, randval = Rand()%100;
         for (lootkind = 0; lootkind < LOOT_NUM_TYPES; lootkind++)
         {
            chance = gTreasureTable[us->treasure_type][lootkind];
            sum = sum + chance;
            if (randval < sum)
               break;
         }
         if (lootkind >= LOOT_NUM_TYPES)
            Warning(("Bad treasure generation, %d, type %d, roll %d!\n",lootkind,us->treasure_type, randval));
         else
         {
            // if we already have this kind of unique treasure, then give gold instead
            if ((lootkind >= LOOT_EXOTIC) && (lootkind < LOOT_POTION))
               if (gPlayer.equipment & (1 << lootkind))
                  lootkind = LOOT_LOTS;
            PlaceTreasure(lootkind,gMonstLoc(id).x,gMonstLoc(id).y);
         }
      }
   gMonsterCount--;

   switch (type)
   {  // here we have to deal with "death callbacks", ummm, as they were...
      case MONSTER_DRAGON:
         if (++gPlayer.dragons==1)
         {  // summon LB, maybe
            _ow_add_message_named("ow_dragondead"); // first one, move on
            SummonLB_n_DestroyShrines();
            // and pump up the dragons, too
            sMonsterStats *drag=&gMonsterStat[MONSTER_DRAGON];
            drag->tohit.die_size+=10;
            drag->tohit.flat_add+=4;
            drag->hitpoints.num_dice+=3;
            drag->hitpoints.die_size+=2;
            drag->damage.num_dice+=1;
            drag->damage.die_size+=2;
            drag->damage.flat_add+=4;
            drag->armor+=5;
            drag->xp_val+=75;
         }
         else
         {
            for (int i=0; i<NUM_ELEM_DRAGONS; i++)
               if (gPlayer.drag_ids[i]==id)
                  gPlayer.drag_ids[i]=0;  // for these guys, 0 is none
            if (gPlayer.dragons==5)
            {
               _ow_add_message_named("ow_win");
               _add_high_score(NULL);
               gLastModeChange=GetSimTime();
               gCurGameMode=kGameDead;    // well, win, but dead, game over sort of way...
               ShockInvPayNanites(-20);   // give the player 20 nanites for winning
            }
         }
         break;
      case MONSTER_SLIME:
      {
         int chance=(gPlayer.level>20?40:20+gPlayer.level);
         if ((Rand()%100)<chance)
         {  // spawn two slime nearby... need to find 2 locations... one will be us
            uchar x=gMonstLoc(id).x, y=gMonstLoc(id).y;
            uchar move_bits=gMonsterStat[MONSTER_SLIME].mobility_bits;
            int nx=x, ny=y;
            if (FindSurroundingGoodPlace(move_bits,&nx,&ny))
            {
               int idx;
               idx=BuildAMonster(-1,MONSTER_SLIME,nx,ny);
               if (idx!=-1) gMonst(idx).flags=MONST_FLG_ATTACK;
               idx=BuildAMonster(-1,MONSTER_SLIME,x,y);
               if (idx!=-1) gMonst(idx).flags=MONST_FLG_ATTACK;               
            }
         }
         break;
      }
   }
}

static void SetupMonsterCampMonster(int idx, int mode, float hp_fac, int x, int y)
{
   if (idx!=-1)
   {
      gMonst(idx).flags=mode|MONST_FLG_CLOUD;
      gMonst(idx).hp=(int)(gMonst(idx).hp*hp_fac);
      gMonst(idx).targ.x=x;
      gMonst(idx).targ.y=y;      
   }
}

// lets rock - create village of type X's near terr Y - for now looks for a 1x1, but hey..
static void CreateMonsterCamp(int type, int terr)
{
   uchar x,y;
   if (!FindClearNearTerr(&x,&y,terr,TRUE,TRUE))
      if (!FindClearNearTerr(&x,&y,terr,TRUE,FALSE))
         FindSafeSquare(TRUE,0,OW_EMPTY,&x,&y);  // give up and just look for anything!
   sWorldMapTile *us=&gTile(x,y);
   if ((us->terrain!=TERRAIN_PLAIN)&&(us->terrain!=TERRAIN_FOREST))
   {  // ummm, nowhere to put monster camp?
      Warning(("Can't place monster camp?\n"));
      return;  
   }
   us->terrain=TERRAIN_CAMP;
   us->loc_id=LOC_MONSTER_BASE+type;  // set our camp type...
   if (us->monster_id!=OW_EMPTY)
   {  // want to make sure no monster
      if ((us->monster_id>=FIRST_REAL_MONSTER_IDX)&&
          (gMonstType(us->monster_id)!=MONSTER_DRAGON)&&
          (gMonstType(us->monster_id)!=MONSTER_PLAYER)&&
          ((gMonst(us->monster_id).flags&MONST_FLG_TALK)==0))
         RemoveAMonster(us->monster_id,FALSE);
   }

   int idx;
   if (us->monster_id==OW_EMPTY)
   {
      idx=BuildAMonster(-1,type,x,y);
      SetupMonsterCampMonster(idx,MONST_FLG_GUARD,2.0,x,y);
   }
   uchar move_bits=gMonsterStat[type].mobility_bits;
   int xtra_monst=2+(gMonsterStat[type].level/3); // sure, who knows...
   for (int j=0; j<xtra_monst; j++)
   {
      int nx=x,ny=y;
      if (FindSurroundingGoodPlace(move_bits,&nx,&ny))
      {
         int idx=BuildAMonster(-1,type,nx,ny);
         SetupMonsterCampMonster(idx,MONST_FLG_GUARD,1.5,x,y);
      }
   }
}

#define XP_BASE_LVL (16)
#define XP_MUL_LVL   (8)
#define XP_MUL_2     (9)
#define XP_MUL_3    (10)
// it is an interative func, i think - perhaps too steep, not sure
static int GetLevelFromXP(int xp)
{
   int i=1, txp=XP_BASE_LVL;
   while (txp<=xp)
   {
      txp+=XP_BASE_LVL+(XP_MUL_LVL*i);
      if (i>6)  txp+=XP_MUL_2*(i-6);   // past 5th, steepen it more
      if (i>14) txp+=XP_MUL_3*(i-14);  // past 12th, steepen it some more
      i++;
   }
   return i;
}

// heal the player either hp hit points, or percent percent of max + 1
void heal_the_player(int hp, int percent)
{
   int use_hp=gMonst(PLAYER_MONSTER_IDX).hp;
   if (hp)
      use_hp+=hp;
   else  // heal by percent
   {
      int hpval=(((int)gPlayer.max_hp)*percent/100);
      if (hpval<1) hpval=1;
      use_hp+=hpval;
   }
   if (use_hp>gPlayer.max_hp)
      use_hp=gPlayer.max_hp;
   gMonst(PLAYER_MONSTER_IDX).hp=use_hp;
}

// initialize gPlayer, place the player, setup combat values, etc...
static void CreatePlayer(void)
{
   sDieRoll def_damage={2,4,1}, def_tohit={1,50,15};
   memset(&gPlayer,0,sizeof(gPlayer));
   gPlayer.level      = 1;
   gPlayer.max_hp     = 5+(Rand()%6);
   FindSafeSquare(FALSE,gMonsterStat[MONSTER_PLAYER].mobility_bits,OW_EMPTY,&gPlayer.loc.x,&gPlayer.loc.y);
   gPlayer.quest      = OW_EMPTY;
   if (ShockMiniGameIsHacked())
      gPlayer.potions    = 1;
   gTileMonstPt(&gPlayer.loc)=PLAYER_MONSTER_IDX;
   gMonst(PLAYER_MONSTER_IDX).type  = MONSTER_PLAYER;
   gMonst(PLAYER_MONSTER_IDX).hp    = gPlayer.max_hp;
   gMonsterStat[MONSTER_PLAYER].damage = def_damage;
   gMonsterStat[MONSTER_PLAYER].tohit  = def_tohit;
}

// build the world...
void CreateTheGameSpace(void)
{
   // do horrible monster stat hack
   memcpy(gMonsterStat,gDefaultMonsterStat,sizeof(gMonsterStat));
   SimGenerateWorld();
   CreatePlayer();
   CreateMonsters();
   CreateMonsterCamp(MONSTER_GOBLIN,TERRAIN_PLAIN);  // 2 goblin camps at start....
   CreateMonsterCamp(MONSTER_GOBLIN,TERRAIN_PLAIN);   
   CreateExotics();          // figure out where they will be
   gCurGameMode = kGamePlay;
}

//////////////////////////
// resources for drawing

#define MONSTER_FRAME_MASK  (1)
#define MONSTER_FRAME_SPEED (8)
#define MONSTER_FRAMES      (1<<MONSTER_FRAME_MASK)

IRes *gResMonster[MONSTER_NUM_TYPES][MONSTER_FRAMES];
IRes *gResTerrain[TERRAIN_NUM_TYPES];
IRes *gResAction[COMBAT_NUM_TYPES];
IRes *gResLoot[LOOT_NUM_TYPES];

////////////////
// actually draw a tile

static IRes *getMonsterFrame(int monster)
{
   int tm=tm_get_millisec_unrecorded();
   int fr=(tm>>MONSTER_FRAME_SPEED)&MONSTER_FRAME_MASK;
   return gResMonster[monster][fr];
}

static void _ow_draw_a_tile(Point scrpos, int tile_x, int tile_y)
{
   sWorldMapTile *us=&gTile(tile_x,tile_y);

   // @TBD: id like to just pull this, really
#define SAFETY_PUP
#ifdef SAFETY_PUP
   if ((us->terrain>=TERRAIN_NUM_TYPES)||
       ((us->treasure!=OW_EMPTY)&&(us->treasure>=LOOT_NUM_TYPES))||
       ((us->monster_id!=OW_EMPTY)&&
        ((us->monster_id>=MAX_MONSTERS)||(gMonstType(us->monster_id)>MONSTER_NUM_TYPES))))
   {
      Warning(("BAD TILE: %d %d: terr %d loot %d m_id %d type %d\n",
              tile_x,tile_y,us->terrain,us->treasure,us->monster_id,gMonstType(us->monster_id)));
      return;
   }
#endif   

   // first the terrain
   DrawByHandle(gResTerrain[us->terrain],scrpos);

   // if our terrain is a city or village, draw the name above it
   // if ((us->terrain == TERRAIN_CITY) || (us->terrain == TERRAIN_VILLAGE))
   //    gr_font_string(gShockFont, gCityNames[us->loc_id], scrpos.x, scrpos.y - 14);

   if (us->treasure!=OW_EMPTY)
      DrawByHandle(gResLoot[us->treasure],scrpos);

   // any monsters or anything....
   if (us->monster_id!=OW_EMPTY)
      DrawByHandle(getMonsterFrame(gMonstType(us->monster_id)),scrpos);

   // action
   if (us->action!=OW_EMPTY)
      DrawByHandle(gResAction[us->action],scrpos);
}

////////////////
// status line manager

#define NUM_STATUS_LINES       (4)
#define MAX_STATUS_LINE_LEN   (64)
#define STATUS_DURATION_MS  (5000)
static char status_lines[NUM_STATUS_LINES][MAX_STATUS_LINE_LEN];
static int  status_cur_line[NUM_STATUS_LINES];  // time to expire this line
static BOOL status_drawn=FALSE;

static void _ow_purge_message(int which)
{
   int i;
   for (i=which+1; i<NUM_STATUS_LINES; i++)
   {
      memcpy(status_lines[i-1],status_lines[i],MAX_STATUS_LINE_LEN);
      status_cur_line[i-1]=status_cur_line[i];
   }
   status_cur_line[i-1]=0;
}

static int  _ow_get_msg_slot(void)
{
   if (status_cur_line[NUM_STATUS_LINES-1]!=0)
   {
      _ow_purge_message(0);
      return NUM_STATUS_LINES-1;
   }
   else
      for (int i=NUM_STATUS_LINES-2; i>=0; i--)
         if (status_cur_line[i]!=0)
            return i+1;
   return 0;
}

// pass TRUE to purge oldest, else simply expire if necessary
static void _ow_update_messages(void)
{
   if (status_cur_line[0]<GetSimTime())
      _ow_purge_message(0);
}

// ok... this has to secretly word wrap and break up the message
static void _ow_add_message(char *msg)
{
   char buf[4*MAX_STATUS_LINE_LEN+10]; // pad for \n's, i think

   if (strlen(msg)>sizeof(buf))
      Warning(("Incoming String tooo long (%s) (%d > %d)\n",msg,strlen(msg),sizeof(buf)));
   
   strncpy(buf,msg,4*MAX_STATUS_LINE_LEN-1);
   buf[4*MAX_STATUS_LINE_LEN-1]='\0';
   gr_font_string_wrap(gShockFont,buf,TEXT_AREA_W); // ??

   char *s=buf, *p;
   int next_slot;
   do {
      next_slot=_ow_get_msg_slot();
      //      p=strchr(s,'\n');  // fix this to be the right thing, duh!
      p=strchr(s,1);  // Har Har
      if (p!=NULL)
      {
         *p='\0';
         strncpy(status_lines[next_slot],s,MAX_STATUS_LINE_LEN-1);
         status_lines[next_slot][MAX_STATUS_LINE_LEN-1]='\0';
         status_cur_line[next_slot]=GetSimTime()+STATUS_DURATION_MS;
         s=p+1;
      }
   } while (p!=NULL);
   if (*s!='\0')
   {
      strncpy(status_lines[next_slot],s,MAX_STATUS_LINE_LEN-1);
      status_lines[next_slot][MAX_STATUS_LINE_LEN-1]='\0';
      status_cur_line[next_slot]=GetSimTime()+STATUS_DURATION_MS;
   }
}

static void _ow_add_message_named(char *str_name)
{
   char msg[128];
   GetNamedString(msg,str_name);
   _ow_add_message(msg);
}

static void _ow_add_message_named(char *str_name, int num)
{
   char msg[128];
   GetNamedStringNum(msg,str_name,num);
   _ow_add_message(msg);
}

////////////////
// action queue support

static int _get_free_action(void)
{
   for (int i=0; i<MAX_ACTIONS; i++)
      if (gActionQueue[i].time_out==0)
         return i;
   return -1;
}

int _ow_add_action(int type, sOWMapLoc *loc, int time_out)
{
   int idx=_get_free_action();
   if (idx==-1)
   {
      Warning(("Out of actions\n"));
      return idx;
   }
   gActionQueue[idx].loc=*loc;
   gActionQueue[idx].time_out=time_out;
   gActionQueue[idx].dx=gActionQueue[idx].dy=0;
   gTilePt(loc).action=type;
   return idx;
}

#define MS_PER_VEL_MOVE 400

// hatefully, i forward declare this, cause i sucked when i built it, so sue me
BOOL UpdateVelAction(BOOL create, int type, sOWMapLoc *loc, int dx, int dy);

void _ow_add_vel_action(int type, sOWMapLoc *loc, int frames, int dx, int dy)
{
   int idx=_ow_add_action(type,loc,frames*MS_PER_VEL_MOVE);
   gActionQueue[idx].dx=dx;
   gActionQueue[idx].dy=dy;
}

void OW_RunActions(void)
{
   for (int i=0; i<MAX_ACTIONS; i++)
   {
      sAction *act=&gActionQueue[i];
      BOOL act_frame=FALSE;
      if (act->time_out)
         if (GetSimFrameTime()>=act->time_out)
         {
            act->time_out  = 0;
            gTilePt(&act->loc).action=OW_EMPTY;
         }
         else
         {
            int old_frac=act->time_out/MS_PER_VEL_MOVE;
            act->time_out -= GetSimFrameTime();
            act_frame=((act->time_out/MS_PER_VEL_MOVE)!=old_frac);
         }
      if (act_frame && ((act->dx|act->dy)!=0))
         if (!UpdateVelAction(FALSE,gTilePt(&act->loc).action,&act->loc,act->dx,act->dy))
            act->time_out=0;
   }
}

////////////////
// draw functions

#define TEXT_X 3
#define TEXT_Y 4

#define TEXT_CHAR_H 9
#define TEXT_CHAR_W 6

#define TILE_X    1
#define TILE_Y    41

#define STATS_LINES 2

////////////////
// draw recent messages

#define POTION_X  ((SIZE_TILE_W * SCREEN_TILES_W) - 1)
#define POTION_Y  (TEXT_Y+(SIZE_TILE_H*SCREEN_TILES_H))

static Rect potion_rect = {{POTION_X, POTION_Y},{POTION_X + 12, POTION_Y + 12}};
static Rect spell_rect  = {{POTION_X, POTION_Y+12},{POTION_X + 12, POTION_Y + 24}};

static void _ow_draw_character_stats()
{
   char draw_buf[64];
   Point pt;
   pt.x = pt.y = 0;

   // draw potions
   Point drawpt;
   drawpt.x = POTION_X;
   drawpt.y = POTION_Y;
   DrawByHandle(gResLoot[LOOT_POTION], drawpt);

   if (ShockMiniGameIsHacked()&&gPlayer.spell_timer==0)
   {
      drawpt.y+=12;
      if (gPlayer.casting)
         drawpt.x-=12;    // move it over as some sort of feedback if you click on it...
      DrawByHandle(gResAction[COMBAT_MISSILE], drawpt);
   }

   sprintf(draw_buf,"%d",gPlayer.potions);
   gr_font_string(gShockFont, draw_buf, POTION_X - 8, POTION_Y);

   pt.x=TEXT_X;
   pt.y=TEXT_Y+(SIZE_TILE_H*SCREEN_TILES_H);

   // @TBD: apparently if HP>100 this can overlap next field!!
   if (gMonst(PLAYER_MONSTER_IDX).hp>100)
      GetNamedFormatString("ow_stats_hp_short");
   else
      GetNamedFormatString("ow_stats_hp");
   sprintf(draw_buf,StrFormat(),gMonst(PLAYER_MONSTER_IDX).hp,gPlayer.max_hp);
   gr_font_string(gShockFont, draw_buf, pt.x, pt.y);
   
   GetNamedFormatString("ow_stats_gp");   
   sprintf(draw_buf,StrFormat(),gPlayer.gp);
   gr_font_string(gShockFont, draw_buf, pt.x+(TEXT_AREA_W/2) + 5, pt.y);
   pt.y+=TEXT_CHAR_H;
   if (gPlayer.level!=GetLevelFromXP(gPlayer.xp))
   {
      GetNamedFormatString("ow_stats_lplus");
      sprintf(draw_buf,StrFormat(),gPlayer.level,GetLevelFromXP(gPlayer.xp));
   }
   else
   {
      GetNamedFormatString("ow_stats_lnorm");      
      sprintf(draw_buf,StrFormat(),gPlayer.level);
   }
   gr_font_string(gShockFont, draw_buf, pt.x, pt.y);
   GetNamedFormatString("ow_stats_xp");
   sprintf(draw_buf,StrFormat(),gPlayer.xp);
   gr_font_string(gShockFont, draw_buf, pt.x+(TEXT_AREA_W/2) + 5, pt.y);
   pt.y+=TEXT_CHAR_H;
}

////////////////
// draw recent messages
static void _ow_draw_message_lines()
{
   Point pt;
   pt.x=TEXT_X;
   pt.y=TEXT_Y+(SIZE_TILE_H*SCREEN_TILES_H)+(STATS_LINES*TEXT_CHAR_H);
   pt.y+=(TEXT_CHAR_H/3);
   gr_hline(pt.x,pt.y,TEXT_AREA_W);
   pt.y+=(TEXT_CHAR_H/3);
   for (int i=0; i<NUM_STATUS_LINES; i++, pt.y+=TEXT_CHAR_H)
      if (status_cur_line[i]!=0)
         gr_font_string(gShockFont, status_lines[i], pt.x, pt.y);
      else
         break;
   status_drawn=TRUE;
}

static BOOL IsScreenSaver(void)
{
   return (gLastInputEvent && (gLastInputEvent+60000<GetSimTime()));
}

////////////////
// actually draw the world
void OverWorldDraw(Point pt)
{
   int fx, fy;
   Point pos, drawpt;

   if (IsScreenSaver())
    { ShockMiniGameScreenSaver(pt,base_rect); return; }

   ShockMiniGameSetupCanvas(pt,base_rect,0);
   gr_set_fcolor(gShockTextColor);
   
   SanityCheckTheWorld("draw");

   if (gCurGameMode!=kGameScores)
   {
      for (pos.y=TILE_OFFSET_X, fy=0; fy<SCREEN_TILES_H; fy++, pos.y+=SIZE_TILE_H)
         for (pos.x=TILE_OFFSET_Y, fx=0; fx<SCREEN_TILES_W; fx++, pos.x+=SIZE_TILE_W)
         {
            int rx=MAP_X(fx+gPlayer.loc.x-PLAYER_OFF_X);
            int ry=MAP_Y(fy+gPlayer.loc.y-PLAYER_OFF_Y);
            _ow_draw_a_tile(pos,rx,ry);
         }
      _ow_draw_character_stats();
      _ow_update_messages();        // every time we draw, check times here
      _ow_draw_message_lines();
      OW_RunActions();              // since these are real time based
   }
   if (gCurGameMode==kGameDead)
   {  // show game over, etc...
#define TILE_AREA_HT (SIZE_TILE_H*(SCREEN_TILES_H-2))
      int y_off=(GetSimTime()-gLastModeChange)/8;
      y_off%=TILE_AREA_HT*2;
      if (y_off>TILE_AREA_HT) y_off=TILE_AREA_HT*2-y_off;
      drawpt.y=TEXT_Y+y_off;
      drawpt.x=TEXT_X+(SIZE_TILE_W*(SCREEN_TILES_W/3));  // why center for real...
      if (WON_THE_GAME())
         GetNamedMessageString("ow_youwon");
      else
         GetNamedMessageString("ow_gameover");
      gr_font_string(gShockFont, StrMsg(), drawpt.x, drawpt.y);
   }
   if (gCurGameMode==kGameScores)
   {  // show scores
      drawpt.y=TEXT_Y+10;  // magic numbers, obviously......
      drawpt.x=TEXT_X;
      _display_scores(drawpt);
      drawpt.y=TEXT_Y;
      drawpt.x=TEXT_X+25;
      gr_font_string(gShockFont, gMiniGameScoreMsg, drawpt.x, drawpt.y);
   }

   ShockMiniGameBlitCanvas(pt,base_rect);
}

////////////////////////////////////////////
// the core of it all - the "simulation"  //
////////////////////////////////////////////

///////////////
// player control (levels, xp, death, special behaviors thereof)

//#define QUICK_TEST_LEVELS
#ifdef QUICK_TEST_LEVELS
#define LVL_TC1   2
#define LVL_HC1   8
#define LVL_MERC  3
#define LVL_SNIK  4
#define LVL_TC2   9
#define LVL_WISE  6
#define LVL_DRAG  5
#define LVL_DC1   7
#define LVL_DC2  10
#else // use the real levels - troll 3 merch 4 snick 7 troll 8 wise 10 drag 12 demon 15
#define LVL_TC1   3
#define LVL_HC1   4
#define LVL_MERC  5
#define LVL_SNIK  7
#define LVL_TC2   9
#define LVL_WISE 10
#define LVL_DRAG 15
#define LVL_DC1  18
#define LVL_DC2  21
#endif

// give player a level, and deal with changes at level boundaries...
void GivePlayerLevel(void)
{
   int new_hp = (gPlayer.level<12)?2+(Rand()%6):1+(Rand()%4); // 1+d6 lvl < 12, d4 otherwise
   char buf[128];
   gPlayer.level++;
   GetNamedFormatString("ow_newlevel");
   sprintf(buf,StrFormat(),gPlayer.level);
   _ow_add_message(buf);
   if (gPlayer.max_hp<222)
   {
      if (gPlayer.max_hp>=200) new_hp=1;  // avoid infinite HP silliness
      gPlayer.max_hp+=new_hp;
      gMonst(PLAYER_MONSTER_IDX).hp+=new_hp;
   }

   // this needs to be done right...
   if (gPlayer.level<=6)
   {
      gMonsterStat[MONSTER_PLAYER].damage.die_size++;
      gMonsterStat[MONSTER_PLAYER].damage.flat_add++;
   }
   else if (gPlayer.level<=12)
      gMonsterStat[MONSTER_PLAYER].damage.die_size++;
   else if (gPlayer.level&0x1)
      gMonsterStat[MONSTER_PLAYER].damage.die_size++;
   
   if (gPlayer.level<=6)
   {
      gMonsterStat[MONSTER_PLAYER].tohit.die_size+=2;      
      gMonsterStat[MONSTER_PLAYER].tohit.flat_add++;
   }
   else if (gMonsterStat[MONSTER_PLAYER].tohit.flat_add<30)  // ??? got me
      gMonsterStat[MONSTER_PLAYER].tohit.flat_add++;
   else if (gMonsterStat[MONSTER_PLAYER].tohit.flat_add<35)
      if (gPlayer.level&0x1)
         gMonsterStat[MONSTER_PLAYER].tohit.flat_add++;
   
   switch (gPlayer.level)  // troll 3 merch 4 snick 7 troll 8 wise 10 drag 12 demon 15
   {  
      case LVL_TC1 : CreateMonsterCamp(MONSTER_TROLL,TERRAIN_PLAIN);         break;
      case LVL_HC1 : CreateMonsterCamp(MONSTER_HEADLESS,TERRAIN_PLAIN);      break;
      case LVL_MERC: CreateTalkingMonster(MONSTER_MERCHANT,TERRAIN_ROAD);    break;
      case LVL_SNIK: CreateTalkingMonster(MONSTER_SNICKERS,TERRAIN_CITY);    break;
      case LVL_TC2 : CreateMonsterCamp(MONSTER_TROLL,TERRAIN_PLAIN);         break;
      case LVL_WISE: CreateTalkingMonster(MONSTER_WISEPERSON,TERRAIN_WATER); break;
      case LVL_DRAG:  // awaken the dragon
         _ow_add_message_named("ow_dragon");
         gMonst(DRAGON_MONSTER_IDX).flags=MONST_FLG_ATTACK;
         break;
      case LVL_DC1 : CreateMonsterCamp(MONSTER_DEMON,TERRAIN_PLAIN);         break;
      case LVL_DC2 : CreateMonsterCamp(MONSTER_DEMON,TERRAIN_PLAIN);         break;
   }
}

void GivePlayerXP(int xp)
{
   BOOL new_level=GetLevelFromXP(gPlayer.xp)<GetLevelFromXP(gPlayer.xp+xp);
   char buf[64];
   gPlayer.xp+=xp;
   GetNamedFormatString("ow_gainxp");
   sprintf(buf,StrFormat(),xp);
   _ow_add_message(buf);
   if (new_level)
      _ow_add_message_named("ow_goshrine");
}

void KillThePlayer(char *cause)
{
   char buf[64];
   gCurGameMode=kGameDead;
   gLastModeChange=GetSimTime();
   GetNamedFormatString("ow_youdied");
   sprintf(buf,StrFormat(),cause);
   RemoveAMonster(PLAYER_MONSTER_IDX,FALSE);
   _ow_add_message(buf);
   _add_high_score(cause);
}

void DamageAMonster(int idx, int dmg, char *cause, BOOL was_player, BOOL action)
{  // if dmg > 0
   sWorldMonster *def_monst=&gMonst(idx);
   char cbuf[128];

   if (def_monst->flags&MONST_FLG_TALK)
      return;   // never damage talking monsters, period...
   
   GetNamedFormatString("ow_damage");
   sprintf(cbuf,StrFormat(),cause,dmg);
   _ow_add_message(cbuf);
   if (action)
      _ow_add_action(COMBAT_HIT,&def_monst->loc,500);

   if (dmg>=def_monst->hp)
   {  // killed
      def_monst->hp=0;
      if (def_monst->type==MONSTER_PLAYER)
      {
#ifdef PLAYTEST
         if (config_is_defined("overworld_invuln"))
         {
            _ow_add_message("You Cheater");
            mprintf("OW Death number %d\n",++gPlayer.death_count);
            def_monst->hp=gPlayer.max_hp;
            return;
         }
#endif      
         KillThePlayer(cause);
         return;
      }
      else
      {
         GetNamedFormatString("ow_itdied");
         GetMonsterName(def_monst->type);  // look, reload StrMonst here, 
         sprintf(cbuf,StrFormat(),StrMonst());
         _ow_add_message(cbuf);
         if (was_player) 
            GivePlayerXP(gMonsterStat[def_monst->type].xp_val);
      }
      RemoveAMonster(idx,TRUE);
   }
   else
      def_monst->hp-=dmg;
}

// so we have XdY+Z > 40 -- XdY > 40-Z - if X is 1, we have a (Y-(40-Z)) in Y chance
int get_thaco(sDieRoll *roll)
{  // if X not 1, we are sad
   //   mprintf("player has %dd%d+%d\n",roll->num_dice,roll->die_size,roll->flat_add);
   return 100*((float)(roll->die_size-(40-roll->flat_add)))/(float)(roll->die_size);
}

void OW_Combat(uchar from_x, uchar from_y, uchar to_x, uchar to_y)
{
   sWorldMapTile *from_sq=&gTile(from_x,from_y);
   sWorldMapTile *to_sq=&gTile(to_x,to_y);

   if ((from_sq->monster_id==OW_EMPTY)||(to_sq->monster_id==OW_EMPTY))
   {
      Warning(("OW Combat called with empty monster (at %d %d - %d %d)\n",from_x,from_y,to_x,to_y));
      return;
   }

   sWorldMonster *att_monst=&gMonst(from_sq->monster_id);
   sWorldMonster *def_monst=&gMonst(to_sq->monster_id);

   def_monst->flags|=MONST_FLG_WAS_HIT;  // really, was attacked, but hey
   GetMonsterName(att_monst->type);      // lots of us use this
   
   int hitroll=do_die_roll(&gMonsterStat[att_monst->type].tohit);
   hitroll-=gMonsterStat[def_monst->type].armor/2;  // 1/2 armor is an AC vs. tohit?
   if (hitroll<40)
   {
      char buf[128];
      GetNamedFormatString("ow_missed");
      sprintf(buf,StrFormat(),StrMonst());
      _ow_add_message(buf);
      _ow_add_action(COMBAT_MISS,&def_monst->loc,500);
      return;
   }

   int dmg=do_die_roll(&gMonsterStat[att_monst->type].damage);
   dmg-=gMonsterStat[def_monst->type].armor;

   char cbuf[128];
   if (dmg<=0)
   {
      GetNamedFormatString("ow_nodamage");
      sprintf(cbuf,StrFormat(),StrMonst());      
      _ow_add_message(cbuf);
      return;
   }
   else
      DamageAMonster(to_sq->monster_id,dmg,StrMonst(),att_monst->type==PLAYER_MONSTER_IDX,TRUE);
}

void MoveCreature(sOWMapLoc *loc, uchar x_new, uchar y_new)
{
   uchar our_id=gTileMonstPt(loc);
   gTileMonstPt(loc)=OW_EMPTY;
   loc->x=x_new;
   loc->y=y_new;
   gTileMonstPt(loc)=our_id;
}

// i.e. largest val in string file, we will add 1 to get 0 -> n
#define NUM_WISE_SAYS   (7)
#define NUM_SNICK_SAYS (20)

static void _do_saying(char *base, int count)
{
   char buf[256], which=Rand()%(count+1);
   GetNamedStringNum(buf,base,which);
   _ow_add_message(buf);
}

// otherwise, pass the terrain type desired
#define GOODBYE_REMOVE    (0xff)
static void _goodbye(int id, char *say, int what_now)
{
   if (say)
      _ow_add_message_named(say);
   if (what_now==GOODBYE_REMOVE)
      RemoveAMonster(id,FALSE);
   else
   {
      uchar x,y;
      FindClearNearTerr(&x,&y,what_now,FALSE,TRUE);
      MoveCreature(&gMonstLoc(id),x,y);
   }
}

void OW_TalkTo(uchar id)
{
   int state;
   switch (gMonstType(id))
   {
      case MONSTER_WISEPERSON:
         state=GetConverseState(WISEPERSON);
         switch (state)
         {
            case 0: _ow_add_message_named("ow_hello"); break;
            case 1: _do_saying("ow_wiseman_",NUM_WISE_SAYS); break;
            case 2: _goodbye(id,"ow_wiseman_bye",GOODBYE_REMOVE); break;
         }
         SetConverseState(WISEPERSON,state+1);
         break;
      case MONSTER_SNICKERS:
         _ow_add_message_named("ow_hello");
         _do_saying("ow_snickers_",NUM_SNICK_SAYS);
         _goodbye(id,NULL,TERRAIN_CITY);
         break;         
      case MONSTER_MERCHANT:
         state=GetConverseState(MERCHANT);
         switch (state)
         {
            case 0:
               _ow_add_message_named("ow_hello");
               _ow_add_message_named("ow_merchant_1");
               state=1;
               break;
            case 1:
               if (gPlayer.gp>=50)
               {
                  gPlayer.potions++;
                  gPlayer.gp-=50;
                  _goodbye(id,"ow_merchant_2",TERRAIN_ROAD);
               }
               else
                  _goodbye(id,"ow_merchant_0",TERRAIN_ROAD);
               state=0;
               break;
         }
         SetConverseState(MERCHANT,state);
         break;
      case MONSTER_LB:  // ok....
         while (gPlayer.level<GetLevelFromXP(gPlayer.xp))
            GivePlayerLevel();  // auto level up and heal whenever talking to LB?
         heal_the_player(0,100); // why not, live it up, etc...
         state=GetConverseState(LORDB);
         switch (state)
         {
            case 0:
               gTileTerrPt(&gPlayer.temple)=TERRAIN_TEMPLE;
               state=1;  // temple has been created
            case 1:
               _ow_add_message_named("ow_lb_1");
               break;  // talked to, now seal the temple
            case 2:    // first time talking to LB since you sealed the temple
               state=3;
            case 3: // post seal, dragons are loose, etc etc
               _ow_add_message_named("ow_lb_2");
               break;
         }
         SetConverseState(LORDB,state);
         break;
      default:
         Warning(("Note: flags %x\n",gMonst(id).flags));
         _ow_add_message_named("ow_silent");
   }
}

// returns gold value or zero for equipment
int GetLootValue(uchar x, uchar y)
{
   static uchar gold_vals[LOOT_MAX_GOLD+1]={1,6,22-5};  // loot little, lots, tons
   int amt=gTile(x,y).treasure;
   if (amt<=LOOT_MAX_GOLD)
      return gold_vals[amt];
   return 0;
}

// actually take the loot
int TakeLoot(uchar x, uchar y)
{
   int amt=GetLootValue(x,y);
   gTile(x,y).treasure=OW_EMPTY;
   return amt;
}

BOOL CanEnter(int monster_type, uchar x, uchar y)
{
   if (gTileMonst(x,y)!=OW_EMPTY)
      return FALSE;  // cant have 2 in the same pplace...
   return ((gMonsterStat[monster_type].mobility_bits&
            gTerrStat[gTileTerr(x,y)].mobility_bits)!=0);
}

static void HandleTreasure(sWorldMapTile *us, int new_x, int new_y)
{
   if (us->treasure>LOOT_MAX_GOLD)
   {
      if (us->treasure>LOOT_MAX_EQUIP)
      {
         switch (us->treasure)
         {
         case LOOT_POTION: gPlayer.potions++; break;
         }
      }
      else
      {  // do we already have this treasure?
         if (!(gPlayer.equipment & (1 << us->treasure)))
         {
            _ow_add_message_named("ow_find_loot_",us->treasure); 
            gPlayer.equipment |= (1 << us->treasure);       // mark it grabbed
            switch (us->treasure)
            {  // update our stats -  note really stupid system for upgrading equipment
               case LOOT_EXOTIC:
                  // if we already have the shield, subtract that first
                  if (gPlayer.equipment & (1 << LOOT_SHIELD))
                     gMonsterStat[MONSTER_PLAYER].armor-=SHIELD_ARMOR_VAL;
                  gMonsterStat[MONSTER_PLAYER].armor+=EXOTIC_ARMOR_VAL;
                  break;
               case LOOT_SHIELD:
                  if ((gPlayer.equipment & (1 << LOOT_EXOTIC))==0)
                     gMonsterStat[MONSTER_PLAYER].armor+=SHIELD_ARMOR_VAL;
                  break;
               case LOOT_HOD:
                  // if we already have the masamune, subtract that first
                  if (gPlayer.equipment & (1 << LOOT_MASAMUNE))
                     gMonsterStat[MONSTER_PLAYER].damage.flat_add-=MASAMUNE_ATTACK;
                  gMonsterStat[MONSTER_PLAYER].damage.flat_add+=HOD_ATTACK;
                  break;
               case LOOT_MASAMUNE:
                  if ((gPlayer.equipment & (1 << LOOT_HOD))==0)
                     gMonsterStat[MONSTER_PLAYER].damage.flat_add+=MASAMUNE_ATTACK;
                  break;
            }
         }
      }
      us->treasure=OW_EMPTY;
   }
   else
   {
      char buf[64];
      int amt=TakeLoot(new_x,new_y);
      GetNamedFormatString("ow_gotgold");
      sprintf(buf,StrFormat(),amt);
      _ow_add_message(buf);
      gPlayer.gp+=amt;
   }
}

void PlayerMoveTo(uchar new_x, uchar new_y)
{
   sWorldMapTile *us=&gTile(new_x,new_y);

   // first, is there a combat
   if (us->monster_id!=OW_EMPTY)
      if (gMonst(us->monster_id).flags&MONST_FLG_TALK)
         OW_TalkTo(us->monster_id);
      else
         OW_Combat(gPlayer.loc.x,gPlayer.loc.y,new_x,new_y);
   else // if not combat, then we are going to try and move into the square
      if (CanEnter(MONSTER_PLAYER,new_x,new_y))
      {
         if (us->treasure!=OW_EMPTY) 
            HandleTreasure(us,new_x,new_y);
         MoveCreature(&gPlayer.loc,new_x,new_y);
         // since the player is special
         gMonstLoc(PLAYER_MONSTER_IDX)=gPlayer.loc;
      }
      else
         _ow_add_message_named("ow_impass");
}

// get useful world deltas
void _get_deltas(sOWMapLoc *l1, sOWMapLoc *l2, int *dx, int *dy)
{
   *dx=l2->x-l1->x;
   *dy=l2->y-l1->y;
   if ((*dx)<-(WORLD_TILES_W/2)) *dx+=WORLD_TILES_W;
   if ((*dy)<-(WORLD_TILES_H/2)) *dy+=WORLD_TILES_H;
   if ((*dx)> (WORLD_TILES_W/2)) *dx-=WORLD_TILES_W;
   if ((*dy)> (WORLD_TILES_H/2)) *dy-=WORLD_TILES_H;
}

int _dist_to_camp(sWorldMonster *us)
{
   int dx,dy;
   _get_deltas(&us->loc,&us->targ,&dx,&dy);
   return abs(dx)+abs(dy);        // get "distance" to targ
}

// did we move, or are we already there
BOOL OW_MoveAI(sWorldMonster *us, sOWMapLoc *targ)
{
   uchar next_x, next_y;
   int dx,dy;

   // figure out next square
   _get_deltas(&us->loc,targ,&dx,&dy);
   if ((dx|dy)==0)
      return FALSE;

   if (dx>0) dx=1; else if (dx<0) dx=-1;
   if (dy>0) dy=1; else if (dy<0) dy=-1;

   next_x=MAP_X(us->loc.x+dx);
   next_y=MAP_Y(us->loc.y+dy);
   
   BOOL do_move=FALSE;
   if (CanEnter(us->type,next_x,next_y))
      do_move=TRUE;
   else
   {  // hmmm, im frustrated... what to do if i cant enter?
      if (dx!=0)
      {
         int try_x=MAP_X(us->loc.x);
         if (CanEnter(us->type,try_x,next_y))
         {
            next_x=try_x;
            do_move=TRUE;
         }
      }
      if (dy!=0)
      {
         int try_y=MAP_Y(us->loc.y);
         if (CanEnter(us->type,next_x,try_y))
         {
            next_y=try_y;
            do_move=TRUE;
         }
      }
   }
   if (do_move)
      MoveCreature(&us->loc,next_x,next_y);

   return TRUE;
}

// does this square contain treasure accessible to us
BOOL _accessible_treasure(sWorldMonster *us, sOWMapLoc *loc)
{
   if (gTilePt(loc).treasure!=OW_EMPTY)      // is there treasure
      if ((gTerrStat[gTileTerrPt(loc)].mobility_bits&
           gMonsterStat[us->type].mobility_bits)!=0)   // can we go there?
         return TRUE;    // let us, then...
   return FALSE;
}

static BOOL hateful_static_were_we_seen;   // for wander look around
static BOOL hateful_static_did_we_enrage;  // for notify friends

// check nearby for friends who may care im being attacked
BOOL _notify_friends(sWorldMonster *us, sOWMapLoc *loc)
{
#ifdef PLAYTEST
   if (config_is_defined("overworld_ainotaware"))
      return FALSE;
#endif   
   if (gTileMonstPt(loc)!=OW_EMPTY)    // is there a monster
   {
      sWorldMonster *them=&gMonst(gTileMonstPt(loc));
      if (us!=them)
         if ((them->flags&(MONST_FLG_ATTACK|MONST_FLG_TALK|MONST_FLG_FLEE))==0)
         {
            int notify_chance;
            if (them->type==us->type)
               notify_chance=(them->flags&MONST_FLG_CLOUD)?95:90;
            else  // use a level based formula here
            {
               int lvl_diff=(gMonsterStat[them->type].level-gMonsterStat[us->type].level);
               notify_chance=60-(15*lvl_diff);
               if (them->flags&(MONST_FLG_CLOUD|MONST_FLG_GUARD))
                  notify_chance=0;  // in a cloud, only help friends
            }
            if (Rand()%100<notify_chance)
            {  // this must be changed, somehow
               them->flags&=~MONST_FLG_WANDER;
               them->flags&=~MONST_FLG_GUARD;
               them->flags|=MONST_FLG_ATTACK;
               hateful_static_did_we_enrage=TRUE;
            }
      }
   }
   return FALSE;  // since we want to notify all of them
}

// callback returns whether to abort the search operation or not?
// loc is MODIFIED in place...
BOOL _check_nearby(sWorldMonster *us, sOWMapLoc *loc, int rad, BOOL (*check)(sWorldMonster *us, sOWMapLoc *loc))
{
   for (int dx=-rad; dx<=rad; dx++)
      for (int dy=-rad; dy<=rad; dy++)
      {
         loc->x=MAP_X(us->loc.x+dx);
         loc->y=MAP_Y(us->loc.y+dy);
         if ((*check)(us,loc))
            return TRUE;
      }
    return FALSE;  // none around we care about...
}

// for now, AI wants to move towards the player, local minmax idiocy 
// perhaps allow lookahead at some point, ideally would flood fill
// but not right now, eh?
void OW_RunAI(int id)
{
   sWorldMonster *us=&gMonst(id);
   if (us->flags&MONST_FLG_WAS_HIT)
   { 
      sOWMapLoc loc=us->loc;
      _check_nearby(us,&loc,5,_notify_friends);
      if ((us->flags&MONST_FLG_CLOUD)&&(us->flags&MONST_FLG_DAMAGED)&&(_dist_to_camp(us)>10))
      {
         us->flags|= MONST_FLG_FLEE;
         us->flags&=~MONST_FLG_ATTACK;
      }
      else
         us->flags|=MONST_FLG_ATTACK;
      us->flags&=~(MONST_FLG_WAS_HIT|MONST_FLG_DAMAGED);  // clear damage flags
      us->flags&=~MONST_FLG_WANDER;    // so we stop wandering, and fight back
   }
   if (us->flags&MONST_FLG_ATTACK)
   {  // is the player nearby
      int dx,dy;
      _get_deltas(&us->loc,&gPlayer.loc,&dx,&dy);
      if ((abs(dx)<=1)&&(abs(dy)<=1)) 
         OW_Combat(us->loc.x,us->loc.y,gPlayer.loc.x,gPlayer.loc.y);
      else if ((us->flags&MONST_FLG_GUARD)==0)
      {  // should use level delta here!!!
         if ((Rand()%100) < (2*(abs(dx)+abs(dy)))-1)
         {
            us->flags &=~MONST_FLG_ATTACK;
            if ((us->flags&MONST_FLG_CLOUD)==0)
               us->flags |= MONST_FLG_WANDER;
         }
         else
            OW_MoveAI(us,&gPlayer.loc);
      }
   }
   if (us->flags&MONST_FLG_FLEE)
      ;
   if (us->flags&MONST_FLG_CLOUD)
      if ((us->flags&(MONST_FLG_ATTACK|MONST_FLG_GUARD))==0)
      {
         sOWMapLoc loc;
         int dist,ret_chance;
         dist=_dist_to_camp(us);
         ret_chance=20+(dist*10);
         if (us->flags&MONST_FLG_FLEE)
            if (dist>4)
               ret_chance=100;
            else
               us->flags&=~MONST_FLG_FLEE;
         if (Rand()%100<ret_chance) // bias chance to head there by distance
            loc=us->targ;
         else
         {  // else random
            loc.x=MAP_X(us->loc.x+(Rand()%9)-4);
            loc.y=MAP_Y(us->loc.y+(Rand()%9)-4);
         }
         OW_MoveAI(us,&loc);  // move towards chosen point
      }
   if (us->flags&MONST_FLG_WANDER)
   {
      sOWMapLoc loc;
      BOOL found=FALSE;
      if (((Rand()&0x1)==0)&&(us->type!=MONSTER_DRAGON))
         found=_check_nearby(us,&loc,5,_accessible_treasure);
      if (!found)
      {
         loc.x=MAP_X(us->loc.x+(Rand()%9)-4);
         loc.y=MAP_Y(us->loc.y+(Rand()%9)-4);
      }
      OW_MoveAI(us,&loc);  // just wander towards it...
      if (us->type!=MONSTER_DRAGON)
      {
         int guard_chance=0;  // if we wander across treasure/city, do we want to guard it?
         if (gTilePt(&us->loc).treasure!=OW_EMPTY)
            guard_chance=gTilePt(&us->loc).treasure==LOOT_LITTLE?15:75;
         else
            guard_chance=gTerrStat[gTileTerrPt(&us->loc)].guard_chance;
         if (guard_chance)
            if ((Rand()%100)<guard_chance)
            {
               us->flags|= MONST_FLG_GUARD;
               us->flags&=~MONST_FLG_WANDER;
            }
      }
   }
   if (us->flags&MONST_FLG_GUARD)
      ;
   if (us->flags&MONST_FLG_TALK)
      ;
   if ((us->flags&(MONST_FLG_GUARD|MONST_FLG_ATTACK|MONST_FLG_TALK|MONST_FLG_CLOUD))==0)
      if ((Rand()&0xf)<3)
      {  // if not guarding or attacking, 3/16 look for the player
         int dx,dy;
         _get_deltas(&us->loc,&gPlayer.loc,&dx,&dy);
         int net_rad=((int)gMonsterStat[us->type].view_rad)-(abs(dx)+abs(dy));
         if (net_rad>0)
            if (Rand()%100<net_rad*10)
#ifdef PLAYTEST
               if (!config_is_defined("overworld_ainotaware"))
#endif
               {  // look, we saw the player
                  us->flags=MONST_FLG_ATTACK;
                  hateful_static_were_we_seen=TRUE;
//                  mprintf("Seen by a %d at %d %d delta %d %d net %d\n",us->type,us->loc.x,us->loc.y,dx,dy,net_rad);
               }
      }
}

void OW_DragonFireball(int idx)
{
   sWorldMonster *us=&gMonst(idx);
   int dx,dy;
   _get_deltas(&us->loc,&gPlayer.loc,&dx,&dy);
   if ((abs(dx)+abs(dy))<gMonsterStat[us->type].view_rad)
   {  // throw it at the player
      if (abs(dx)*2<abs(dy))
       { dx=0; dy=(dy>0)?1:-1; }
      else if (abs(dy)*2<abs(dx))
       { dy=0; dx=(dx>0)?1:-1; }
      else
       { dy=(dy>0)?1:-1; dx=(dx>0)?1:-1; }
   }
   else if ((Rand()&0x7)==0)
   {  // otherwise, 1 in 8, just throw anyway
      dx=(Rand()%3)-1; dy=(Rand()%3)-1;
      if ((dx|dy)==0)  // well, ok, 1 in 8 minus 1 in 9, so 8/9 of 1/8.  hmmm
         return;  // never mind, we suck
   }
   sOWMapLoc loc_tmp=us->loc;
   if (UpdateVelAction(TRUE,COMBAT_FIREBALL,&loc_tmp,dx,dy))
      _ow_add_vel_action(COMBAT_FIREBALL,&loc_tmp,8,dx,dy);
}

// actually move the object
// if create, then _dont_ damage things if you hit them, and if success _place_ in the world
// return TRUE if it moved, FALSE if destroyed
BOOL UpdateVelAction(BOOL create, int type, sOWMapLoc *loc, int dx, int dy)
{
   int nx=MAP_X(loc->x+dx), ny=MAP_Y(loc->y+dy);
   if (gTileMonst(nx,ny)!=OW_EMPTY)
   {  
      if (!create)
      {  // do some damage then delete
         char buf[64];
         int idx=gTileMonst(nx,ny);
         sDieRoll spell_dmg[]={{3,8,3},{1,8,2}}, *use;
         use=(type==COMBAT_FIREBALL)?&spell_dmg[0]:&spell_dmg[1];
         int dmg=do_die_roll(use);
         if (gMonstType(idx)!=MONSTER_DRAGON)
         {
            GetNamedStringNum(buf,"ow_spell_",type-COMBAT_FIRST_SPELL+1);  // 1 is FB, 2 is missile
            DamageAMonster(idx,dmg,buf,FALSE,FALSE);
         }
         gTilePt(loc).action=OW_EMPTY; // remove old action
      }
      return FALSE;
   }
   if (gTile(nx,ny).action!=OW_EMPTY)
   {  // for now, oh well....
   }
   gTilePt(loc).action=OW_EMPTY;
   loc->x=nx; loc->y=ny; // move us to our new locale, eh?
   gTilePt(loc).action=type;
   return TRUE;
}

//////////////////////////////
// various area checks for player state

// check to see if player needs healing and can afford healing at this area
void _ow_check_healing_area(int cost)
{
   if (gMonst(PLAYER_MONSTER_IDX).hp<gPlayer.max_hp)
   {
      char buf[64];
      if (gPlayer.gp>=cost)
      {
         heal_the_player(0,10);
         gPlayer.gp-=cost;
         GetNamedFormatString("ow_payheal");
         sprintf(buf,StrFormat(),cost);
      }
      else
      {
         GetNamedFormatString("ow_nocash");
         sprintf(buf,StrFormat(),cost);
      }
      _ow_add_message(buf);      
   }
}

//////////////////////////////
// quest controller

#define QUEST_BASEEXP    5
#define QUEST_MAXEXP     20
#define QUEST_MAXLVLMUL  2     
#define QUEST_GROWEXP    2
static void DoQuest(sWorldMapTile *us)
{
   if (gPlayer.quest == OW_EMPTY) 
   {
      if (us->terrain == TERRAIN_CITY)
      {
         do {
            if (Rand() % 100 < 25)   // 1 in 4 go to cities
               gPlayer.quest = Rand() % NUM_CITIES;   
            else                     // rest go to villages
               gPlayer.quest = NUM_CITIES + (Rand() % NUM_VILLAGES);
         } while (gPlayer.quest==us->loc_id);
         char buf[255];
         GetNamedFormatString("ow_getquest");
         sprintf(buf,StrFormat(),gCityNames[gPlayer.quest]);
         _ow_add_message(buf);
      }
   }
   else    // are we in the right city/village?
      if (us->loc_id == gPlayer.quest)
      {  // give some exp, which grows as quests are done
         int grow = QUEST_GROWEXP + (gPlayer.level/10);
         int exp  = (gPlayer.questcount * grow) + QUEST_BASEEXP;
         int max  = QUEST_MAXEXP+(QUEST_MAXLVLMUL*gPlayer.level);
         if (exp > max)
            exp = max;
         GivePlayerXP(exp);
         gPlayer.questcount++;              // reset quest DB
         gPlayer.quest = OW_EMPTY;
         _ow_add_message_named("ow_doquest");
      }
}

static int CountMonsters(int type, int x, int y, int rad)
{
   int cnt=0;
   for (int i=-rad; i<=rad; i++)
      for (int j=-rad; j<=rad; j++)
      {
         int id=gTileMonst(MAP_X(x+i),MAP_Y(y+j));
         if (id!=OW_EMPTY)
            if (gMonstType(id)==type)
               cnt++;
      }
   return cnt;
}

#ifdef PLAYTEST
#define CampTalk() config_is_defined("overworld_camps")
#define _camp_mprintf(x) do { if (CampTalk()) mprintf x; } while (0)
#else
#define CampTalk() FALSE
#define _camp_mprintf(x) 
#endif

static void PutAtCamp(int idx, int x, int y, float hp_fac, BOOL in_cloud)
{
   int flag=in_cloud?MONST_FLG_CLOUD:MONST_FLG_GUARD;
   SetupMonsterCampMonster(idx,flag,hp_fac,x,y);
}

typedef enum { kCampAsleep, kCampAwake, kCampCharge, kCampDead } eCampState;

// @TBD: really need to call per frame, build up more incrementally
// the waves of creatures every 16 frames is IDIOTIC
void UpdateSingleCamp(int x, int y)
{
   int camp_type=gTile(x,y).loc_id-LOC_MONSTER_BASE;
   int lvl_delta=gPlayer.level-2*gMonsterStat[camp_type].level;
   int targ_cnt=3+(gMonsterStat[camp_type].level+lvl_delta)/2;
   int camp_state=lvl_delta>0?kCampCharge:gPlayer.level<=2?kCampAsleep:kCampAwake;
   if (gPlayer.level>4+(3*gMonsterStat[camp_type].level))
      camp_state=kCampDead;
   else if (targ_cnt>8) targ_cnt=8;

   // first off, is there someone standing on the camp
   if (gTileMonst(x,y)==OW_EMPTY)
   {  
      int idx=BuildAMonster(-1,camp_type,x,y);
      PutAtCamp(idx,x,y,2.0,camp_state==kCampCharge);
      _camp_mprintf(("Putting a new guy on top of the camp...\n"));
   }
   if ((camp_state==kCampAsleep)||(camp_state==kCampDead))
   {
      _camp_mprintf(("Camp at %d %d state %s (delta %d)\n",
                     x,y,camp_state==kCampAsleep?"Asleep":"Dead",lvl_delta));
      return;
   }
   int blt=0, cnt=CountMonsters(camp_type,x,y,7); // blt=#built, cnt=how many within 7
   _camp_mprintf(("Update camp at %d %d, state %d.. type %d found %d targ %d lvld %d\n",
                  x,y,camp_state,camp_type,cnt,targ_cnt,lvl_delta));
   for (int i=-1; i<=1; i++)
      for (int j=-1; j<=1; j++)
      {  // now check nearby areas to try and spawn more!
         int ux=MAP_X(x+i), uy=MAP_Y(y+j);
         //         mprintf("[%d %d].",ux,uy);
         if (gMonstType(gTileMonst(ux,uy))==camp_type)
         {  // im already here... maybe send me out to wander?
            if (camp_state==kCampCharge)
               if ((Rand()&0x3)<2) // 50/50 for now
               {
                  sWorldMonster *us=&gMonst(gTileMonst(ux,uy));
                  if ((us->flags&MONST_FLG_GUARD)==MONST_FLG_GUARD)
                  {
                     us->flags&=~MONST_FLG_GUARD;
                     _camp_mprintf(("switched camper at %d %d to only CLOUD\n",ux,uy));
                  }
               }
         }
         else if ((i|j)!=0)
            if (gTileMonst(ux,uy)==OW_EMPTY)
            {  // now, and ugly formula about building more monsters...
               if ((gMonsterCount<90)&&(cnt+blt<targ_cnt))
               {  // ok, there is space overall and in our area
                  int chance=30+(4*(targ_cnt-(cnt+blt)))+(lvl_delta*6);
                  _camp_mprintf(("Build chance at %d %d is %d...",ux,uy,chance));
                  if (Rand()%100<chance)
                  {
                     int idx=BuildAMonster(-1,camp_type,ux,uy);
                     PutAtCamp(idx,x,y,1.4,camp_state==kCampCharge);
                     blt++;
                     _camp_mprintf(("built\n"));
                  }
                  else
                     _camp_mprintf(("not built\n"));
               }
               else
                  _camp_mprintf(("too many mcs (%d) not enough mics (%d on %d)\n",gMonsterCount,cnt+blt,blt));
            }
      }
}

// monster camp updates
void UpdateMonsterCamps(void)
{  // idiotic, but hey, avoids a parallel array
   for (int x=0; x<WORLD_TILES_W; x++)
      for (int y=0; y<WORLD_TILES_H; y++)
         if (gTileTerr(x,y)==TERRAIN_CAMP)
            UpdateSingleCamp(x,y);
}

//////////////////////////////
// the actual sim itself - per frame updates

void OW_RunSim(void)
{
   // run AI for all the monsters...
   hateful_static_did_we_enrage=hateful_static_were_we_seen=FALSE;
   for (int i=1; i<MAX_MONSTERS; i++)    // hateful, so we only get one of these 
      if (gMonstType(i)!=OW_EMPTY)       //  at most per frame, not tons of them
         OW_RunAI(i);
   if (hateful_static_did_we_enrage)
      _ow_add_message_named("ow_enrage");    // only allow these once per frame
   else if (hateful_static_were_we_seen)     //   and only do seen if not enraging
      _ow_add_message_named("ow_wereseen");  //   nearby dudes...

   // respawn monsters and treasure
   BOOL new_monster=FALSE;
   if (gMonsterCount<30)
      new_monster=TRUE;
   else if (gMonsterCount<50)
      new_monster=((Rand()&0xff)<8);
   else if (gMonsterCount<100)
      new_monster=((Rand()&0xff)<4);
   if (new_monster)
      CreateAMonster(TRUE,-1,OW_EMPTY);

   // check monster camps - perhaps we should do iffing and turn counting internally - smoother!
   if ((gPlayer.turns&0x0f)==0x08)    // every 16, offset from other checks
      UpdateMonsterCamps();

   // spawn new treasure every 32 turns? - probably should count, really
   if ((gPlayer.turns&0x01f)==0)
   {
      static sDieRoll sharp_die={4,2,0}; // 4-8, avg 6
      int roll=do_die_roll(&sharp_die)-4; // 0-4, avg 2, prob 1/16, 4/16, 6/16, 4/16, 1/16
      if (roll>=2) roll-=2; // now we have 0 - 7 ways, 1 - 8 ways, 2 - 1 way
      if ((roll<0)||(roll>2)) roll=1; // should never happen, but hey, safety first...
      uchar x,y;
      FindSafeSquare(TRUE,gMonsterStat[MONSTER_PLAYER].mobility_bits,OW_EMPTY,&x,&y);
      PlaceTreasure(roll,x,y);
   }
   
   // misc player control stuff
   
   // shrine setup
   if (gTileTerrPt(&gPlayer.loc)==TERRAIN_SHRINE)
   {
      if (gPlayer.level<GetLevelFromXP(gPlayer.xp))
      {
         int turns_needed=gPlayer.level>8?16:gPlayer.level*2;
         if (++gPlayer.shrine_time==1)
            _ow_add_message_named("ow_startpray");
         if (gPlayer.shrine_time&1)
            _ow_add_message_named("ow_keeppray");
         if (gPlayer.shrine_time>turns_needed)
         {
            GivePlayerLevel();
            gPlayer.shrine_time=0;
         }
      }
   }
   else if (gPlayer.shrine_time)
   {
      if (gPlayer.level<GetLevelFromXP(gPlayer.xp))
         _ow_add_message_named("ow_stoppray");
      gPlayer.shrine_time=0;
   }

   // auto-healing
   if (gPlayer.heal_timer!=0)
      gPlayer.heal_timer--;
   else if (gMonst(PLAYER_MONSTER_IDX).hp<gPlayer.max_hp)
   {
      gPlayer.heal_timer=64;
      heal_the_player(0,5);
   }

   // if hacked, you can cast spells..
   if (gPlayer.spell_timer!=0)
      gPlayer.spell_timer--;

   // if im in a town or village, do healing, check quests
   switch (gTileTerrPt(&gPlayer.loc))
   {
      case TERRAIN_VILLAGE:
         _ow_check_healing_area(VILLAGE_HEAL_COST);
         DoQuest(&gTilePt(&gPlayer.loc));
         break;
      case TERRAIN_CITY:
         _ow_check_healing_area(CITY_HEAL_COST);
         DoQuest(&gTilePt(&gPlayer.loc));
         break;
      case TERRAIN_CAMP:
      {
         int type=(gTilePt(&gPlayer.loc).loc_id-LOC_MONSTER_BASE);
         GivePlayerXP(15+3*gMonsterStat[type].level*gMonsterStat[type].level);
         _ow_add_message_named("ow_destroycamp");
         gTilePt(&gPlayer.loc).terrain=TERRAIN_PLAIN;
         gTilePt(&gPlayer.loc).loc_id=OW_EMPTY;
         break;
      }
      case TERRAIN_TEMPLE:     // if at the elemental temple
      {
         int state=GetConverseState(LORDB);
         if (state==1)  // you've talked to him, so temple is up and this is first time here
         {
            _ow_add_message_named("ow_temple");
            GivePlayerXP(222);
            gPlayer.drag_ids[ELEM_DRAGON_MTN]   =CreateAMonster(TRUE,-1,MONSTER_DRAGON);
            gPlayer.drag_ids[ELEM_DRAGON_WATER] =CreateAMonster(TRUE,-1,MONSTER_DRAGON);
            gPlayer.drag_ids[ELEM_DRAGON_FOREST]=CreateAMonster(TRUE,-1,MONSTER_DRAGON);
            gPlayer.drag_ids[ELEM_DRAGON_ROAD]  =CreateAMonster(TRUE,-1,MONSTER_DRAGON);         
            SetConverseState(LORDB,2);
         }
         break;
      }
   }

   // and speaking of crazy dragon fun
   if ((gPlayer.dragons==0)&&(gPlayer.level>(LVL_DRAG/2))) // normal dragon gets ornery
      if ((Rand()&0x3f)<3)
         OW_DragonFireball(DRAGON_MONSTER_IDX);
   if (gPlayer.dragons>=1)   // if the elemental dragons are out...
      for (int j=0; j<NUM_ELEM_DRAGONS; j++)
         if (gPlayer.drag_ids[j]!=0)  // there is one
         {  // change the terrain underneath me
            static uchar myterr[]={TERRAIN_MOUNTAIN,TERRAIN_WATER,TERRAIN_FOREST,TERRAIN_ROAD};
            sWorldMonster *us=&gMonst(gPlayer.drag_ids[j]);
            gTileTerrPt(&us->loc)=myterr[j];
            if ((Rand()&0x1f)<3)   // think about a fireball every 10 turns or so
               OW_DragonFireball(gPlayer.drag_ids[j]);
         }
   
   // give more hp over time
   if (((++gPlayer.turns)&0x0ff)==0)
   {
      if (gPlayer.max_hp<180)  // have to do levels from here on up....
         gPlayer.max_hp++;     // sorry
      _ow_add_message_named("ow_moreturn");
   }
}

////////////////////////
// deal with the mouse

BOOL WereMousingBadTime(void)
{
   if (IsScreenSaver())
   {
      gLastInputEvent=GetSimTime();
      return TRUE;  // since we want to "eat" the event
   }
   gLastInputEvent=GetSimTime();
   if (gCurGameMode!=kGamePlay)
   {
      if (gLastModeChange+3000<GetSimTime())
         switch (gCurGameMode)
         {
            case kGameDead:
               gCurGameMode=kGameScores;
               gLastModeChange=GetSimTime();
               break;
            case kGameScores:
               CreateTheGameSpace();
               _ow_add_message_named("ow_restart");
               break;
         }
      return TRUE;
   }
   return FALSE;
}

void PickDXDYFromClick(Point pt, int *i_dx, int *i_dy)
{
   Point delta;
   int dx=0, dy=0, dtx, dty;
   
   delta.x=pt.x-(base_rect.ul.x+(PLAYER_OFF_X*SIZE_TILE_W)+(SIZE_TILE_W/2));
   delta.y=pt.y-(base_rect.ul.y+(PLAYER_OFF_Y*SIZE_TILE_H)+(SIZE_TILE_H/2));
   dtx=(delta.x>0)?1:-1;
   dty=(delta.y>0)?1:-1;
   
   if ((abs(delta.x)>SIZE_TILE_W)||(abs(delta.y)>SIZE_TILE_H))
      if (abs(delta.x)<SIZE_TILE_W)       dy=dty;
      else if (abs(delta.y)<SIZE_TILE_H)  dx=dtx;
      else
      {
         float slope = (float)delta.y/(float)delta.x;
         if (fabs(slope)>2.0)      dy=dty;
         else if (fabs(slope)<0.5) dx=dtx;
         else            { dx=dtx; dy=dty; }
      }
   *i_dx=dx;
   *i_dy=dy;
}

void OverWorldMouse(Point pt)
{
   if (WereMousingBadTime())
      return;

   Point mpt;
   mpt.x = pt.x - base_rect.ul.x;
   mpt.y = pt.y - base_rect.ul.y;
   if (RectTestPt(&potion_rect,mpt))
   {
      if (gPlayer.potions > 0)
      {
         gPlayer.potions--;
         heal_the_player(POTION_HEAL,0);
      }
      else
         _ow_add_message_named("ow_nopotion");
   }
   else if (gPlayer.casting)
   {
      int dx,dy;
      PickDXDYFromClick(pt,&dx,&dy);
      sOWMapLoc loc_tmp=gPlayer.loc;
      if (UpdateVelAction(TRUE,COMBAT_MISSILE,&loc_tmp,dx,dy))
      {
         _ow_add_vel_action(COMBAT_MISSILE,&loc_tmp,7,dx,dy);
         gPlayer.spell_timer=64;  // ??? level dependant, maybe?
      }
      gPlayer.casting=FALSE;
   }
   else if (ShockMiniGameIsHacked()&&RectTestPt(&spell_rect,mpt))
      gPlayer.casting=TRUE;
   else  // normal thing
   {
      int dx,dy;
      PickDXDYFromClick(pt,&dx,&dy);      
      if ((dx|dy)!=0) 
      {
         uchar new_x=MAP_X(gPlayer.loc.x+dx);
         uchar new_y=MAP_Y(gPlayer.loc.y+dy);
         PlayerMoveTo(new_x,new_y);
      }
   }

   // hey, we "updated" the sim - run the AI, run the stats, so on...
#ifdef PLAYTEST
   if (!config_is_defined("overworld_slew"))
#endif         
      OW_RunSim();
}

void OverWorldRightMouse(Point pt)
{
   if (WereMousingBadTime())
      return;
   
   int dx=(pt.x-(base_rect.ul.x+(PLAYER_OFF_X*SIZE_TILE_W)));
   int dy=(pt.y-(base_rect.ul.y+(PLAYER_OFF_Y*SIZE_TILE_H)));
   if (dx<0) dx-=(SIZE_TILE_W-1);
   if (dy<0) dy-=(SIZE_TILE_H-1);
   dx/=SIZE_TILE_W;
   dy/=SIZE_TILE_H;

   if ((dx>=-PLAYER_OFF_X)&&(dx<=PLAYER_OFF_X)&&
       (dy>=-PLAYER_OFF_Y)&&(dy<=PLAYER_OFF_Y))
   {
      int tx=MAP_X(gPlayer.loc.x+dx);
      int ty=MAP_Y(gPlayer.loc.y+dy);
      sWorldMapTile *us=&gTile(tx,ty);
      char buf[64];
#ifdef PLAYTEST
      if (config_is_defined("overworld_rc_loc"))
         mprintf("RC at %d %d\n",tx,ty);
#endif      
      // is it the exotic armor?
      if (gPlayer.level>=LVL_WISE)
         if ((tx==gPlayer.exotic.x && ty==gPlayer.exotic.y)&&(abs(dx)<2)&&(abs(dy)<2))
         {  // can only uncover exotic armor if you are near it...
            us->treasure=LOOT_EXOTIC;  
            _ow_add_message_named("ow_find_loot_3");
            gPlayer.exotic.x=gPlayer.exotic.y=OW_EMPTY;
            return;    // you have found them, they are gone for good
         }

      // is it the player getting there own stats
      if (us->monster_id==PLAYER_MONSTER_IDX)
      {  // show combat stats for the player
         sMonsterStats *pPS=&gMonsterStat[MONSTER_PLAYER];
         _ow_add_message_named("ow_stats_intro");
         if (gPlayer.quest == OW_EMPTY)
         {
            GetNamedFormatString("ow_stats_ac");
            sprintf(buf,StrFormat(),pPS->armor);
         }
         else
         {
            GetNamedFormatString("ow_stats_acquest");
            sprintf(buf,StrFormat(),pPS->armor,gCityNames[gPlayer.quest]);
         }
         _ow_add_message(buf);
         GetNamedFormatString("ow_stats_dmg");
         sprintf(buf,StrFormat(),pPS->damage.num_dice,pPS->damage.die_size,pPS->damage.flat_add);
         _ow_add_message(buf);         
         GetNamedFormatString("ow_stats_thaco"); // need a real calculation here...
         sprintf(buf,StrFormat(),get_thaco(&pPS->tohit));
         _ow_add_message(buf);         
         return;
      }

      // if just terrain, print terrain name
      if (us->monster_id!=OW_EMPTY)
      {
         int mtype=gMonstType(us->monster_id);
         if ((mtype==MONSTER_DRAGON)&&(gPlayer.dragons>=1))
         {
            for (int i=0; i<NUM_ELEM_DRAGONS; i++)
               if (gPlayer.drag_ids[i]==us->monster_id)
               {
                  GetNamedStringNum(buf,"ow_id_drag_",i);
                  break;
               }
         }
         else
         {  // normal monster
            GetNamedFormatString("ow_id_monst");
            GetMonsterName(mtype);
            sprintf(buf,StrFormat(),StrMonst());
         }
         _ow_add_message(buf);
#ifdef PLAYTEST
         if (config_is_defined("overworld_rc_monsters"))
            mprintf("Monster %d: a %d flags %x hp %d\n",
                    us->monster_id,mtype,gMonst(us->monster_id).flags,gMonst(us->monster_id).hp);
#endif         
      }

      // analyze the treasure, if there is any
      if (us->treasure!=OW_EMPTY)
      {
         if (us->treasure<=LOOT_MAX_GOLD)
         {
            GetNamedFormatString("ow_id_loot");
            int amt=GetLootValue(tx,ty);
            sprintf(buf,StrFormat(),amt);
         }
         else
            GetNamedStringNum(buf,"ow_id_loot_",us->treasure);
         _ow_add_message(buf);
      }

      // if a camp, or there is no monster or treasure, id the terrain, 
      if (us->terrain==TERRAIN_CAMP||((us->monster_id==OW_EMPTY)&&(us->treasure==OW_EMPTY)))
      {
         char *terrname="ow_terrain_X", terr_buf[64];
         GetNamedFormatString("ow_id_terr");
         terrname[11]='0'+us->terrain;
         GetNamedString(terr_buf,terrname);
         sprintf(buf,StrFormat(),terr_buf);
         _ow_add_message(buf);         
         if ((us->terrain == TERRAIN_CITY) || (us->terrain == TERRAIN_VILLAGE))
            if (us->loc_id != OW_EMPTY)
            {
               GetNamedFormatString("ow_id_city");
               sprintf(buf,StrFormat(),gCityNames[us->loc_id]);
               _ow_add_message(buf);
            }
         if (us->terrain == TERRAIN_CAMP)
            if (us->loc_id != OW_EMPTY)
            {
               GetNamedFormatString("ow_id_camp");
               GetMonsterName(us->loc_id-LOC_MONSTER_BASE);
               sprintf(buf,StrFormat(),StrMonst());
               _ow_add_message(buf);
            }            
      }
   }  // @TBD: hook some cheat up to this... but WHAT??? - i know... magic spell...
   else
      Warning(("Right click Out of range\n"));
}

////////////////////////
// debug only stats table dumper, for us to tune with 

#ifdef DBG_ON
static int get_roll(sDieRoll *roll)
{
   int val=roll->flat_add;
   val+=(roll->num_dice*roll->die_size/2);
   return val;
}

void OW_DumpStats(void)
{
   // monsters first
   for (int i=1; i<MONSTER_NUM_TYPES; i++)  // skip the player and any talking monsters
      if ((gDefaultMonsterStat[i].start_flags&MONST_FLG_TALK)==0)
      {
         sMonsterStats *us=&gDefaultMonsterStat[i];
         int thaco=get_thaco(&us->tohit);
         float netd=get_roll(&us->damage)*thaco/100.0;
         char buf[128];
         GetMonsterName(i);
         sprintf(buf,"M %2.2d: Th %2.2d Hp %2.2d Dmg %2.2d NetD %4.4f Lvl %d Xp %2.2d Arm %d\n",
                 i,thaco,get_roll(&us->hitpoints),get_roll(&us->damage),netd,us->level,us->xp_val,us->armor);
      }
}
#endif      

////////////////////////
// key handler

static Rect key_rect = {{LMFD_X, LMFD_Y}, {LMFD_X + LMFD_W, LMFD_Y + LMFD_H}}; 

static bool rawkey_handler_func(uiEvent* ev, Region* r, void* data)
{
   Point pt;
   mouse_get_xy(&pt.x,&pt.y);
   if (!RectTestPt(&key_rect,pt))
      return(FALSE);

   ushort keycode;
   bool result;
   
   kbs_event kbe;
   kbe.code = ((uiRawKeyEvent*)ev)->scancode;
   kbe.state = ((uiRawKeyEvent*)ev)->action;

   kb_cook(kbe,&keycode,&result);

   BOOL taken = FALSE;
   int dx, dy;
   if (keycode&KB_FLAG_DOWN)
      switch(keycode&~KB_FLAG_DOWN)
      {
         case KEY_PAD_HOME:   dx = -1; dy = -1; taken = TRUE; break;
         case KEY_PAD_UP:     dx = 0;  dy = -1; taken = TRUE; break;
         case KEY_PAD_PGUP:   dx = 1;  dy = -1; taken = TRUE; break;
         case KEY_PAD_LEFT:   dx = -1; dy = 0;  taken = TRUE; break;
         case KEY_PAD_CENTER: dx = 0;  dy = 0;  taken = TRUE; break;
         case KEY_PAD_RIGHT:  dx = 1;  dy = 0;  taken = TRUE; break;
         case KEY_PAD_END:    dx = -1; dy = 1;  taken = TRUE; break;
         case KEY_PAD_DOWN:   dx = 0;  dy = 1;  taken = TRUE; break;
         case KEY_PAD_PGDN:   dx = 1;  dy = 1;  taken = TRUE; break;
      }

   if (taken)
   {
      if (WereMousingBadTime())
         return(TRUE);

      if ((dx != 0) || (dy != 0))
      {
         uchar new_x=MAP_X(gPlayer.loc.x+dx);
         uchar new_y=MAP_Y(gPlayer.loc.y+dy);
         PlayerMoveTo(new_x,new_y);
      }

#ifdef PLAYTEST
      if (!config_is_defined("overworld_slew"))
#endif         
         OW_RunSim();
      return(TRUE);
   }

   return(FALSE);
}

////////////////////////
// init and term

static int key_handler_id;
void OverWorldInit(void)
{
   int i;
   char temp[255];
   for (i=0; i < MONSTER_NUM_TYPES; i++)
      for (int j=0; j<MONSTER_FRAMES; j++)
      {
         sprintf(temp,"owmon%2.2d%c",i,'a'+j);
         gResMonster[i][j] = LoadPCX(temp);
      }
   for (i=0; i < TERRAIN_NUM_TYPES; i++)
   {
      sprintf(temp,"owterr%2.2d",i);
      gResTerrain[i] = LoadPCX(temp);
   }
   for (i=0; i < COMBAT_NUM_TYPES; i++)
   {
      sprintf(temp,"owcmbt%2.2d",i);
      gResAction[i] = LoadPCX(temp);
   }
   for (i=0; i < LOOT_NUM_TYPES; i++)
   {
      sprintf(temp,"owloot%2.2d",i);
      gResLoot[i] = LoadPCX(temp);
   }

   gLastModeChange=gLastInputEvent=GetSimTime();
   if (gCurGameMode==kGameInit)
   {  // create the world itself...
      CreateTheGameSpace();
      _ow_add_message_named("ow_entergame");
   }
   else
      _ow_add_message_named("ow_resume");

#ifdef DBG_ON   
   if (config_is_defined("overworld_stat_table"))
      OW_DumpStats();
#endif      

   Region* root = GetRootRegion();
   uiInstallRegionHandler(root,UI_EVENT_KBD_RAW,rawkey_handler_func,NULL,&key_handler_id);
   uiGrabFocus(root, UI_EVENT_KBD_RAW);
}

void OverWorldTerm(void)
{
   int i;
   for (i=0 ; i < MONSTER_NUM_TYPES; i++)
      for (int j=0; j<MONSTER_FRAMES; j++)
         SafeFreeHnd(&gResMonster[i][j]);
   for (i=0 ; i < TERRAIN_NUM_TYPES; i++)
      SafeFreeHnd(&gResTerrain[i]);
   for (i=0 ; i < COMBAT_NUM_TYPES; i++)
      SafeFreeHnd(&gResAction[i]);
   for (i=0 ; i < LOOT_NUM_TYPES; i++)
      SafeFreeHnd(&gResLoot[i]);

   Region* root = GetRootRegion();
   uiReleaseFocus(root, UI_EVENT_KBD_RAW);
   uiRemoveRegionHandler(root,key_handler_id);
}
