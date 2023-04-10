/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgdecal.c,v 1.4 2000/02/19 12:54:58 toml Exp $

#include <lg.h>
#include <dev2d.h>
#include <csg.h>
#include <bspdata.h>
#include <wr.h>
#include <wrdbrend.h>
#include <portal.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////
//
//  decal hack
//
//

bool decal_hack;

static uchar bits16[16][16] =
{
  {  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {  0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {  0,0,2,2,0,0,2,2,2,0,0,0,0,0,0,0 },
  {  0,0,0,2,0,0,2,0,0,0,0,0,0,0,0,0 },
  {  0,0,0,2,0,0,2,2,0,0,0,0,2,0,0,0 },
  {  0,0,0,2,2,0,0,2,0,2,2,2,0,0,0,0 },
  {  0,0,0,0,2,7,9,17,20,2,0,0,0,0,0,0 },
  {  0,0,0,0,2,0,31,38,0,0,2,0,0,0,0,0 },
  {  0,0,0,2,2,2,2,0,0,0,2,2,0,0,0,0 },
  {  0,0,2,2,0,0,2,2,2,0,0,2,0,0,0,0 },
  {  0,2,2,0,0,0,0,2,0,0,0,2,2,0,0,0 },
  {  0,0,0,0,0,2,2,2,2,0,0,0,0,0,0,0 },
  {  0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0 },
  {  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  {  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
};

static uchar bits8[8][8] =
{
  {  0,0,0,0,0,0,0,0 },
  {  0,2,0,2,0,0,0,0 },
  {  0,2,0,2,0,0,0,0 },
  {  0,0,2,0,2,2,0,0 },
  {  0,0,2,2,0,2,0,0 },
  {  0,2,0,2,0,2,0,0 },
  {  0,0,0,2,0,0,0,0 },
  {  0,0,0,0,0,0,0,0 },
};

static uchar bits4[4][4] =
{
  { 0,0,0,0 },
  { 0,2,0,0 },
  { 0,2,2,0 },
  { 0,0,0,0 },
};

static uchar bits2[2][2] = { 2,0,0,0 };

static uchar bits1[1][1] = { 0 };

static grs_bitmap goof_decal[5];

r3s_texture hacked_decal(void)
{
   static int init;
   if (!init) {
      init = 1;

      gr_init_bitmap(goof_decal  , &bits16[0][0], BMT_FLAT8, BMF_TRANS, 16, 16);
      gr_init_bitmap(goof_decal+1, &bits8[0][0] , BMT_FLAT8, BMF_TRANS,  8,  8);
      gr_init_bitmap(goof_decal+2, &bits4[0][0] , BMT_FLAT8, BMF_TRANS,  4,  4);
      gr_init_bitmap(goof_decal+3, &bits2[0][0] , BMT_FLAT8, BMF_TRANS,  2,  2);
      gr_init_bitmap(goof_decal+4, &bits1[0][0] , BMT_FLAT8, BMF_TRANS,  1,  1);
   }
   return goof_decal;
}

#define MAX_TEST_DECALS 64
PortalDecal test_decal[MAX_TEST_DECALS];

int decal_compare(const void *p, const void *q)
{
   PortalDecal *a = (PortalDecal *) p;
   PortalDecal *b = (PortalDecal *) q;

   if (a->offset < b->offset)
      return -1;
   else
      return a->offset > b->offset;
}

#ifdef DOUG_DIDNT_SUCK
void NEW_build_polygon_decals(PortalLightMap *p, int brush, int face)
{
   // need a real API to this stuff
//   decal_setup(brush,face);
}
#endif

void build_polygon_decals(PortalLightMap *p, int brush, int face)
{
   int n;
   test_decal[0].offset = 256 * 256;
   test_decal[0].decal = 0;
   n = 1;

#if 0
   // add decal in world space
   {
      mxs_vector where;
      float u,v;
      int x,y;
      where.x = where.y = where.z = 4.5;
          // 4.5 is one texture + half a tile in worldspace
      compute_poly_uv_from_loc(&where, &u, &v);
      x = (int) floor(u*4) - p->base_u;
      y = (int) floor(v*4) - p->base_v;
      if (x >= 0 && x < p->w && y >= 0 && y < p->h) {
         test_decal[n].offset = (y << 8) + x;
         test_decal[n].decal = hacked_decal();
         ++n;
      }
   }
#endif

#if 0
   // add decals in brush space

   { int i,j;
    for (j=-4; j < 4; ++j) {
     for (i=-4; i < 4; ++i) {
      mxs_vector where;
      float u,v;
      int x,y;
      compute_loc_from_brush_uv(&where, i+0.125, j+0.125);
          // 0.125 is center of a tile in uv space
      compute_poly_uv_from_loc(&where, &u, &v);
      x = (int) floor(u*4) - p->base_u;
      y = (int) floor(v*4) - p->base_v;
      if (x >= 0 && x < p->w && y >= 0 && y < p->h) {
         test_decal[n].offset = (y << 8) + x;
         test_decal[n].decal = hacked_decal();
         if (++n == MAX_TEST_DECALS) goto done;
      }
     }
    }
  done:
   }
#endif

   if (n == 1)
      p->decal = 0;
   else {
      qsort(test_decal, n, sizeof(test_decal[0]), decal_compare);
      p->decal = Malloc(n * sizeof(test_decal[0]));
      memcpy(p->decal, test_decal, n*sizeof(test_decal[0]));
   }
}
