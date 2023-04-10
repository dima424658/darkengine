/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/celstobj.h,v 1.4 2000/01/29 13:38:36 adurant Exp $
#pragma once

#ifndef _CELSTOBJ_H_
#define _CELSTOBJ_H_


// We are assuming only one implementation of this - the implementation file contains statics!

#include <skyint.h>
#include <lgd3d.h>
#include <r3d.h>

struct sStructDesc;
struct sMissionCelestialObj;
struct IRes;


struct goddamn_stupid_r3s_point
{
   mxs_vector p;
   ulong ccodes;
   g2s_point grp;
};


class cCelestialObject : public ISkyObject
{
public:
   cCelestialObject(int nIndex);
   virtual void AppInit();
   virtual void Init();
   virtual void Term();
   virtual void Render();

private:
   void SetDefaults();
   void SetEasterEggDefaults();

   int nIndex;
   sMissionCelestialObj *pCelestialObj;

   BOOL bDidGameInit;
   IRes *pCelestRes;
   mxs_vector Points[4];
   goddamn_stupid_r3s_point pcorn[4];
   r3s_phandle vlist[4];
   int nPalIx;

};

#endif