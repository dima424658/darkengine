/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/ccrappa.cpp,v 1.3 1998/10/05 17:20:26 mahk Exp $

#include <creature.h>
#include <matrixs.h>
#include <math.h>
#include <ccrhuman.h>

// must be last header
#include <dbmem.h>


class cApparitionCreature : public cHumanoidCreature
{
public:
   cApparitionCreature(int type,ObjID objID) :
      cHumanoidCreature(type,objID) {}

   cApparitionCreature(int type,ObjID objID, fCreatureReadWrite func, int version) : 
      cHumanoidCreature(type,objID,func,version) {}
   virtual ~cApparitionCreature() {}

   virtual void ComputeBoundingData();

private:
   void Init();
};

class cApparitionCreatureFactory : public cCreatureFactory
{
   virtual cCreature *Create(int type, ObjID objID) 
        { return new cApparitionCreature(type,objID); }
   virtual cCreature *CreateFromFile(int type, ObjID objID, fCreatureReadWrite func, int version)
        { return new cApparitionCreature(type, objID, func, version); }
};

static cApparitionCreatureFactory g_ApparitionCreatureFactory;
EXTERN cCreatureFactory *g_pApparitionCreatureFactory=&g_ApparitionCreatureFactory;

// from crappa.c
#define TOE       0
#define BUTT      8
#define HEAD      19

// apparition has own bbox func because he has a lot of joints that never
// get set, because he wants to be able to play human motions but doesn't
// have lower body.
void cApparitionCreature::ComputeBoundingData()
{
   mxs_vector bigun,max_off,min_off;
   mxs_vector *j;
   mxs_vector *bmin=&m_BMin;
   mxs_vector *bmax=&m_BMax;
   int i;

   mx_copy_vec(bmin,&m_pJoints[TOE]);
   mx_copy_vec(bmax,&m_pJoints[TOE]);


   for(i=BUTT;i<=HEAD;i++)
   {
      j=&m_pJoints[i];
      if(j->x<bmin->x)
         bmin->x=j->x;
      else if (j->x>bmax->x)
         bmax->x=j->x;
      if(j->y<bmin->y)
         bmin->y=j->y;
      else if (j->y>bmax->y)
         bmax->y=j->y;
      if(j->z<bmin->z)
         bmin->z=j->z;
      else if (j->z>bmax->z)
         bmax->z=j->z;
   }

   // calc radius
   mx_sub_vec(&max_off,bmax,&m_pJoints[m_iRootJoint]);
   mx_sub_vec(&min_off,bmin,&m_pJoints[m_iRootJoint]);
   for(i=0;i<3;i++)
   {
      if(fabs(max_off.el[i])>fabs(min_off.el[i]))
         bigun.el[i]=max_off.el[i];
      else
         bigun.el[i]=min_off.el[i];
   }
   m_Radius=mx_mag_vec(&bigun);
}
