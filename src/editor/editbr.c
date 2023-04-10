/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/editbr.c,v 1.53 2000/02/19 12:27:58 toml Exp $

// Brush Manager (editbr.c)
//    this system manages the creation, parameterization, and modification
// of a single brush structure.  this includes any edits to the media, 
// surface textures/parameters, sizing and stretching of the brush
// itself.  this does not include brush lists or construction.  

#include <stdlib.h>
#include <io.h>
#include <rand.h>

#include <lg.h>
#include <lgassert.h>
#include <hheap.h>
#include <mprintf.h>
#include <command.h>

#include <uiedit.h>     // get color fixup code

#include <editbr.h>
#include <brlist.h>
#include <editbr_.h>
#include <brinfo.h>
#include <primal.h>

#include <ged_rmap.h>
#include <csgbrush.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define DEF_SIZE 2.0
#define DEF_MEDIA 1

// no id, rot 0, scale 4 (magic), u,v at 0
TexInfo defTexInf={-1,0,16,0,0};

static HheapHead brushHeap;
static int brushNextID=1;


#define DBG_BRUSHALLOC
#ifdef DBG_BRUSHALLOC
#define AllocBrush()    Malloc(sizeof(editBrush))
#define FreeBrush(br)   Free(br)
#else
#define AllocBrush()    HheapAlloc(&brushHeap)
#define FreeBrush(br)   HheapFree(&brushHeap,br)
#endif 

// sets the current heap for the brush system to use
void brushSysInit(void)
{
   primalBr_Init();
   HheapInit(&brushHeap,sizeof(editBrush),128,TRUE);
   brushNextID=0;
}

// sets the current heap for the brush system to use
void brushSysFree(void)
{
#ifdef DBG_ON
   int cnt=HheapGetNumItems(&brushHeap);
   if (cnt!=0)
      Warning(("Freeing Brush Sys with %d alloced brushes\n",cnt));
#endif   
   HheapFreeAll(&brushHeap);
   primalBr_Term();   
}

// instantiates a copy of brush_id into the edit buffer
// for now, we have no idea what brush_id is, so i always pretend its a cube?
void brushZero(editBrush *curBrush, int primal_brush)
{
   int i;

   curBrush->pos.el[0]=0.0;
   curBrush->pos.el[1]=0.0;
   curBrush->pos.el[2]=0.0;
   curBrush->ang.el[0]=0.0;
   curBrush->ang.el[1]=0.0;
   curBrush->ang.el[2]=0.0;
   curBrush->sz.x=DEF_SIZE;
   curBrush->sz.y=DEF_SIZE;
   curBrush->sz.z=DEF_SIZE;
   curBrush->tx_id=0;
   curBrush->media=DEF_MEDIA;

   curBrush->flags = kEBF_None;

   if (brushNextID>=MAX_CSG_BRUSHES)
   {
      Warning(("Compressing BrushID Space - remember this happened if you crash\n"));
      brushNextID=ged_remap_brushes();
   }

   curBrush->br_id=brushNextID++;      // this is so every brush has a unique
   curBrush->grid.grid_enabled=FALSE;  // id.  it only has to be valid from one
                             // portalization to the next, so it resets on load
   curBrush->primal_id = primal_brush;

   if (primalBr_FaceCnt(primal_brush)>EDITBR_MAX_FACES)
   {
      Warning(("Will break on %d (%d)\n",primal_brush,primalBr_FaceCnt(primal_brush)));
      curBrush->num_faces = EDITBR_MAX_FACES;
   }
   else
      curBrush->num_faces = primalBr_FaceCnt(primal_brush);
   
   curBrush->cur_face  = -1;
   for (i=0; i<curBrush->num_faces; i++)
      curBrush->txs[i]=defTexInf;  // uses global info, sets scale and rot

   curBrush->edge=0;
   curBrush->point=0;
   curBrush->use_flg=0;
   curBrush->group_id=0;

   curBrush->pad0=0;
}

// if primal_brush==DEF_PRIMAL use the global
int editbr_default_primal=PRIMAL_CUBE_IDX;
editBrush *brushInstantiate(int primal_brush)
{
   editBrush *curBrush=AllocBrush();
   if (primal_brush==DEF_PRIMAL)
      primal_brush=editbr_default_primal;
   brushZero(curBrush,primal_brush);
   return curBrush;
}

// this copies a brush into a newly allocated brush, ie. one with a new br_id
editBrush *brushCopy(editBrush *srcBrush)
{
   editBrush *dstBrush;

   dstBrush=AllocBrush();
   *dstBrush=*srcBrush;
   dstBrush->br_id=brushNextID++;
   return dstBrush;
}

// delete a brush - this doesnt get it out of list first, so you had better
bool brushDelete(editBrush *curBrush)
{
#ifdef DBG_ON
   if (blistCheck(curBrush)!=-1)
   {
      Warning(("Hey, brush %x in the current brushlist\n",curBrush));
      return FALSE;
   }
#endif
   FreeBrush(curBrush);
   return TRUE;
}

void brushClearAll(void)
{
   editBrush *brtoasted;
   CommandExecute ("unlock_all");
   while ((brtoasted=blistDelete())!=NULL)
      brushDelete(brtoasted);
   brushNextID=1;
}

bool brushWritetoFile(editBrush *brdata, int out_hnd)
{
   int sz = sizeof(editBrush) - EDITBR_MAX_FACES * sizeof(TexInfo);
   if (brushGetType(brdata)==brType_TERRAIN)
      sz+=brdata->num_faces * sizeof(TexInfo);
   return write(out_hnd,brdata,sz)==sz;
}

// warning, this returns the same address every time, really
// ie. it assumes you will then copy it to somewhere useful
editBrush *brushReadfromFile(int in_hnd)
{
   static editBrush brdata;
   int sz = sizeof(editBrush) - EDITBR_MAX_FACES * sizeof(TexInfo);
   
   if (read(in_hnd,&brdata,sz)==sz)
   {
      if (brushGetType(&brdata)==brType_TERRAIN)
         read(in_hnd,&brdata.txs,sizeof(TexInfo)*brdata.num_faces);
      return &brdata;
   } else
      return NULL;
}

editBrush *oldbrushReadfromFile(int in_hnd)
{
   static editBrush brdata;
   int sz = sizeof(editBrush) - EDITBR_MAX_FACES * sizeof(TexInfo);
   
   if (read(in_hnd,&brdata,sz)==sz)
   {
      if (brushGetType(&brdata)==brType_TERRAIN)
         read(in_hnd,&brdata.txs,sizeof(TexInfo)*brdata.num_faces);
      else
         lseek(in_hnd,sizeof(TexInfo)*brdata.num_faces,SEEK_CUR);
      return &brdata;
   } else
      return NULL;
}

// set the type of a brush, knows about the secret negative thing
int brushSetType(editBrush *br, int type)
{
   if (type!=brType_TERRAIN)
      br->media=-type;
   else if (br->media<0)
      br->media=1;
   return br->media;
}

// get the type of a brush
int brushGetType(editBrush *br)
{
   return ((br)->media>0)?brType_TERRAIN:-(br)->media;
}
