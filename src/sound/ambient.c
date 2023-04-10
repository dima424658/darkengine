/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambient.c,v 1.29 1999/04/26 15:12:00 mwhite Exp $
// ambient sound object initial cut
// actual system which do things sound things
// ambprop has the prop sys side

#include <stdlib.h>
#include <string.h>

#include <mprintf.h>
#include <timer.h>
#include <config.h>
#include <matrix.h>
#include <appagg.h>

#include <objremap.h>
#include <schbase.h>
#include <schema.h>
#include <objpos.h>
#include <tagfile.h>
#include <vernum.h>
#include <wrtype.h>
#include <simtime.h>
#include <dmgmodel.h>

#include <ambbase.h>
#include <ambient_.h>
#include <rand.h>

#include <dbmem.h>

#include <songutil.h>

// @TODO: this is the only thing bound by MAX_AMB_OBJS
AmbientRunTime  ambRTData[MAX_AMB_OBJS];

#ifdef PLAYTEST
bool ambient_mono=FALSE, ambient_heartbeat=FALSE;
 int amb_heartbeat_count=0;
#define amb_info(x)  do { if (ambient_mono) mprintf x ; } while (0)
#define amb_info2(x) do { if (ambient_mono) mprintf x ; } while (0)
#define amb_hbeat()  if (ambient_heartbeat&&(((++amb_heartbeat_count)&0x7f)==0)) ShowHeartbeatStats()
#define _amb_xtra_clear(hnd) \
 do { \
    if (hnd!=NO_CUR_HANDLE) \
       mprintf("Ambient: inloop Halt didnt null handle for %x\n",hnd); \
    hnd=NO_CUR_HANDLE; \
 } while (0)
#else
#define amb_info(x)
#define amb_info2(x)
#define amb_hbeat()
#define _amb_xtra_clear(h)
#endif

#define AMB_ENV_INACTIVE (-1)

// this is dorky - i really want to make entry 0 in our list always the current environment
// and not let any objects get assigned to it, but im not sure how to do that
int            cur_env_idx = AMB_ENV_INACTIVE, last_env_idx=AMB_ENV_INACTIVE;
int            hAuxSch1, hAuxSch2;

//AmbientSound   curEnv;   // track the current "environment"

// @HACK for beta milestone, so we don't play in edit mode
ObjID          restore_obj = OBJ_NULL;

static int sgCurrSongThemeIdx;

#define _ambsnd_handle(i)   (ambRTData[i].schemaHandle)
#define _ambsnd_lastplay(i) (ambRTData[i].last_played)

#define AMB_ENV_PDATA     (-1)
#define AMB_ENV_AUX_PDATA (-2)

#define NO_CUR_HANDLE  SCH_HANDLE_NULL
#define MIN_REFIRE     1000

// setup the initial data 'n such...
void AmbientRunTimeInit(void)
{
   int i;

   for (i=0; i<MAX_AMB_OBJS; i++)
   {
      _ambsnd_handle(i)=NO_CUR_HANDLE;
      _ambsnd_lastplay(i)=0;
   }
   hAuxSch1=hAuxSch2=NO_CUR_HANDLE;
}         // no currently running handle either

void AmbientReset()
{
#ifndef SHIP   
   int i;
   for (i=0; i<MAX_AMB_OBJS; i++)
   {
      if (_ambsnd_handle(i)!=NO_CUR_HANDLE)
      {
         mprintf("Warning... currently slot %d has handle %x\n",i,_ambsnd_handle(i));
         if (i==cur_env_idx)
            mprintf("Warning... this is the current environment schema\n");
         else if (i==last_env_idx)
            mprintf("Warning... this is the last environment schema\n");
      }
      _ambsnd_handle(i)=NO_CUR_HANDLE;          
      _ambsnd_lastplay(i)=0;
   }
   if (hAuxSch1!=NO_CUR_HANDLE)
      mprintf("Warning... currenty have active auxSch1 %x\n",hAuxSch1);
   if (hAuxSch2!=NO_CUR_HANDLE)
      mprintf("Warning... currenty have active auxSch2 %x\n",hAuxSch2);
#endif   
   AmbientRunTimeInit();
}

#ifdef PLAYTEST
static void ShowHeartbeatStats(void)
{
   int i;
   if (cur_env_idx!=AMB_ENV_INACTIVE)
      mprintf("CurEnv is %s\n",ambState(cur_env_idx)->schema_name.text);
   for (i=0; i<MAX_AMB_OBJS; i++)
   {
      if (_ambsnd_handle(i)!=NO_CUR_HANDLE)
      {
         AmbientSound *state=ambState(i);
         mprintf("Amb slot %d: currently %x [%s]",i,_ambsnd_handle(i),state?state->schema_name.text:"Unknown");
         if (i==cur_env_idx)
            mprintf(" current env");
         else if (i==last_env_idx)
            mprintf(" last env");
         mprintf("\n");
      }
   }
   if (hAuxSch1!=NO_CUR_HANDLE)
      mprintf("Amb AuxSch1 currently %x\n",hAuxSch1);
   if (hAuxSch2!=NO_CUR_HANDLE)
      mprintf("Amb AuxSch2 currently %x\n",hAuxSch2);
}
#endif

// if we get kill callback
static void _AmbSchemaKillCallback(int hSchema, ObjID schemaID, void *pData)
{
   int idx=(int)pData;
   if (idx==AMB_ENV_AUX_PDATA)  // if secret hacked aux thing, deal and get out
   {
      if (hSchema==hAuxSch1)
      {
         amb_info(("schema kill of aux env 1 (%x)\n",hSchema));
         hAuxSch1=NO_CUR_HANDLE;
      }
      else if (hSchema==hAuxSch2)
      {
         amb_info(("schema kill of aux env 2 (%x)\n",hSchema));
         hAuxSch2=NO_CUR_HANDLE;
      }
      else
         Warning(("Ambient: Got kill event for aux sample im not using!!\n"));
      return;
   }
   if (idx==AMB_ENV_PDATA)
   {
      if (hSchema==_ambsnd_handle(cur_env_idx))
         idx=cur_env_idx;
      else if (hSchema==_ambsnd_handle(last_env_idx))
         idx=last_env_idx;
      else
         mprintf("Amb seeing ENV handle (%x) but isnt cur or last\n",hSchema);
   }
   // if we make it here, it is primary environment or an object
#ifdef PLAYTEST
   else if (idx>=0)
      if (hSchema!=_ambsnd_handle(idx))
         mprintf("AmbSchemaKillCallback:  idx v. handle mismatch (idx %d gave %x v. %x)\n",idx,_ambsnd_handle(idx),hSchema);
#endif
   if (idx>=0)
   {
      AmbientSound *state=ambState(idx);
#ifdef PLAYTEST
      if (idx==cur_env_idx)
         amb_info(("schema kill active environment (idx %d) %s (%x)\n",idx,ambState(cur_env_idx)->schema_name.text,hSchema));
      else if (idx==last_env_idx)
         amb_info(("schema kill last environment (idx %d) (%x)\n",idx,hSchema));
      else
         amb_info(("schema kill of object ambient (idx %d) %s (%x)\n",idx,ambState(idx)->schema_name.text,hSchema));
#endif
      _ambsnd_handle(idx)=NO_CUR_HANDLE;  // look, im done playing
      if (idx==cur_env_idx)
         cur_env_idx=AMB_ENV_INACTIVE;
      else if (idx==last_env_idx)
         last_env_idx=AMB_ENV_INACTIVE;

      if (state)
      {
         if (state->flags&AMBFLG_S_KILLOBJ)
         {
            IDamageModel *pDmgModel=AppGetObj(IDamageModel);
            IDamageModel_SlayObject(pDmgModel,ambObjID(idx),OBJ_NULL);
            SafeRelease(pDmgModel);
         }
         else if (state->flags&AMBFLG_S_REMOVE)
            ambRemove(idx);
         else if (state->flags&AMBFLG_S_AUTOOFF)
            ambSetFlags(idx,state->flags|AMBFLG_S_TURNEDOFF);
      }
#ifdef PLAYTEST
      else
         mprintf("AmbSchemaKillCallback: NULL state for idx %d\n",idx);
#endif      
   }
#ifdef PLAYTEST   
   else
      amb_info(("Schema kill of nothing? (idx %d) (schema %x)\n", idx, hSchema));
#endif   
}

// default callback
static sSchemaCallParams schemaCallData={0,0,OBJ_NULL,NULL,0,_AmbSchemaKillCallback,0,NULL};

int _AmbientRestore();

// for now, we boneheadly scan the whole area every frame
// really, we should cache for each object how far away it was
// things greater than 2x rad can update every couple seconds
// as opposed to checking every frame
void AmbientRunFrame(mxs_vector *head_pos)  // where is the head?
{  
   ulong sim_time=tm_get_millisec();  // should probably use sim-time, eh?
   int i, near_env_idx=AMB_ENV_INACTIVE;
   float near_env_dist=1e20;
   float rad;

   for (i=0; i< ambMax(); i++)
      if (ambObjID(i)!=OBJ_NULL)
      {  // go through and check ranges for each object from the head
         ObjPos* pos = ObjPosGet(ambObjID(i));
         AmbientSound* state = ambState(i);
         float realdist;
         mxs_vector diff;

         // Song hack.
         if ( state->flags & AMBFLG_S_MUSIC )
         {
            if (i != sgCurrSongThemeIdx)
            {
               // Check to see if head is inside sphere.
               rad = (float)state->rad;
               mx_sub_vec(&diff,&pos->loc.vec,head_pos);
               if ( mx_mag2_vec(&diff) < (rad * rad) )
               {
                  // Head is inside sphere... Change theme.
                  sgCurrSongThemeIdx = i;
                  SongUtilSetTheme ( state->schema_name.text );
               }
            }
            continue; // Sorry.. I never use these, but it's easier here than a big "else".
         }

         if (state->flags&AMBFLG_S_TURNEDOFF)
         {  // if ive been turned off, make sure im not active
            if (_ambsnd_handle(i)!=NO_CUR_HANDLE)
            {  // if we are currently playing
               if (i==cur_env_idx)
               {
                  SchemaPlayHalt(_ambsnd_handle(i));
                  if (hAuxSch1!=NO_CUR_HANDLE) SchemaPlayHalt(hAuxSch1);
                  if (hAuxSch2!=NO_CUR_HANDLE) SchemaPlayHalt(hAuxSch2);
               }
               else
                  SchemaPlayHalt(_ambsnd_handle(i));
            }
            continue;
         }

         mx_sub_vec(&diff,&pos->loc.vec,head_pos);

#define RAD_MUL 1.05
         rad = (float)state->rad;

         if (((state->flags&AMBFLG_S_ENVIRON)==0)&&(_ambsnd_handle(i)!=NO_CUR_HANDLE))
         {  // we are already playing - but have we gone too far away 
            if ( mx_mag2_vec(&diff)*RAD_MUL > ( rad * rad ) )
            {
               SchemaPlayHalt(_ambsnd_handle(i));
               amb_info(("Killing local ambient (idx %d) %s (%x)\n",
                         i,state->schema_name.text,_ambsnd_handle(i)));
               _amb_xtra_clear(_ambsnd_handle(i));
            }
         }
         else
            // This "quick test" is apparently a mistake: should be "abs(diff.x)+abs(diff.y)+abs(diff.z)",
            // but abs is roughly as expensive as actually doing the squaring.  The test happens to not
            // actually break code, and shortcuts some cases, but in general is misleading and inefficient.
            // (It's additional computation that generally becomes "if (true)").
            //if ((diff.x+diff.y+diff.z)<(float)(state->rad<<1))  // may be worth considering
               if ( ( realdist=mx_mag2_vec(&diff)*RAD_MUL ) < ( rad *  rad ) )
                  if (state->flags&AMBFLG_S_ENVIRON)
                  {  // within radius of an environmental ambient object
                     if (realdist<near_env_dist)
                     {
                        near_env_dist=realdist;
                        near_env_idx=i;
                     }
                  }
                  else if (sim_time>_ambsnd_lastplay(i)+MIN_REFIRE)
                  {  // check to make sure we dont refire within time x passed
                     schemaCallData.flags    = SCH_SET_OBJ|SCH_SET_CALLBACK|SCH_RADIUS_VOLUME;
                     if ((state->flags&AMBFLG_S_NOSHARPCURVE)==0)
                        schemaCallData.flags |= SCH_SHARP_ATTEN;
                     schemaCallData.sourceID = ambObjID(i);  // objID
                     schemaCallData.pData    = (void *)i;    // handle
                     schemaCallData.volume   = state->rad;
                     _ambsnd_handle(i)=SchemaPlay(&state->schema_name,&schemaCallData,NULL);
                     if (_ambsnd_handle(i)==SCH_HANDLE_NULL)
                        Warning(("Ambient couldn't play %s\n",state->schema_name.text));
                     _ambsnd_lastplay(i)     = sim_time;
                     amb_info(("Fired ambient object (idx %d) %s (%x)\n",
                               i,state->schema_name.text,_ambsnd_handle(i)));
                  }  // not sure how to wire up the modify callback, sadly
      }

   if (restore_obj != OBJ_NULL)
      near_env_idx = _AmbientRestore();

   if (near_env_idx != AMB_ENV_INACTIVE)
   {  // Bail if we're in the currently active schema
      if (near_env_idx == cur_env_idx)
         return;

      last_env_idx = cur_env_idx;
      cur_env_idx = near_env_idx;
      amb_info(("Switching into env %d from %d\n",cur_env_idx,last_env_idx));

      if (last_env_idx!=AMB_ENV_INACTIVE && ambState(last_env_idx)==NULL)
      {
#ifdef PLAYTEST
         mprintf("Needing to use INACTIVE since state of %d is NULL\n",last_env_idx);
#endif
         amb_info(("Needing to use INACTIVE since state of %d is NULL\n",last_env_idx));
         last_env_idx=AMB_ENV_INACTIVE;
      }

      // Don't start main if it has the same name
      if ((last_env_idx==AMB_ENV_INACTIVE)||
          (stricmp(ambState(last_env_idx)->schema_name.text,ambState(cur_env_idx)->schema_name.text)))
      {  // it is an environmental of a different name
         if ((last_env_idx!=AMB_ENV_INACTIVE)&&(_ambsnd_handle(last_env_idx)!=NO_CUR_HANDLE))
         {  // we are currently playing, want to stop it
            amb_info(("Leaving environment schema (idx %d) %s (%x)\n",
                      last_env_idx,ambState(last_env_idx)->schema_name.text,_ambsnd_handle(last_env_idx)));
            SchemaPlayHalt(_ambsnd_handle(last_env_idx));  // @TODO: Fade?
            _amb_xtra_clear(_ambsnd_handle(last_env_idx));
         }
#ifdef PLAYTEST         
         else
         {
            if (last_env_idx==AMB_ENV_INACTIVE)
            {
               if ((hAuxSch1!=NO_CUR_HANDLE)||(hAuxSch2!=NO_CUR_HANDLE))
                  Warning(("Had aux 1 or 2, no primary ambient bed\n"));
               amb_info(("Leaving environment NULL (%x %x)\n",hAuxSch1,hAuxSch2));
            }
            else
               amb_info(("Leaving environment %s main inactive (aux %x %x)\n",
                         ambState(last_env_idx)->schema_name.text,hAuxSch1,hAuxSch2));
         }
#endif         
         if (ambState(near_env_idx)->schema_name.text[0]!='\0')
         {
            schemaCallData.flags    = SCH_SET_CALLBACK;
            schemaCallData.pData    = (void *)AMB_ENV_PDATA; // cur_env_idx;
            _ambsnd_handle(cur_env_idx)=SchemaPlay(&ambState(cur_env_idx)->schema_name, &schemaCallData, NULL);
            amb_info(("Entering environment schema (idx %d) %s (%x)\n",
                      cur_env_idx,ambState(cur_env_idx)->schema_name.text,_ambsnd_handle(cur_env_idx)));
         }
         else
            amb_info(("Entering environment schema NULL\n"));
      }
      else if (last_env_idx!=AMB_ENV_INACTIVE)
      {  // same base schema, maybe new aux stuff
         _ambsnd_handle(cur_env_idx)=_ambsnd_handle(last_env_idx);
         _ambsnd_handle(last_env_idx)=NO_CUR_HANDLE;
      }

      // Stop auxiliaries
      if (hAuxSch1!=NO_CUR_HANDLE)
      { 
         amb_info2(("kill aux 1 %x\n",hAuxSch1));
         SchemaPlayHalt(hAuxSch1); 
         _amb_xtra_clear(hAuxSch1);
      }
      if (hAuxSch2!=NO_CUR_HANDLE)
      { 
         amb_info2(("kill aux 2 %x\n",hAuxSch2));
         SchemaPlayHalt(hAuxSch2); 
         _amb_xtra_clear(hAuxSch2);         
      }

      // Start auxiliaries
      if (ambState(near_env_idx)->aux_schema_1.text[0]!='\0')
      {
         schemaCallData.flags    = SCH_SET_CALLBACK;
         schemaCallData.pData = (void *)AMB_ENV_AUX_PDATA;    
         hAuxSch1=SchemaPlay(&ambState(cur_env_idx)->aux_schema_1, &schemaCallData,NULL);
         amb_info2(("start aux 1 %x\n",hAuxSch1));
      }
      if (ambState(near_env_idx)->aux_schema_2.text[0]!='\0')
      {
         schemaCallData.flags    = SCH_SET_CALLBACK;
         schemaCallData.pData = (void *)AMB_ENV_AUX_PDATA;             
         hAuxSch2=SchemaPlay(&ambState(cur_env_idx)->aux_schema_2, &schemaCallData, NULL);
         amb_info2(("start aux 2 %x\n",hAuxSch2));
      }
   }
   amb_hbeat();
}

//////////////////////////
// save/load support

static TagVersion AmbientVersion = {1, 2};
static TagFileTag AmbientTag = {"AMBIENT"};

BOOL AmbientSave(ITagFile *file)
{
   HRESULT result = ITagFile_OpenBlock(file, &AmbientTag, &AmbientVersion);
   BOOL rv = FALSE;

   if (result == S_OK)
   {
      ObjID obj = ambObjID(cur_env_idx);
      ITagFile_Write(file, (char *) &obj, sizeof(obj));
      ITagFile_CloseBlock(file);
   }
   if (cur_env_idx != -1)
   {  // @HACK for beta: make sound start again
      restore_obj = ambObjID(cur_env_idx);
//      cur_env_idx = -1;
   }

   return rv;
}


BOOL AmbientLoad(ITagFile *file)
{
   TagVersion found_version = AmbientVersion;
   HRESULT result = ITagFile_OpenBlock(file, &AmbientTag, &found_version);
   BOOL rv = FALSE;

   if (result == S_OK
    && found_version.major == AmbientVersion.major
    && found_version.minor == AmbientVersion.minor)
   {
      ITagFile_Read(file, (char *) &restore_obj, sizeof(restore_obj));
      ITagFile_CloseBlock(file);
   }
   return rv;
}

// attempt to restore on load
static int _AmbientRestore(void)
{
   ObjID obj = ObjRemapOnLoad(restore_obj);

   restore_obj = OBJ_NULL;

   if (obj != OBJ_NULL) {
      int max = ambMax();
      int i;
      for (i = 0; i < max; ++i)
         if (ambObjID(i) == obj)
         {
            amb_info(("Restore idx %d from obj %d\n",i,obj));
            return i;
         }
   }
   Warning(("Could not restore old ambient state from obj %d.\n", obj));
   return -1;
}

#ifdef PLAYTEST
#include <r3d.h>
#include <g2.h>
#include <fix.h>
void AmbientDrawRadii(BOOL all)
{
   int ocol = gr_get_fcolor();
   r3s_point pt;
   int i;

   gr_set_fcolor(241);
   for (i=0; i<ambMax(); i++)
      if (ambObjID(i)!=OBJ_NULL)      
      {
         if (!all && (_ambsnd_handle(i)==NO_CUR_HANDLE))
            continue;
         r3_start_block();
         r3_transform_block(1, &pt, &ObjPosGet(ambObjID(i))->loc.vec);
         if (pt.ccodes == 0)
         {
            AmbientSound *state=ambState(i);
            int s=(int)r3_get_hsize(pt.p.z,(float)state->rad);
            int rad=fix_div(s,grd_bm.w);
            g2_circle(pt.grp.sx, pt.grp.sy, rad);
         }
         r3_end_block();
      }
   gr_set_fcolor(ocol);
}
#endif
