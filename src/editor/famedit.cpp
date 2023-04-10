/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/famedit.cpp,v 1.9 1998/10/04 02:17:24 dc Exp $
// editor only family management code

#include <stdlib.h>
#include <string.h>

#include <mprintf.h>

#include <lresname.h>

#include <editbr_.h>
#include <editbr.h>
#include <brinfo.h>
#include <brlist.h>

#include <family_.h>
#include <famedit.h>

#include <brushgfh.h>
#include <command.h>
#include <texmem.h>
#include <palmgr.h>
#include <status.h>

// for reporting on location of texture 0
#include <portal.h>
#include <portal_.h>
#include <wrdbrend.h>
#include <wrdb.h>
#include <anim_txt.h>
#include <render.h>

#include <report.h>

#include <dbmem.h>

// i am happy this isnt in stdlib, and thus not ruining the purity of cplusplus
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b) )
#endif

// do we need to do this swap?
static BOOL check_swap(int *map_table, short *val, int cnt)
{
   int nval=0;
   if ((*val)==-1) return FALSE;
   if (((*val)>=WATERIN_IDX)&&((*val)<=255)) // hateful initial check to save sky_tex
      return FALSE;
   else if (((*val)>=0)&&((*val)<cnt))
      nval=map_table[*val];

   if (nval==-1) nval=0;   // since we dont want -1 if there is no texture
   if (nval!=(*val))
   {
      *val=(short)nval;
      return TRUE;
   }
   return FALSE;
}

// remap the current brush texture handles
BOOL _familyRemapBrushes(int *tex_swap, int cnt)
{
   editBrush *us;
   bool chg=FALSE;
   int i, hIter;

   us=blistIterStart(&hIter);
   tex_swap[BACKHACK_IDX]=BACKHACK_IDX;
   while (us!=NULL)
   {
      if (brushGetType(us)==brType_TERRAIN)
      {
         chg|=check_swap(tex_swap,&us->tx_id,cnt);
         for (i=0; i<us->num_faces; i++)
            chg|=check_swap(tex_swap,&us->txs[i].tx_id,cnt);
      }
      us=blistIterNext(hIter);
   }
   return chg;
}

// compress out unused family elements
static void family_cmd_compress(char *str)
{
   fam_record *toast_rec=NULL;
   int i, targ_loadid, hIter;
   int texs_used[TEXMEM_MAX];
   editBrush *us;
   
   if (stricmp("all",str)==0)
      targ_loadid=-1;
   else if (stricmp("anon",str)==0)
      targ_loadid=TEXMEM_LOADID_ANON;
   else
   {
      toast_rec=_fam_lookup_record(str);
      if (toast_rec)
         targ_loadid=toast_rec->load_id;
      else    // just remove this family
      {
         Status("No such family to compress");
         return;
      }
   }
   // scan through all textures, build count list for each texture
   memset(texs_used,0,sizeof(int)*TEXMEM_MAX);
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if (brushGetType(us)==brType_TERRAIN)
      {
         if ((us->tx_id==-1)||(us->tx_id>=TEXMEM_MAX))
            Warning(("Brush id %d texture has id %d\n",us->br_id,us->tx_id));
         else
            texs_used[us->tx_id]++;
         for (i=0; i<us->num_faces; i++)
         {
            int val=us->txs[i].tx_id;
            if (val!=-1)
               if (val>=TEXMEM_MAX) // this shouldnt be possible
                  Warning(("Brush id %d texture has id %d on face %d\n",us->br_id,us->tx_id,i)); 
               else
                  texs_used[val]++;
         }
      }
      us=blistIterNext(hIter);      
   }
   // once done, for each unused, see if they match our loadid
   for (i=1; i<TEXMEM_MAX; i++)  // @TODO: get rid of ugly start at 1 due to secret knowledge hack
      if ((texs_used[i]==0)&&(i<texmemGetMax()))
         if ((targ_loadid==-1)||(texmem_loadid[i]==targ_loadid))
            texmemFreeSingle(i);
   if (toast_rec!=NULL)
      toast_rec->load_id=TEXMEM_LOADID_NULL;
   else if (targ_loadid==-1)
      _fam_clear_records();
   _fam_synch(TRUE);
}

// draw the histogram for this cnt element array, offset is "real" index of 0
static void _draw_histo(int *histo, int cnt, int offset, BOOL xtras, char **report_str, BOOL totals)
{
   int i, j, mem_used[2]={0,0}, usages_per=10;
   char buf[80*256], *p; // hey, editor only, woo-woo
   p=buf;
   for (i=0; i<cnt; i++)
   {
      IRes *me=texmemGetHandle(i+offset);
      int frames=0;
      int size=ectsAnimTxtGetRawSize(me,&frames);      
      if (histo[i])
      {
         if (xtras&&(i+offset>0))
         {
            grs_bitmap *bm=texmemGetTexture(i+offset);
            if ((size>128*128)&&histo[i]<16&&((i+offset)<250))
               if (frames>1)
                  rsprintf(&p,"WARNING: txt %s only %d uses, is %d frames %d by %d big (%dKb)\n",
                           me?me->GetName():"Unknown",histo[i],frames,bm?bm->w:0,bm?bm->h:0,size/1024);
               else
                  rsprintf(&p,"WARNING: txt %s only %d uses, is %d by %d big (%dKb)\n",
                           me?me->GetName():"Unknown",histo[i],bm?bm->w:0,bm?bm->h:0,size/1024);
            if (frames>1)
               rsprintf(&p," f%1.1d",frames);
            else
               rsprintf(&p,"   ");
            rsprintf(&p," %8.8s %3.3dx%3.3d  ",me?me->GetName():"Unknown",bm?bm->w:0,bm?bm->h:0);
            mem_used[0]+=size*4/3;
         }
         rsprintf(&p,"%3d|%5d>",i+offset,histo[i]);
         for (j=0; j<min((9+histo[i])/usages_per,xtras?46:66); j++)
            rsprintf(&p,"#");
         rsprintf(&p,"\n");
      }
      else if (i+offset>0)  // if not, we cant tell, so, oh well
         if (me)
         {
            grs_bitmap *bm=texmemGetTexture(i+offset);            
            mem_used[1]+=size*4/3;
            if (frames>1)
               rsprintf(&p," f%1.1d",frames);
            else
               rsprintf(&p,"   ");
            rsprintf(&p," %8.8s %3.3dx%3.3d  Is Unused\n",me?me->GetName():"Unknown",bm?bm->w:0,bm?bm->h:0);
         }
   }
   rsprintf(&p,"note: a single # represents a group of %d usages\n",usages_per);
   if (totals)
      rsprintf(&p,"Texture RAM in use in WR ~%dKb, ~%dKb loaded but not in world\n",mem_used[0]/1024,mem_used[1]/1024);
   if (report_str)
      rsprintf(report_str,buf);
   else
      mprint(buf);
}

// should look at something, someday
#define DoWeUseSky() GetSkyMode()==kSkyModeTextures?1:0

// look for this ID
static void family_find_tmap_id(int tmap_id)
{
   int i, used_this_face, hIter;
   int histo[257];
   char tmp[128];
   editBrush *us;
   int cnt[3];

   cnt[0]=cnt[1]=cnt[2]=0;
   memset(histo,0,sizeof(int)*257);
   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if (brushGetType(us)==brType_TERRAIN)
      {
         used_this_face=0;
         for (i=0; i<us->num_faces; i++)
         {
            if (us->txs[i].tx_id==-1)
               used_this_face++;
            else if (us->txs[i].tx_id==tmap_id)
               cnt[1]++;
            histo[us->txs[i].tx_id+1]++;
         }
         if (used_this_face>0)
         {
            if (us->tx_id==tmap_id)
            {
               cnt[0]++;
               cnt[2]+=used_this_face;
            }
            histo[us->tx_id+1]+=used_this_face;
         }
      }
      us=blistIterNext(hIter);
   }
   histo[250]=histo[251]=histo[252]=histo[253]=histo[254]=histo[255]=DoWeUseSky();
   sprintf(tmp,"Faces: %d Br: %d",cnt[1]+cnt[2],cnt[0]);
   Status(tmp);
   mprintf("TmapID %d: ",tmap_id);
   mprintf("%d brushes, %d face overrides, %d faces use it as brush base\n",cnt[0],cnt[1],cnt[2]);
   _draw_histo(histo,257,-1,TRUE,NULL,TRUE);
}

////////////////
// usage in WR, not brush list
static void texture_wr_usage_count(int tmap_id, char **rep_str)
{
   int histo[256];
   PortalCell *p;
   int ci, i;

   memset(histo,0,sizeof(int)*256);
   for (ci=0; ci<wr_num_cells; ci++)
      if ((p=wr_cell[ci])!=NULL)
         for (i=0; i<p->num_render_polys; i++)
            histo[p->render_list[i].texture_id]++;
   histo[250]=histo[251]=histo[252]=histo[253]=histo[254]=histo[255]=DoWeUseSky();
   if (tmap_id)
   {
      char buf[128];
      if (histo[tmap_id])
         sprintf(buf,"Tmap %d used %d times",tmap_id,histo[tmap_id]);
      else
         sprintf(buf,"Tmap %d not used");
      Status(buf);
   }
   else
      _draw_histo(histo,256,0,TRUE,rep_str,TRUE);
}

static void wr_usage_count_cmd(int tmap_id)
{
   texture_wr_usage_count(tmap_id,NULL);
}

//////////////
// find texture zero usages in the WR
static void texture_wr_find_zero(char **rep_str)
{
   char *b, *m=NULL;  // ?????
   PortalCell *p;
   int ci, i, zeroes_found=0;

   if (rep_str)
      b=*rep_str;
   else
   {
      b=m=(char *)Malloc(16384);
      m[0]='\0';
   }


   for (ci=0; ci<wr_num_cells; ci++)
      if ((p=wr_cell[ci])!=NULL)
      {
         int voff=0;
         for (i=0; i<p->num_render_polys; i++)
         {
            PortalPolygonCore *poly=&p->poly_list[i];
            if (p->render_list[i].texture_id==0)
            {
               int k, n=poly->num_vertices;
               zeroes_found++;
               rsprintf(&b,"Cell %d poly %d uses texture 0\n",ci,i);
               for (k=0; k<n; k++)
                  rsprintf(&b," %.2f %.2f %.2f%s",
                          p->vpool[p->vertex_list[voff+k]].el[0],
                          p->vpool[p->vertex_list[voff+k]].el[1],
                          p->vpool[p->vertex_list[voff+k]].el[2],
                           (k==(n-1))?"\n":" :");
            }
            voff+=poly->num_vertices;
         }
      }
   rsprintf(&b,"Found %d zero textures in level\n",zeroes_found);
   if (m)
   {
      mprint(m);
      Free(m);
   }
   else
      *rep_str=b;
}

static void wr_find_zero_cmd(void)
{
   texture_wr_find_zero(NULL);
}

// bridge over to the palmgr counter functions
static void pal_bridge_func(BOOL spew)
{
   int cnt=palmgr_get_current_count(spew);
   char buf[20];

   sprintf(buf,"Using %d pals",cnt);
   Status(buf);
}

///////////////////////
// report functions for textures/WR

#ifdef REPORT
static void _textures_report(int WarnLevel, void *data, char *buffer)
{
   char *p=buffer;

   rsprintf(&p,"\nWorld Rep Textures\n");

   // first lets scan for warnings
   texture_wr_find_zero(&p);

   // now lets check for grim scenes
   if (WarnLevel>=kReportLevel_Info)
      texture_wr_usage_count(0,&p);
}
#endif
 
// editor only family commands 
static Command family_commands[]=
{
   { "compress_family", FUNC_STRING, family_cmd_compress, "remove unused textures from family or <all>" },
   { "texture_usage_count", FUNC_INT, family_find_tmap_id, "get usage count for this texture" },
   { "texture_wr_usage_count", FUNC_INT, wr_usage_count_cmd, "get wr usage for tmap_id, or histo if 0" },
   { "texture_wr_find_zero", FUNC_VOID, wr_find_zero_cmd, "find texture zero in the world" },   
   { "palmgr_count", FUNC_BOOL, pal_bridge_func },
};

// start up the famedit stuff
void family_edit_init(void)
{
   COMMANDS(family_commands,HK_EDITOR);
   ReportRegisterGenCallback(_textures_report,kReportWorldDB,"Textures",NULL);   
}

void family_edit_term(void)
{
   ReportUnRegisterGenCallback(_textures_report,kReportWorldDB,NULL);   
}
