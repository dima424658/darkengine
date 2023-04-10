/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/anim_txt.c,v 1.25 1999/08/05 17:28:08 Justin Exp $
// hack for animating textures for ects prior to real syste

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <lg.h>
#include <mprintf.h>

#include <config.h>
#include <cfgdbg.h>

#include <storeapi.h>
#include <resapi.h>
#include <resdata.h>

#include <texmem.h>
#include <portal.h>
#include <simtime.h>

#include <tanimprp.h>
#include <textarch.h>

#include <anim_txt.h>

#include <_animtxt.h>

#include <dbmem.h>

#ifndef SHIP
#define optConfigSpew(var,x) do { if (config_is_defined(var)) mprintf x; } while (0)
#else
#define optConfigSpew(var,x)
#endif

// for now, parse config to get a list of which textures animate
// then go in and fix up the resNameHandle or bitmap pointers using it
// set a flag in the user data
                
BOOL ectsAnimLoading=FALSE;

// @TBD: This method is now obsolete, and being kept only because lresname
// depends on it. Kill it when we kill lresname
BOOL ectsAnimTxtValidName(char *fname)
{
   Warning(("ectsAnimTxtValidName called with %s.\n", fname));
   return FALSE;
}

BOOL ectsAnimTxtIgnore(const char *fname)
{
   char *t, test_c;
   if (ectsAnimLoading) return FALSE;
   t=strrchr(fname,'_');
   if (t==fname) return FALSE;
   if (t==NULL) return FALSE;
   if (t>=fname+strlen(fname)-1) return FALSE;
   if (((t-fname)==strlen(fname)-2)||   // one character after t
       (*(t+2)=='.'))
   {
      test_c=*(t+1);
      if ((test_c>='0')&&(test_c<='9'))
         return TRUE;
   }
   return FALSE;
}

int ectsAnimTxtTime=0;

#define FLAGS_PROP     (0xff)
#define FLAG_WRAP      (1<<0)
#define FLAG_RANDINC   (1<<1)
#define FLAG_REVERSE   (1<<2)

#define FLAGS_RUNTIME  (0xff<<8)
#define FLAG_PORTAL    (1<<8)
#define FLAG_CONFIGURE_ME (1<<9)

#define DEF_RATE 250
#define DEF_FLAG FLAG_WRAP

#define MAX_SECRET_TXT (256)

static IRes *yet_another_secret_list[MAX_SECRET_TXT];

void add_to_secret_list(IRes *hnd)
{
   int i;
   for (i=0; i<MAX_SECRET_TXT; i++)
      if (yet_another_secret_list[i]==NULL)
      {
         yet_another_secret_list[i]=hnd;
         return;
      }
}

void rem_from_secret_list(IRes *hnd)
{
   int i;
   for (i=0; i<MAX_SECRET_TXT; i++)
      if (yet_another_secret_list[i]==hnd)
      {
         yet_another_secret_list[i]=NULL;
         return;
      }
}

// returns NULL if no extension, else beginning of extension within fname, w/o "."
// TODO: really, should make sure the . is past the last slash
//   so you can do moose/camel.dir/harold/elephant right
//
// Formerly in rn_cnvrt; moved so we could get rid of that.
//
// This should probably become a common utility routine somewhere...
char *extension_p(char *fname)
{
   char *t;
   t=strrchr(fname,'.');
   if (t==fname+strlen(fname)-1)  // last character means, basically, no extension
      return NULL;
   else if (t==fname) // first character
      return NULL;
   else if (t==NULL)  // no .
      return NULL;
   else
   {  // we should now make sure there are no slashes past here in the string
      if (strchr(t+1,'\\')||strchr(t+1,'/'))
         return NULL;
      return t+1;     // ah-ha... we are real, we found a real . cool
   }
}

// strip the extension as well
IRes *ectsAnimSearchForFrame(char *name, int idx, char *path, BOOL portal_tmap)
{
   char buf[80], *s;
   strcpy(buf,name);
   s=extension_p(buf);
   if ((s!=NULL)&&(s!=buf)) *(s-1)='\0';
   if (strlen(buf)>=7) return NULL;   // too long a name for a frame add on
   sprintf(buf+strlen(buf),"_%d",idx);
   return texmemLoadSingleRes(path, buf, portal_tmap);
}

static IAnimTexProperty* gpAnimProp = NULL;

// do initial setup and configure for an anim texture from properties
static void ectsAnimConfigure(ectsAnimInfo *us)
{
   static sAnimTexProp defprop = { DEF_RATE, DEF_FLAG }; 
   sAnimTexProp* prop = &defprop;
   PROPERTY_GET(gpAnimProp,GetResTextureObj(us->frame_hnd[0]),&prop);
   us->rate=prop->rate;
   us->flags &= FLAGS_RUNTIME; 
   us->flags|=prop->flags;
   us->flags &= ~FLAG_CONFIGURE_ME; 
}

// takes a res with animData and new animprop values, refreshes said anim
BOOL ectsAnimRefresh(IRes *pRes, sAnimTexProp *newvals)
{
   ectsAnimInfo *animData;
   if (!pRes) return FALSE;
   animData = GetResAnimData(pRes);
   if (!animData) return FALSE;
   animData->rate=newvals->rate;
   animData->flags=(animData->flags&FLAGS_RUNTIME)|(newvals->flags&FLAGS_PROP);
   animData->flags &= ~FLAG_CONFIGURE_ME; 
   return TRUE;
}

// sees if the texture named here has anims, if so load an anim block, then know about it
//   path and portal_tmap for now, i guess
void  ectsAnimTxtCheckLoad(IRes *me, BOOL portal_tmap, char *path, int txt_id)
{
   char name[MAX_STORENAME+1];
   ectsAnimInfo *animData;

   if (!me)
      return;

   animData = GetResAnimData(me);
   strcpy(name, IRes_GetName(me));

   if (animData)
   {
      // We are already a known animation
      if (animData->lock_cnt && animData->txt_id && txt_id && (animData->txt_id!=txt_id))
         Warning(("AnimTxt: already animating with different txt_id! (old %d vs new %d)\n",animData->txt_id,txt_id));
      else
      {
         optConfigSpew("anim_txt_load",("AnimTxt: %s bumping lock to %d\n",name, animData->lock_cnt+1));
      }
      animData->lock_cnt++;   // we already exist, create new local instance, i guess??
   }
   else
   {
      int frame_idx=1, old_cback_idx=texmem_callback_idx, base_align;
      IRes *nxt_frame;

      // GROSS only use _ at end if !txt_id hacks
      if (txt_id==0 && (name[strlen(name)-1]!='_'))
         return;
      if (name[strlen(name)-1]=='_')
         name[strlen(name)-1]='\0';
      ectsAnimLoading=TRUE;
      texmem_callback_idx=TEXMEM_NO_HND;  // make sure we are at NO_HND
      // Note that it's okay to not find nxt_frame; we don't complain...
      nxt_frame=ectsAnimSearchForFrame(name,frame_idx,path,portal_tmap);
      if (nxt_frame)
      {
         // 16 bit horror
         if (!portal_tmap)
         {
            grs_bitmap *bm = (grs_bitmap *) IRes_Lock(me);
            base_align=bm->align;
            bm = (grs_bitmap *) IRes_Lock(nxt_frame);
            bm->align=base_align;
            IRes_Unlock(me);
            IRes_Unlock(nxt_frame);
         }
         optConfigSpew("anim_txt_load",("Loading anim texture %s ",name));
         animData=(ectsAnimInfo *)Malloc(sizeof(ectsAnimInfo));
         SetResAnimData(me, animData);
         animData->flags = 0; 
         animData->frame_hnd[0]=me;
         animData->frame_hnd[frame_idx++]=nxt_frame;
         while ((nxt_frame=ectsAnimSearchForFrame(name,frame_idx,path,portal_tmap))!=NULL)
         {
            animData->frame_hnd[frame_idx++]=nxt_frame;
            if (!portal_tmap)
            {
               grs_bitmap *bm=(grs_bitmap *) IRes_Lock(nxt_frame);
               bm->align=base_align;
               IRes_Unlock(nxt_frame);
            }
            if (frame_idx>=MAX_FRAMES)
            {
#ifndef SHIP
               mprintf("HEY! Used all available anim_txt frames, maybe more for %s (%d)\n",name,frame_idx);
#endif
               break;
            }
         }
         optConfigSpew("anim_txt_load",("- found %d frames\n",frame_idx));
         animData->cnt=frame_idx;
         animData->lock_cnt=1;
         animData->cur=0;
         while (frame_idx<MAX_FRAMES)
            animData->frame_hnd[frame_idx++]=NULL;
         // if so, add an animsys record
         animData->txt_id=txt_id;
         animData->last_time=GetSimTime();
         ectsAnimConfigure(animData);
         animData->flags |= FLAG_CONFIGURE_ME; 

         if (portal_tmap)
            animData->flags|=FLAG_PORTAL;
         if (txt_id)
            add_to_secret_list(me);
      }
      ectsAnimLoading=FALSE;
      texmem_callback_idx=old_cback_idx;
   }
}

// it will act on the edge hit if it doesnt toast, ie. set the new frame id
static void ectsAnimHitEdge(ectsAnimInfo *anim, BOOL top_edge)
{
   if (anim->flags&FLAG_WRAP)
      anim->cur=top_edge?0:anim->cnt-1;
   else
   {
      anim->cur=top_edge?anim->cnt-2:1;
      if (anim->flags&FLAG_REVERSE)  // should be == top_edge, but not quite sure?
         anim->flags&=~FLAG_REVERSE;
      else
         anim->flags|= FLAG_REVERSE;
   }
}

static void ectsAnimUpdateTxt(ectsAnimInfo *anim)
{  // @NOTE: Assumes that the frame's resource is already locked in memory...
   texmemGetTexture(anim->txt_id)=IRes_DataPeek(anim->frame_hnd[anim->cur]);
   clear_surfaces_for_texture(anim->txt_id);
}

// actually run a local
void ectsAnimRunSingle(ectsAnimInfo *anim)
{
   int diff=ectsAnimTxtTime-anim->last_time;

   if (anim->flags & FLAG_CONFIGURE_ME)
      ectsAnimConfigure(anim); 

   if (diff < 0) 
   {
      optConfigSpew("anim_txt_frame",("Anim texture %d was run in the future\n",anim->txt_id)); 
      anim->last_time = ectsAnimTxtTime;
      diff = 0;
   }
   else if (diff>2000)
   {
      optConfigSpew("anim_txt_frame",("Anim texture %d was run with diff %d\n",anim->txt_id,diff));
      anim->last_time = ectsAnimTxtTime;
      diff = 256;
   }

   // Load the rate data from prop 
   // for now, rate cannot be changed once loaded
//   ectsAnimConfigure(anim,anim->txt_id);

   optConfigSpew("anim_txt_frame",("Anim texture %d update at frame %d (rate %d sim %d)\n",anim->txt_id,anim->cur,anim->rate,GetSimTime()));
   while (diff>anim->rate) // turn over to next frame
   {
      diff-=anim->rate;
      if (anim->flags&FLAG_REVERSE)
         if (anim->cur>0)
            anim->cur--;
         else
            ectsAnimHitEdge(anim,FALSE);
      else
         if (anim->cur<anim->cnt-1)
            anim->cur++;
         else
            ectsAnimHitEdge(anim,TRUE);
      if (anim->txt_id)   // can we move this out
         ectsAnimUpdateTxt(anim);
      anim->last_time+=anim->rate;
      optConfigSpew("anim_txt_frame",("Anim texture %d now frame %d (sim %d)\n",anim->txt_id,anim->cur,GetSimTime()));
   }
}

void *ectsAnimTxtGetFrame(IRes *me)
{
   ectsAnimInfo *animData;
   if (!me) return NULL;

   animData = GetResAnimData(me);
   if (!animData)
   {  // @NOTE: we are assuming that the textures are permanently locked...
      return IRes_DataPeek(me);
   } else {
      // this really should be done by animsys, not locally
      ectsAnimRunSingle(animData);
      return IRes_DataPeek(animData->frame_hnd[animData->cur]);
   }
}

void ectsAnimTxtUpdateAll(void)
{
   int i;
   for (i=0; i<MAX_SECRET_TXT; i++)
      if (yet_another_secret_list[i])
         ectsAnimTxtGetFrame(yet_another_secret_list[i]);
}

// @TBD: this doesn't have any sort of good correspondent in the new
// resource model. Do we care?
void  ectsAnimTxtRemapAll(int *mapping, int cnt)
{
   int i;
   for (i=0; i<MAX_SECRET_TXT; i++)
      if (yet_another_secret_list[i])
      {
         IRes *me=yet_another_secret_list[i];
         ectsAnimInfo *animData=GetResAnimData(me);
         if (animData)
         {
            int old=animData->txt_id;
            if (old<cnt)
               animData->txt_id=mapping[old];
         }
      }
}

#ifndef SHIP
int   ectsAnimTxtGetRawSize(IRes *me, int *fr)
{
   int size=0, frames=0;
   ectsAnimInfo *animData;
   
   if (!me)
   {
      *fr=0;
      return 0;
   }
   animData = GetResAnimData(me);
   if (animData)
   {
      int i;
      for (i=1; i<MAX_FRAMES; i++)
         if (animData->frame_hnd[i])
         {
            grs_bitmap *bm=IRes_Lock(animData->frame_hnd[i]);
            if (bm)
            {
               size+=bm->w*bm->h;
               frames++;
            }
            IRes_Unlock(animData->frame_hnd[i]);
         }
         else
            break;
   }
   else
   {
      grs_bitmap *bm=IRes_Lock(me);
      if (bm)
      {
         size+=bm->w*bm->h;
         frames++;
      }
      IRes_Unlock(me);
   }
   if (fr) *fr=frames;
   return size;
}
#endif

void  ectsAnimTxtCheckFree(IRes *me)
{
   int i;
   ectsAnimInfo *animData;

   if (!me) return;
   animData = GetResAnimData(me);
   if (!animData) return;

   if (animData->txt_id)
   {
      animData->cur=0;             // for replacement
      ectsAnimUpdateTxt(animData); // get our tmap right, so the subsequent real free is correct
      rem_from_secret_list(me);
   }
   if (--animData->lock_cnt>0) return;
   for (i=1; i<MAX_FRAMES; i++)
      if (animData->frame_hnd[i])
         texmemFreeSingleRes(animData->frame_hnd[i],(animData->flags&FLAG_PORTAL));
      else if (i<animData->cnt)
         Warning(("Hey - nohnd but within my cnt in ectsAnimTxtCheckFree\n"));
   // free animsys
   Free(animData);
   SetResAnimData(me, NULL);
}

void ectsAnimTxtInit()
{
   gpAnimProp = CreateAnimTexProperty(); 
}

void ectsAnimTxtTerm()
{
   SafeRelease(gpAnimProp);
}
