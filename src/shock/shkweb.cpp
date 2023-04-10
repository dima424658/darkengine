/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header:

#include <math.h>

#include <mprintf.h>
#include <random.h>
#include <dev2d.h>

#include <shkweb.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Renders the web in a 3d context in a color
void sWeb::Render(int color)
{
   r3_set_color(color);

   r3_transform_block(m_spokes,m_pPOuter,m_pOuter);
   r3_transform_block(m_spokes,m_pPInner,m_pInner);

   int i;
   for (i=0;i<m_spokes;++i)
   {
      r3_draw_line(&m_pPOuter[i],&m_pPInner[i]);
   }
   for (i=0;i<m_spokes;++i)
   {
      r3_draw_line(&m_pPOuter[i],&m_pPOuter[(i+1)%m_spokes]);
      r3_draw_line(&m_pPInner[i],&m_pPInner[(i+1)%m_spokes]);
   }
}

sWeb::sWeb(float radius,int spokes,float radVar,float spokeVar,float aspect,float depth)
{
   m_spokes = spokes;
   m_depth = depth;
   m_pOuter = new sMxVector[m_spokes];
   m_pInner = new sMxVector[m_spokes];
   m_pPOuter = new r3s_point[m_spokes];
   m_pPInner = new r3s_point[m_spokes];
   m_pSpokeAngles = new float[m_spokes+1];

   // Create all the spokes
   float curRad;
   float delRad = MX_REAL_2PI/m_spokes;
   int spoke;
   for (spoke=0,curRad=0;curRad+delRad<MX_REAL_2PI;spoke++,curRad+=delRad*(1+spokeVar*RandFloat()))
   {
      float curRadius = radius*(1.0-radVar*RandFloat());
      m_pOuter[spoke].Set(0,-cos(curRad)*curRadius,-sin(curRad)*curRadius*aspect);
      // Adjust for aspect ratio
      float ang = atan2(sin(curRad)*aspect,cos(curRad));
      if (ang<0) ang+=MX_REAL_2PI;
      m_pSpokeAngles[spoke] = ang;
      m_pInner[spoke] = m_pOuter[spoke];
      m_pInner[spoke].x = depth;
   }
   m_spokes = spoke;
   m_pSpokeAngles[spoke] = m_pSpokeAngles[0]+MX_REAL_2PI;

}

// Find spoke between these two spots in screen space, must be in context
// of render canvas
int sWeb::FindSpoke(int x,int y,bool rejectOnOuter)
{
   float hx = grd_bm.w/2;
   float hy = grd_bm.h/2;
   float rad = atan2(y-hy,x-hx);
   if (rad<0) rad+=MX_REAL_2PI;
   int i;
   for (i=0;i<m_spokes;++i) {
      if (rad < m_pSpokeAngles[i]) break;
   }
   int spoke = i-1;
 
   if (rejectOnOuter) {
      float a1x,a1y;
      a1x = fix_float(m_pPOuter[spoke].grp.sx);
      a1y = fix_float(m_pPOuter[spoke].grp.sy);
      // Get vector for outer   
      float ax,ay;
      ax = fix_float(m_pPOuter[(spoke+1)%m_spokes].grp.sx) - a1x;
      ay = fix_float(m_pPOuter[(spoke+1)%m_spokes].grp.sy) - a1y;

      // and vector for point to outer
      float bx,by;
      bx = x - ((a1x-hx)*.5 + hx);
      by = y - ((a1y-hy)*.5 + hy);

      if ((ax*by - ay*bx)>0) spoke = -1;
   }
   return spoke;
}

// Gives you the outer coords halfway between the spokes as defined by FindSpoke
void sWeb::SpokeCenter(sMxVector *pVec,int spoke)
{
   pVec->Interpolate(m_pOuter[spoke],m_pOuter[(spoke+1)%m_spokes],.5);
}


sWeb::~sWeb()
{
   delete m_pOuter;
   delete m_pInner;
   delete m_pPOuter;
   delete m_pPInner;
   delete m_pSpokeAngles;
}







