%{
#include <stdlib.h>
#include <mschtok.h>
#include <dbg.h>
#include <cfgdbg.h>
#include <mtagvals.h>
#include <drkcret.h> /* for actor types */
#include <shkcret.h> /* for actor types */
#include <dpccret.h> /* for Deep Cover actor types */

#ifdef DEBUG
#include <memall.h>
#include <dbmem.h>

#define local_strdup(s) (strcpy((char *)(malloc(strlen(s) + 1)), (s)))
#else
#define local_strdup(s) strdup(s)
#endif

%}

alpha  [a-zA-Z]
alphanum [a-zA-Z0-9\.]
digit    [0-9]
firstchar [a-zA-Z_]
followchar [a-zA-Z0-9_]
quote [\"]
string [^\"]
lparen "("
rparen ")"
colon ":"
astring [^<^>^\n]
comment "//"
dot "."

%x PREPROC
%x COMMENT
%e 2000  /* so don't run out of NFA space */
%n 600   /* and don't run out of DFA space */

%%

[\t ]+        /* ignore whitespace */;

/* inline comments */
{comment} {
   ConfigSpew("lexspew", ("schlex: comment\n"));
   BEGIN COMMENT;
}

<COMMENT>\n {
   ConfigSpew("lexspew", ("schlex: end comment\n"));
   BEGIN INITIAL;
}

^"#include"     {
   ConfigSpew("lexspew", ("schlex: include\n"));
   BEGIN PREPROC;
   return INCLUDE;
}
^"#define"      {
   ConfigSpew("lexspew", ("schlex: define\n"));
   BEGIN PREPROC;
   return DEFINE;
}

<PREPROC>\n     {
   ConfigSpew("lexspew", ("schlex: return\n"));
   BEGIN INITIAL;
}

<PREPROC>{firstchar}{followchar}* {
   ConfigSpew("lexspew", ("schlex: ident\n"));
   mschlval.strval = local_strdup(mschtext);
   return IDENT;
}
<PREPROC>-?{digit}+ {
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.ival = atoi(mschtext);
   return INT;
}
<PREPROC>{quote}{string}*{quote} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}
<PREPROC>{lparen}{astring}*{rparen} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}

/*  define tokens here */

schema                  {ConfigSpew("lexspew", ("schlex: schema\n")); return SCHEMA;};
tag                     return TAG;
opt                     return TAG_OPT;
archetype               return ARCHETYPE;
motion                  return MOTION;
actor                   return ACTOR;
actors_declare          return ACTORS_DECLARE;
tags:                   return TAGLIST_HEADER;
motions:                return MOTLIST_HEADER;
tags                    return TAGLIST_HEADER;
motions                 return MOTLIST_HEADER;
stretch                 return STRETCH;
distance                return DISTANCE;
timewarp                return TIMEWARP;
duration                return DURATION;
blend_none              return MOT_BLEND_NONE;
blend_default           return MOT_BLEND_DEFAULT;
blend                   return MOT_BLEND_LENGTH;
neck_is_fixed           return MOT_NECK_FIXED;
neck_not_fixed          return MOT_NECK_NOT_FIXED;
is_turn                 return MOT_IS_TURN;
is_loco                 return MOT_IS_LOCO;
in_place                return MOT_IN_PLACE;                 

/* Dark-specific tokens */
all_dark_actors         { mschlval.ival=kNumDarkActorTypes; return INT; }

humanoid                { mschlval.ival=kDATYPE_Humanoid; return INT; }
playerLimb              { mschlval.ival=kDATYPE_PlayerLimb; return INT; }
playerBowLimb           { mschlval.ival=kDATYPE_PlayerBowLimb; return INT; }
burrick                 { mschlval.ival=kDATYPE_Burrick; return INT; }
spider                  { mschlval.ival=kDATYPE_Spider; return INT; }
constantine             { mschlval.ival=kDATYPE_Constantine; return INT; }
sweel                   { mschlval.ival=kDATYPE_Sweel; return INT; }
apparition              { mschlval.ival=kDATYPE_Apparition; return INT; } 
robot                   { mschlval.ival=kDATYPE_Robot; return INT; }

/* Shock-specific tokens */
all_shock_actors        { mschlval.ival=kNumShockActorTypes; return INT; }
droid                   { mschlval.ival=kShATYPE_Droid; return INT; }
overlord                { mschlval.ival=kShATYPE_Overlord; return INT; }
arachnid                { mschlval.ival=kShATYPE_Arachnid; return INT; }

/* Deep Cover-specific tokens */
all_deepc_actors        { mschlval.ival=kNumDPCActorTypes; return INT; }
olddog                  { mschlval.ival=kDPCATYPE_OldDog; return INT; }
dog                     { mschlval.ival=kDPCATYPE_Dog; return INT; }
deephuman               { mschlval.ival=kDPCATYPE_DeepHuman; return INT; }

/* game-shared ai tokens */
tagval_set              { mschlval.ival=kMTV_set; return INT; }

tagval_middle             { mschlval.ival=kMTV_middle; return INT; }
tagval_left             { mschlval.ival=kMTV_left; return INT; }
tagval_right             { mschlval.ival=kMTV_right; return INT; }
tagval_front              { mschlval.ival=kMTV_front; return INT; }
tagval_back              { mschlval.ival=kMTV_back; return INT; }
tagval_high             { mschlval.ival=kMTV_high; return INT; }
tagval_low             { mschlval.ival=kMTV_low; return INT; }

tagval_forward             { mschlval.ival=kMTV_forward; return INT; }
tagval_forwards            { mschlval.ival=kMTV_forward; return INT; }
tagval_backward            { mschlval.ival=kMTV_backward; return INT; }
tagval_backwards        { mschlval.ival=kMTV_backward; return INT; }
tagval_upstairs        { mschlval.ival=kMTV_upstairs; return INT; }
tagval_downstairs      { mschlval.ival=kMTV_downstairs; return INT; }

tagval_swing_short      { mschlval.ival=kMTV_swing_short; return INT; }
tagval_swing_medium     { mschlval.ival=kMTV_swing_medium; return INT; }
tagval_swing_long       { mschlval.ival=kMTV_swing_long; return INT; }

/* many of these are soon to be obsolete */
tagval_true             { mschlval.ival=kMTV_true; return INT; }
tagval_fast             { mschlval.ival=kMTV_fast; return INT; }
tagval_stationary       { mschlval.ival=kMTV_stationary; return INT; }

tagval_perpleft         { mschlval.ival=kMTV_perpleft; return INT; }
tagval_perpright        { mschlval.ival=kMTV_perpright; return INT; }

tagval_searching        { mschlval.ival=kMTV_searching; return INT; }
tagval_swordmelee       { mschlval.ival=kMTV_swordmelee; return INT; }
tagval_alert            { mschlval.ival=kMTV_alert; return INT; }

tagval_swing            { mschlval.ival=kMTV_swing; return INT; }
tagval_dodge            { mschlval.ival=kMTV_dodge; return INT; }
tagval_parry            { mschlval.ival=kMTV_parry; return INT; }

tagval_pose_calib       { mschlval.ival=kMTV_pose_calib; return INT; }
tagval_pose_swordready  { mschlval.ival=kMTV_pose_swordready; return INT; }

tagval_gesture          { mschlval.ival=kMTV_gesture; return INT; }
tagval_hurt             { mschlval.ival=kMTV_hurt; return INT; }
tagval_stunned          { mschlval.ival=kMTV_stunned; return INT; }
tagval_surprised        { mschlval.ival=kMTV_surprised; return INT; }
tagval_alerttrans1     { mschlval.ival=kMTV_alerttrans1; return INT; }
tagval_alerttrans2     { mschlval.ival=kMTV_alerttrans2; return INT; }
tagval_alerttrans3     { mschlval.ival=kMTV_alerttrans3; return INT; }
tagval_die              { mschlval.ival=kMTV_die; return INT; }        

tagval_hurt_light       { mschlval.ival=kMTV_hurt_light; return INT; }
tagval_hurt_heavy       { mschlval.ival=kMTV_hurt_heavy; return INT; }

/* generic stuff */

{firstchar}{followchar}* {
   int val;

   ConfigSpew("lexspew", ("schlex: ident\n"));
   if (!IncTabLookup(mschtext, &val))
   {
      mschlval.strval = local_strdup(mschtext);
      return IDENT;
   }
   else
   {
      mschlval.ival = val;
      return INT;
   }
}

{quote}{string}*{quote} {
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}

-?{digit}+ {
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.ival = atoi(mschtext);
   return INT;
}

-?({digit}+)|({digit}*{dot}{digit}+)|({digit}+{dot}{digit}*) {
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.fval = atof(mschtext);
   return FLOAT;
}

= return EQUAL;
{colon} return COLON;
{lparen} return LPAREN;
{rparen} return RPAREN;



.               /* ignore */;

%%

BOOL OpenMschin(char *file_name)
{
   FILE *in_file;

   if (mschin != NULL)
      fclose(mschin);
   if (!(in_file = fopen(file_name, "r")))
   {
      Warning(("OpenMschIn: can't open file %s for reading\n",
               file_name));
      return FALSE;
   }
   mschin = in_file;
   return TRUE;
}

extern int mschparse();

void MschParseFile(char *schemaFile)
{
   mschin = NULL;
   msch_reset();
   if (!OpenMschin(schemaFile))
      return;
   while (!feof(mschin))
      mschparse();
   fclose(mschin);
}

int mschwrap(void) { return 1;}



