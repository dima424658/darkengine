/* $Header: r:/t2repos/thief2/src/songlang.y,v 1.3 1999/03/18 14:31:35 mwhite Exp $ */

%token SONG
%token DECLARESECTIONS
/*
%token DECLAREEVENTS
*/
%token SECTION
%token SAMPLE
%token SILENCE
%token SECONDS
%token SAMPLES
%token LOOPCOUNT
%token IMMEDIATE
%token PROBABILITY
%token ONEVENT
%token GOTOSECTION
%token GOTOEND
%token VOLUME
%token PAN
/*
%token TO
*/
%token <intval> INT_NUM
%token <intval> PERCENT_NUM
%token <floatval> FLOAT_NUM
%token <strval> QSTRING
%token <strval> ID

%type <intval> loopcount
%type <intval> probability
%type <intval> volume

%union
{
   char *strval;
   int  intval;
   float floatval;
}

%start song

%{

#include <mprintf.h>
#include <song.h>

static ISong* sgpCurrSong;
static ISongSection* sgpCurrSection; 
static ISongSample* sgpCurrSample;
static ISongEvent* sgpCurrEvent;
static ISongGoto* sgpCurrGoto;

static BOOL sgInSections;

static unsigned _FindSectionIndexFromID (ISong* pSong, char* id)
{
   ISongSection* pSection;
   sSongSectionInfo sectionInfo;
   int index;
   unsigned i;
   unsigned count;

   /* Do linear search to match ID. */
   index = -1;
   count = ISong_CountSections (sgpCurrSong);
   for (i = 0; i < count; i++)
   {
      ISong_GetSection (pSong, i, &pSection);
      ISongSection_GetSectionInfo (pSection, &sectionInfo);
      ISongSection_Release (pSection);

      if (!strcmp (&(sectionInfo.id), id))
      {
         index = i;
         break;
      }
   }

   return (unsigned) index;
}

static BOOL _StringLengthErrorCheck (char* pStr)
{
   char tmpStr[100];

   if (strlen (pStr) >= kSONG_MaxStringLen)
   {
      sprintf (tmpStr, "String must be less than %d characters long.", kSONG_MaxStringLen);
      songyyerror (tmpStr);
      return TRUE;
   }

   return FALSE;
}

%}

%%

song: SONG ID declarations onevents sections
      {
         sSongInfo songInfo;

         /* Error on string too long. */
         if (_StringLengthErrorCheck ($2))
            SONGYYABORT;

         ISong_GetSongInfo (sgpCurrSong, &songInfo);
         strcpy (songInfo.id, $2);
         ISong_SetSongInfo (sgpCurrSong, &songInfo);
      }
|     SONG error
      {
         songyyerror ("Song parsing failure.  Aborting.");
         SONGYYABORT;
      }
|     error
      {
         songyyerror ("File parsing failure.  Aborting.");
         SONGYYABORT;
      }
;

declarations: declaresections
;

declaresections: DECLARESECTIONS sectiondeclarationlist
|     DECLARESECTIONS error
      {
         songyyerror ("DeclareSections parsing failure.  Aborting.");
         SONGYYABORT;
      }
;

sectiondeclarationlist: sectiondeclaration
|       sectiondeclarationlist sectiondeclaration
;

sectiondeclaration: ID
      {
         sSongSectionInfo sectionInfo;
         ISongSection* pSection;

         /* Error on string too long */
         if (_StringLengthErrorCheck ($1))
            SONGYYABORT;

         /* Create a new section and place it in the song.  It is a placeholder to
          * match up against later when a section is actually defined. */
         memset (&sectionInfo, 0, sizeof (sectionInfo));
         strcpy (sectionInfo.id, $1);

         /* Add the section declaration to the song. */
         CreateSongSection (&pSection, NULL);
         ISongSection_SetSectionInfo (pSection, &sectionInfo);
         ISong_AddSection (sgpCurrSong, pSection);
         ISongSection_Release (pSection);
      }
;

sections: section
|         sections section
;

section: SECTION ID sectionoptions samplesorsilence onevents
      {
         int index;
         sSongSectionInfo sectionInfo;

         /* Set the ID */
         /* Error on string too long. */
         if (_StringLengthErrorCheck ($2))
            SONGYYABORT;

         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         strcpy (sectionInfo.id, $2);
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);

         /* Look up the section.  The ID should match one in the song if it
          * has been declared. */
         index = _FindSectionIndexFromID (sgpCurrSong, $2);
         if (index == -1)
         {
            songyyerror("Section not declared.");
            SONGYYABORT;
         }

         /* Assign the section. */
         ISong_SetSection (sgpCurrSong, index, sgpCurrSection);

         /* Reset section. */
         ISongSection_Release (sgpCurrSection);
         CreateSongSection (&sgpCurrSection, NULL);
      }
|        SECTION error
      {
         songyyerror ("Section parsing failure.  Aborting.");
         SONGYYABORT;
      }
;

samplesorsilence: samples
|                 silence
;

sectionoptions: /* empty */
      {
         /* Kind of tricky : This will get matched before events in a section.  So, this
          * flag is set here so we can discern between an "OnEvent" which is at the song
          * level vs. one at the section level. */
         sgInSections = TRUE;
      }
|               sectionoptions volume
      {
         /* Set the volume for the current section. */
         sSongSectionInfo sectionInfo;
         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         sectionInfo.volume = $2;
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);
      }
|
                sectionoptions loopcount
      {
         /* Set the loopsount for the current section. */
         sSongSectionInfo sectionInfo;
         ISongSection_GetSectionInfo (sgpCurrSection, &sectionInfo);
         sectionInfo.loopCount = $2;
         ISongSection_SetSectionInfo (sgpCurrSection, &sectionInfo);
      }
;

volume: VOLUME INT_NUM
      {  $$ = $2; }
;

samples: sample
|        samples sample
;

sample: SAMPLE QSTRING /* sampleoptions */
      {
         sSongSampleInfo sampleInfo;

         /* Error on string too long. */
         if (_StringLengthErrorCheck ($2))
            SONGYYABORT;

         ISongSample_GetSampleInfo (sgpCurrSample, &sampleInfo);
         strcpy (sampleInfo.name, $2);
         ISongSample_SetSampleInfo (sgpCurrSample, &sampleInfo);

         /* Add sample to current section. */
         ISongSection_AddSample (sgpCurrSection, sgpCurrSample);

         /* Reset globals. */
         ISongSample_Release (sgpCurrSample);
         CreateSongSample (&sgpCurrSample, NULL);
      }
|       SAMPLE error
      {
         songyyerror ("Sample parsing error.  Aborting.");
         SONGYYABORT;
      }
;

/*
sampleoptions: /* empty */
/*|              sampleoptions loopcount
      {
         sSongSampleInfo sampleInfo;

         /* Set the sample's loopcount. */ 
/*         ISongSample_GetSampleInfo (sgpCurrSample, &sampleInfo);
         sampleInfo.loopCount = $2;
         ISongSample_SetSampleInfo (sgpCurrSample, &sampleInfo);
      }
|              sampleoptions probability
      {
         sSongSampleInfo sampleInfo;

         /* Set the sample's probability. */
/*         ISongSample_GetSampleInfo (sgpCurrSample, &sampleInfo);
         sampleInfo.probability = $2;
         ISongSample_SetSampleInfo (sgpCurrSample, &sampleInfo);
      }
;
*/

silence : SILENCE FLOAT_NUM SECONDS
      {
         songyyerror ("Silence not implemented.");
      }
;

loopcount: LOOPCOUNT INT_NUM
      {  $$ = $2; }
;

probability: PROBABILITY PERCENT_NUM
      {  $$ = $2; }
;

onevents: /* empty */
|         onevents onevent
;

onevent: ONEVENT QSTRING oneventoptions gotosections
      {
         sSongEventInfo eventInfo;

         /* Error on string too long. */
         if (_StringLengthErrorCheck ($2))
            SONGYYABORT;

         /* Set the event string */
         ISongEvent_GetEventInfo (sgpCurrEvent, &eventInfo);
         strcpy (eventInfo.eventString, $2);
         ISongEvent_SetEventInfo (sgpCurrEvent, &eventInfo);

         if (sgInSections)
            /* Add the event to the current section. */
            ISongSection_AddEvent (sgpCurrSection, sgpCurrEvent);
         else
            ISong_AddEvent (sgpCurrSong, sgpCurrEvent);
         
         /* Reset globals. */
         ISongEvent_Release (sgpCurrEvent);
         CreateSongEvent (&sgpCurrEvent, NULL);
      }
|        ONEVENT error
      {
         songyyerror ("Event parsing failure.  Aborting.");
         SONGYYABORT;
      }
;

oneventoptions: /* empty */
|               oneventoptions IMMEDIATE
      {
         sSongEventInfo eventInfo;

         ISongEvent_GetEventInfo (sgpCurrEvent, &eventInfo);
         eventInfo.flags |= kSONG_EventFlagImmediate;
         ISongEvent_SetEventInfo (sgpCurrEvent, &eventInfo);
      }
;

gotosections: gotosection
|             gotosections gotosection
;
               
gotosection: GOTOSECTION ID probability
      {
         int index;
         sSongGotoInfo gotoInfo;

         /* Error on string too long. */
         if (_StringLengthErrorCheck ($2))
            SONGYYABORT;
         
         /* Match the goto's ID with declared ones and add to the event. */
         index = _FindSectionIndexFromID (sgpCurrSong, $2);
         if (index == -1)
         {
            songyyerror ("Section undeclared.");
            SONGYYABORT;
         }

         /* Set the info. */
         ISongGoto_GetGotoInfo (sgpCurrGoto, &gotoInfo);
         gotoInfo.probability       = $3;
         gotoInfo.sectionIndex      = index;
         ISongGoto_SetGotoInfo (sgpCurrGoto, &gotoInfo);

         /* Add the goto to the event. */
         ISongEvent_AddGoto (sgpCurrEvent, sgpCurrGoto);

         /* Reset globals. */
         ISongGoto_Release (sgpCurrGoto);
         CreateSongGoto (&sgpCurrGoto, NULL);
      }
|            GOTOEND probability
      {
         int index;
         sSongGotoInfo gotoInfo;
         
         /* End index is 1 past last section. */
         index = ISong_CountSections (sgpCurrSong);

         /* Set the info. */
         ISongGoto_GetGotoInfo (sgpCurrGoto, &gotoInfo);
         gotoInfo.probability       = $2;
         gotoInfo.sectionIndex      = index;
         ISongGoto_SetGotoInfo (sgpCurrGoto, &gotoInfo);

         /* Add the goto to the event. */
         ISongEvent_AddGoto (sgpCurrEvent, sgpCurrGoto);

         /* Reset globals. */
         ISongGoto_Release (sgpCurrGoto);
         CreateSongGoto (&sgpCurrGoto, NULL);
      }
|            GOTOSECTION error
      {
         songyyerror ("GotoSection parsing failure.  Aborting.");
         SONGYYABORT;
      }
|
             GOTOEND error
      {
         songyyerror ("GotoEnd parsing failure. Aborting.");
         SONGYYABORT;
      }
;

%%

// From the lexer... 
extern BOOL SongYyParseFile(char *songFile);

// Returns TRUE if successful and sets *ppSong to the song that was parsed.
// Returns FALSE upon failure and sets *ppSong to NULL.
BOOL SongYaccParse (char* songFile, ISong **ppSong)
{
   BOOL parseOK;

   /* Initialize static globals. */
   CreateSong (&sgpCurrSong, NULL);
   CreateSongSection (&sgpCurrSection, NULL);
   CreateSongSample (&sgpCurrSample, NULL);
   CreateSongEvent (&sgpCurrEvent, NULL);
   CreateSongGoto (&sgpCurrGoto, NULL);

   /* Need to know if parsing a section to determine where to add events to:
    * the song or the section.  This is to support default OnEvents.
    */
   sgInSections = FALSE;

   /* Parse the file. */
   parseOK = SongYyParseFile(songFile);

   /* Clean up. */
   SafeRelease (sgpCurrGoto);
   SafeRelease (sgpCurrEvent);
   SafeRelease (sgpCurrSample);
   SafeRelease (sgpCurrSection);

   if (parseOK)
   {
      /* Don't release the song... return it. */
      *ppSong = sgpCurrSong;
   }
   else
   {
      /* We failed, so release the song and give back a NULL song. */
      SafeRelease (sgpCurrSong);
      *ppSong = NULL;
   }

   return parseOK;
}
