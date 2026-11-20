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
   return INTEGER_LITERAL;
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
all_dark_actors         { mschlval.ival=kNumDarkActorTypes; return INTEGER_LITERAL; }

humanoid                { mschlval.ival=kDATYPE_Humanoid; return INTEGER_LITERAL; }
playerLimb              { mschlval.ival=kDATYPE_PlayerLimb; return INTEGER_LITERAL; }
playerBowLimb           { mschlval.ival=kDATYPE_PlayerBowLimb; return INTEGER_LITERAL; }
burrick                 { mschlval.ival=kDATYPE_Burrick; return INTEGER_LITERAL; }
spider                  { mschlval.ival=kDATYPE_Spider; return INTEGER_LITERAL; }
constantine             { mschlval.ival=kDATYPE_Constantine; return INTEGER_LITERAL; }
sweel                   { mschlval.ival=kDATYPE_Sweel; return INTEGER_LITERAL; }
apparition              { mschlval.ival=kDATYPE_Apparition; return INTEGER_LITERAL; }
robot                   { mschlval.ival=kDATYPE_Robot; return INTEGER_LITERAL; }

/* Shock-specific tokens */
all_shock_actors        { mschlval.ival=kNumShockActorTypes; return INTEGER_LITERAL; }
droid                   { mschlval.ival=kShATYPE_Droid; return INTEGER_LITERAL; }
overlord                { mschlval.ival=kShATYPE_Overlord; return INTEGER_LITERAL; }
arachnid                { mschlval.ival=kShATYPE_Arachnid; return INTEGER_LITERAL; }

/* Deep Cover-specific tokens */
all_deepc_actors        { mschlval.ival=kNumDPCActorTypes; return INTEGER_LITERAL; }
olddog                  { mschlval.ival=kDPCATYPE_OldDog; return INTEGER_LITERAL; }
dog                     { mschlval.ival=kDPCATYPE_Dog; return INTEGER_LITERAL; }
deephuman               { mschlval.ival=kDPCATYPE_DeepHuman; return INTEGER_LITERAL; }

/* game-shared ai tokens */
tagval_set              { mschlval.ival=kMTV_set; return INTEGER_LITERAL; }

tagval_middle             { mschlval.ival=kMTV_middle; return INTEGER_LITERAL; }
tagval_left             { mschlval.ival=kMTV_left; return INTEGER_LITERAL; }
tagval_right             { mschlval.ival=kMTV_right; return INTEGER_LITERAL; }
tagval_front              { mschlval.ival=kMTV_front; return INTEGER_LITERAL; }
tagval_back              { mschlval.ival=kMTV_back; return INTEGER_LITERAL; }
tagval_high             { mschlval.ival=kMTV_high; return INTEGER_LITERAL; }
tagval_low             { mschlval.ival=kMTV_low; return INTEGER_LITERAL; }

tagval_forward             { mschlval.ival=kMTV_forward; return INTEGER_LITERAL; }
tagval_forwards            { mschlval.ival=kMTV_forward; return INTEGER_LITERAL; }
tagval_backward            { mschlval.ival=kMTV_backward; return INTEGER_LITERAL; }
tagval_backwards        { mschlval.ival=kMTV_backward; return INTEGER_LITERAL; }
tagval_upstairs        { mschlval.ival=kMTV_upstairs; return INTEGER_LITERAL; }
tagval_downstairs      { mschlval.ival=kMTV_downstairs; return INTEGER_LITERAL; }

tagval_swing_short      { mschlval.ival=kMTV_swing_short; return INTEGER_LITERAL; }
tagval_swing_medium     { mschlval.ival=kMTV_swing_medium; return INTEGER_LITERAL; }
tagval_swing_long       { mschlval.ival=kMTV_swing_long; return INTEGER_LITERAL; }

/* many of these are soon to be obsolete */
tagval_true             { mschlval.ival=kMTV_true; return INTEGER_LITERAL; }
tagval_fast             { mschlval.ival=kMTV_fast; return INTEGER_LITERAL; }
tagval_stationary       { mschlval.ival=kMTV_stationary; return INTEGER_LITERAL; }

tagval_perpleft         { mschlval.ival=kMTV_perpleft; return INTEGER_LITERAL; }
tagval_perpright        { mschlval.ival=kMTV_perpright; return INTEGER_LITERAL; }

tagval_searching        { mschlval.ival=kMTV_searching; return INTEGER_LITERAL; }
tagval_swordmelee       { mschlval.ival=kMTV_swordmelee; return INTEGER_LITERAL; }
tagval_alert            { mschlval.ival=kMTV_alert; return INTEGER_LITERAL; }

tagval_swing            { mschlval.ival=kMTV_swing; return INTEGER_LITERAL; }
tagval_dodge            { mschlval.ival=kMTV_dodge; return INTEGER_LITERAL; }
tagval_parry            { mschlval.ival=kMTV_parry; return INTEGER_LITERAL; }

tagval_pose_calib       { mschlval.ival=kMTV_pose_calib; return INTEGER_LITERAL; }
tagval_pose_swordready  { mschlval.ival=kMTV_pose_swordready; return INTEGER_LITERAL; }

tagval_gesture          { mschlval.ival=kMTV_gesture; return INTEGER_LITERAL; }
tagval_hurt             { mschlval.ival=kMTV_hurt; return INTEGER_LITERAL; }
tagval_stunned          { mschlval.ival=kMTV_stunned; return INTEGER_LITERAL; }
tagval_surprised        { mschlval.ival=kMTV_surprised; return INTEGER_LITERAL; }
tagval_alerttrans1     { mschlval.ival=kMTV_alerttrans1; return INTEGER_LITERAL; }
tagval_alerttrans2     { mschlval.ival=kMTV_alerttrans2; return INTEGER_LITERAL; }
tagval_alerttrans3     { mschlval.ival=kMTV_alerttrans3; return INTEGER_LITERAL; }
tagval_die              { mschlval.ival=kMTV_die; return INTEGER_LITERAL; }

tagval_hurt_light       { mschlval.ival=kMTV_hurt_light; return INTEGER_LITERAL; }
tagval_hurt_heavy       { mschlval.ival=kMTV_hurt_heavy; return INTEGER_LITERAL; }

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
      return INTEGER_LITERAL;
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
   return INTEGER_LITERAL;
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



