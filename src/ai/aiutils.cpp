/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiutils.cpp,v 1.39 2000/03/19 13:35:54 adurant Exp $
//

#include <lg.h>
#include <vec2.h>

#include <doorprop.h>
#include <doorphys.h>
#include <frobctrl.h>
#include <lockprop.h>
#include <objpos.h>
#include <objscale.h>
#include <objshape.h>
#include <physapi.h>
#include <phprop.h>
#include <propbase.h>
#include <wrtype.h>
#include <phcore.h>
#include <phconst.h>
#include <phmods.h>
#include <phmodobb.h>
#include <phmodsph.h>
#include <objedit.h>
#include <aiwr.h>

#include <aipthobb.h>
#include <aitagtyp.h>
#include <aiutils.h>

#include <matrix.h>

#include <simtime.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

// #define kInterEps 0.0002

float kInterEps = 0.0006;

int GetXYIntersection(const mxs_vector & line1a, const mxs_vector & line1b,
                      const mxs_vector & line2a, const mxs_vector & line2b,
                      mxs_vector * pResult)
{
   mxs_vector vec1, vec2, vec3;

   Vec2Sub(&vec1, &line1b, &line1a);
   Vec2Sub(&vec2, &line2b, &line2a);
   Vec2Sub(&vec3, &line1a, &line2a);

   double slope1, slope2, slope3;

   slope1 = (ffabsf(vec1.x) > kInterEps) ? (double)vec1.y / (double)vec1.x : DBL_MAX;
   slope2 = (ffabsf(vec2.x) > kInterEps) ? (double)vec2.y / (double)vec2.x : DBL_MAX;
   if (fabs(slope1 - slope2) < kInterEps)
   {
      if (aflt(vec3.x, kInterEps) && aflt(vec3.y, kInterEps))
         return kLineIdent;

      slope3 = (ffabsf(vec3.x) > kInterEps) ? (double)vec3.y / (double)vec3.x : DBL_MAX;
      if (fabs(slope1 - slope3) < kInterEps)
         return kLineIdent;
      return kLineNoIntersect;
   }

   double s = ((((double)vec3.y * (double)vec2.x) - ((double)vec2.y * (double)vec3.x)) /
               (((double)vec2.y * (double)vec1.x) - ((double)vec1.y * (double)vec2.x)));


   pResult->x = (float)((double)line1a.x + ((double)vec1.x * s));
   pResult->y = (float)((double)line1a.y + ((double)vec1.y * s));

   return kLineIntersect;
}

///////////////////////////////////////////////////////////////////////////////

int GetXYSegmentIntersect(const mxs_vector & seg1a, const mxs_vector & seg1b,
                          const mxs_vector & seg2a, const mxs_vector & seg2b,
                          mxs_vector * pResult)
{
   int fLine = GetXYIntersection(seg1a, seg1b, seg2a, seg2b, pResult);

   if (fLine == kLineIntersect)
   {
      if (Vec2PointSegmentSquared((Vec2 *)pResult, (Vec2 *)&seg1a, (Vec2 *)&seg1b) > sq(kInterEps) ||
          Vec2PointSegmentSquared((Vec2 *)pResult, (Vec2 *)&seg2a, (Vec2 *)&seg2b) > sq(kInterEps))
      {
         return kLineNoIntersect;
      }
   }
   return fLine;
}

///////////////////////////////////////////////////////////////////////////////

int GetXYLineSegIntersect(const mxs_vector & line1a, const mxs_vector & line1b,
                          const mxs_vector & seg2a, const mxs_vector & seg2b,
                          mxs_vector * pResult)
{
   int fLine = GetXYIntersection(line1a, line1b, seg2a, seg2b, pResult);

   if (fLine == kLineIntersect)
   {
      if (Vec2PointSegmentSquared((Vec2 *)pResult, (Vec2 *)&seg2a, (Vec2 *)&seg2b) > sq(kInterEps))
      {
         return kLineNoIntersect;
      }
   }
   return fLine;
}

///////////////////////////////////////////////////////////////////////////////

BOOL GetObjPosition(ObjID obj, Position * p)
{
   if (obj != OBJ_NULL)
   {
      ObjPos* pos = ObjPosGet(obj);

      if (pos)
      {
         CellFromLoc(&pos->loc);
         if (pos->loc.cell != CELL_INVALID)
         {
            memcpy(p, pos, sizeof(*p));
            return TRUE;
         }
      }
   }
   memset(p, 0, sizeof(*p));
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

const mxs_vector * GetObjLocation(ObjID obj)
{
   static cMxsVector zeroVec(0, 0, 0);
   if (obj != OBJ_NULL)
   {
      ObjPos* pos = ObjPosGet(obj);

      if (pos)
      {
         CellFromLoc(&pos->loc);
         if (pos->loc.cell != CELL_INVALID)
            return &pos->loc.vec;
      }
   }
   return &zeroVec;
}

///////////////////////////////////////////////////////////////////////////////

BOOL GetObjLocation(ObjID obj, mxs_vector * p)
{
   if (obj != OBJ_NULL)
   {
      ObjPos* pos = ObjPosGet(obj);

      if (pos)
      {
         CellFromLoc(&pos->loc);
         if (pos->loc.cell != CELL_INVALID)
         {
            *p = pos->loc.vec;
            return TRUE;
         }
      }
   }
   p->x = p->y = p->z = 0.0;
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

BOOL GetObjFacing(ObjID obj, floatang * pFacing)
{
   if (obj != OBJ_NULL)
   {
      ObjPos* pos = ObjPosGet(obj);

      if (pos)
      {
         CellFromLoc(&pos->loc);
         if (pos->loc.cell != CELL_INVALID)
         {
            *pFacing = (pos->fac.tz * TWO_PI) / (2*MX_ANG_PI);
            return TRUE;
         }
      }
   }
   *pFacing = 0.0;
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////

void GetObjVelocity(ObjID obj, mxs_vector * p)
{
   PhysGetVelocity(obj, p);
}

///////////////////////////////////////////////////////////////////////////////

void GetObjAngVelocity(ObjID obj, mxs_vector * p)
{
   PhysGetRotationalVelocity(obj, p);
}

///////////////////////////////////////////////////////////////////////////////

void SetObjImpulse(ObjID obj, float x, float y, float z, float facing, BOOL rotating)
{
   mxs_vector phys_heading;
   mxs_angvec phys_facing;

   mx_mk_vec(&phys_heading, 7.5*x, 7.5*y, 7.5*z);

   if (!rotating)
   {
      PhysControlVelocity(obj, &phys_heading);
   }
   else
   {
      mxs_vector non_z_heading;
      mx_mk_vec(&non_z_heading, phys_heading.x, phys_heading.y, 0);

      mx_mk_angvec(&phys_facing, 0, 0, (ushort)((facing * 180) / PI));

      PhysAxisControlVelocity(obj, 0, (mx_mag_vec(&non_z_heading)));

      // "lift"
      PhysAxisControlVelocity(obj, 2, 6*z);

      // We might want to smooth rather than slam this
#if 1
      PhysGetModRotation(obj, &phys_facing);
      phys_facing.tz += (facing * MX_ANG_PI / MX_REAL_PI);

      PhysSetModRotation(obj, &phys_facing);
#else
      PhysAxisControlRotationalVelocity(obj, 2, facing * MX_REAL_PI * 20 / 180);
#endif
   }
}


///////////////////////////////////////////////////////////////////////////////

// Gosh, it would be nice if there was a physics interface for this.
BOOL AIGetPhysSphereZMinMax(ObjID obj, float *pMinZ, float *pMaxZ)
{
   int i;
   cPhysModel *pPhysModel;
   int nNum;
   float fMinZ = 100000;
   float fMaxZ = -100000;
   float fZ;
   float fRad;
   BOOL bFoundOne = FALSE;

   if (((pPhysModel = g_PhysModels.GetActive(obj)) == NULL) || ((nNum = pPhysModel->NumSubModels()) == 0))
   {
      *pMinZ = 0;
      *pMaxZ = 0;
      return FALSE;
   }

   for (i = 0; i < nNum; i++)
   {
      fZ = pPhysModel->GetSubModOffset(i).z;
      if (pPhysModel->GetType(i) != kPMT_Sphere)
         continue;

      bFoundOne = TRUE;

      fRad = ((cPhysSphereModel *)pPhysModel)->GetRadius(i);

      if ((fZ+fRad) > fMaxZ)
         fMaxZ = fZ+fRad;
      if ((fZ-fRad) < fMinZ)
         fMinZ = fZ-fRad;
   }

   if (bFoundOne)
   {
      *pMinZ = fMinZ;
      *pMaxZ = fMaxZ;
   }
   else
   {
      *pMinZ = 0;
      *pMaxZ = 0;
   }

   return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

static BOOL GetPhysOBBModelBBox(ObjID obj, mxs_vector *pos, mxs_angvec *fac, mxs_vector* bmin, mxs_vector* bmax)
{
   cPhysModel *pModel;
   mxs_vector bbox;

   if ((pModel = g_PhysModels.GetActive(obj)) == NULL)
      return FALSE;

   if (pModel->GetType(0) != kPMT_OBB)
      return FALSE;

   *fac = pModel->GetRotation();

   mxs_matrix obb_rot;
   mxs_vector obb_offset;

   mx_ang2mat(&obb_rot, fac);
   mx_mat_mul_vec(&obb_offset, &obb_rot, &((cPhysOBBModel *)pModel)->GetOffset());

   mx_add_vec(pos, &pModel->GetLocationVec(), &obb_offset);
   mx_copy_vec(&bbox, &((cPhysOBBModel *)pModel)->GetEdgeLengths());

   mx_scale_vec(bmin, &bbox, -0.5);
   mx_scale_vec(bmax, &bbox,  0.5);

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

static void GetFloorLoc(ObjID objID, const mxs_vector * pAltLoc, mxs_vector * pLoc)
{
   // First, get the true floor location plus some
   Location start, end, hit;

   ObjPos * pPos = ObjPosGet(objID);
   if ((pPos = ObjPosGet(objID)) == NULL || pPos->loc.cell == CELL_INVALID)
      return;
      
   if (!pAltLoc)
   {
      MakeHintedLocationFromVector(&start, &pPos->loc.vec, &pPos->loc);
      MakeHintedLocationFromVector(&end, &pPos->loc.vec, &pPos->loc);
   }
   else
   {
      MakeHintedLocationFromVector(&start, pAltLoc, &pPos->loc);
      MakeHintedLocationFromVector(&end, pAltLoc, &pPos->loc);
   }

   end.vec.z -= 50.0;

   UpdateChangedLocation(&end);

   AIRaycast(&start, &end, &hit, kAIR_NoHintWarn);

   *pLoc = hit.vec;
   pLoc->z += 0.1;
}

///////////////////////////////////////

extern BOOL AIPathExactOBB(ObjID object); // clean up properly after ship T2 (toml 03-06-00)

BOOL AIGetObjFloorBBox(ObjID object, tAIFloorBBox * pBBox, const mxs_vector * pAltLoc, const mxs_angvec * pAltFac)
{
   AssertMsg(!IsSimTimePassing(), "Calling function too slow for sim time!");
   
   const ObjPos * pPos;
   mxs_vector     boxMinMax[2];
   mxs_matrix     orientation;

   if ((pPos = ObjPosGet(object)) != NULL &&
       pPos->loc.cell != CELL_INVALID)
   {
      mxs_angvec baseFac = pPos->fac;
      mxs_vector loc = pPos->loc.vec;
      mxs_vector floorLoc;
      GetFloorLoc(object, pAltLoc, &floorLoc);

      if (!GetPhysOBBModelBBox(object, &loc, &baseFac, &boxMinMax[0], &boxMinMax[1]))
      {
         if (!ObjGetObjRelBBox(object, &boxMinMax[0], &boxMinMax[1]))
            return FALSE;
      }

      int        i;
      mxs_angvec fac = (!pAltFac) ? baseFac : *pAltFac;
      mxs_vector temp;

      // Detect possible 0-dimension door.
      if (fabs(boxMinMax[0].x-boxMinMax[1].x) < 0.001)
         Warning(("Object %d has possible 0-sized bounding box\n", object));

      fac.tz = 0;
      mx_ang2mat(&orientation, &fac);
      BoundBBox(&orientation, &boxMinMax[0], &boxMinMax[1]);

      // @TBD (toml 03-06-00): I'm not sure why we grow at all, or at least, 
      // why so much, and why as a scale and not a simple add. In any evernt, 
      // this should be investigated post T2
      if (!AIPathExactOBB(object))
      {
         #define kBoxGrow 0.020
         mx_scaleeq_vec(&boxMinMax[0], 1.0 + kBoxGrow / 2);
         mx_scaleeq_vec(&boxMinMax[1], 1.0 + kBoxGrow / 2);
      }

      float z = max(floorLoc.z, loc.z + boxMinMax[0].z);

      for (i = 0; i < 4; i++)
      {
         temp.x = boxMinMax[i/2].x;
         temp.y = boxMinMax[(i && i < 3) ? 1 : 0].y;
         temp.z = 0.0;
         mx_rot_z_vec(&((*pBBox)[i]), &temp, (!pAltFac) ? baseFac.tz : pAltFac->tz);

         if (!pAltLoc)
         {
            (*pBBox)[i].x += loc.x;
            (*pBBox)[i].y += loc.y;
         }
         else
         {
            (*pBBox)[i].x += pAltLoc->x;
            (*pBBox)[i].y += pAltLoc->y;
         }
         (*pBBox)[i].z = z;
      }
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Door manipulation
//

// Also @TODO: We'll want property or object listeners so we can keep
// this list up-to-date as doors are destroyed during the game.

static cDynArray<sAIDoorInfo> _g_Doors;

///////////////////////////////////////

const cDynArray<sAIDoorInfo> & AIGetDoors()
{
   return _g_Doors;
}


static int DoorSearch(const void * pKey, const sAIDoorInfo * pRight)
{
   if (((ObjID)(pKey)) < pRight->obj)
      return -1;
   else if (((ObjID)(pKey)) > pRight->obj)
      return 1;
   else
      return 0;
}


BOOL DoorIsBig(ObjID door)
{
   int nIndex = _g_Doors.BSearch((void *)door, DoorSearch);
   return (nIndex != BAD_INDEX) && (_g_Doors[nIndex].flags & kAIDF_LargeDoor);
}



///////////////////////////////////////

static int DoorCompare(const sAIDoorInfo *pLeft, const sAIDoorInfo *pRight)
{
   if (pLeft->obj < pRight->obj)
      return -1;
   else
      return 1;
}


// from aipathdb.cpp:
extern float *g_pLargeDoorSize;


void AIUpdateDoors()
{
   _g_Doors.SetSize(0);

   sAIDoorInfo       info;
   sPropertyObjIter  iter;
   ObjID             obj;
   sRotDoorProp *    pRotDoor;
   sTransDoorProp *  pTransDoor;
   sAIDoorBBox      current;
   BOOL             pathable;
   mxs_vector diagnal;
   float fSize;

   _g_Doors.SetSize(0);

   // doors which block visibility, both rotating and translating
   g_pRotDoorProperty->IterStart(&iter);
   while (g_pRotDoorProperty->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      if (g_pObjPathableProperty->Get(obj, &pathable) && pathable)
         continue;

      g_pRotDoorProperty->Get(obj, &pRotDoor);
      info.obj = obj;
      info.flags = 0;  //Clear the flags.
      if (pRotDoor->vision_blocking)
         info.flags |= kAIDF_BlocksVision;

      // See what the rotated X/Y extents are, set bit.
      // Use diagnal of bounding box. Double doors are problem for now...
      current.object = obj;
      AIGetObjFloorBBox(obj,
                        &current.bbox,
                        &pRotDoor->base_closed_location,
                        &pRotDoor->base_closed_facing);
      mx_sub_vec(&diagnal, current.bbox, current.bbox+2);
      fSize = mx_mag_vec(&diagnal);
      if (fSize >= *g_pLargeDoorSize)
         info.flags |= kAIDF_LargeDoor;

      _g_Doors.Append(info);
   }
   g_pRotDoorProperty->IterStop(&iter);

   g_pTransDoorProperty->IterStart(&iter);
   while (g_pTransDoorProperty->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      if (g_pObjPathableProperty->Get(obj, &pathable) && pathable)
         continue;

      g_pTransDoorProperty->Get(obj, &pTransDoor);
      info.obj = obj;
      info.flags = 0; //Clear the flags
      if (pTransDoor->vision_blocking)
         info.flags |= kAIDF_BlocksVision;

      // See what the rotated X/Y extents are, set bit. Double doors are problem for now...

      current.object = obj;
      AIGetObjFloorBBox(obj,
                        &current.bbox,
                        &pTransDoor->base_closed_location);

      mx_sub_vec(&diagnal, current.bbox, current.bbox+2);
      fSize = mx_mag_vec(&diagnal);
      if (fSize >= *g_pLargeDoorSize)
         info.flags |= kAIDF_LargeDoor;

      _g_Doors.Append(info);
   }
   g_pTransDoorProperty->IterStop(&iter);

   _g_Doors.Sort(DoorCompare);
}

///////////////////////////////////////

BOOL AICanOpenDoor(ObjID ai, ObjID obj)
{
   if (!IsDoor(obj) || GetDoorStatus(obj) == kDoorOpen || GetDoorStatus(obj) == kDoorOpening ||
                       GetDoorStatus(obj) == kDoorHalt)
      return TRUE;

   sObjAccess accesses[MAX_OBJ_ACCESS];
   return CheckObjectAccess(ai, obj, accesses, FALSE) != 0;
}


///////////////////////////////////////

BOOL AIDoorIsOpen(ObjID obj)
{
   if (IsDoor(obj))
      return (GetDoorStatus(obj) == kDoorOpen);
   return TRUE;
}

static BOOL AIUseDoorControllers(ObjID ai, ObjID obj, BOOL all)
{
   sObjAccess accesses[MAX_OBJ_ACCESS];
   int        nAccesses;

   nAccesses = CheckObjectAccess(ai, obj, accesses, all);
   if (nAccesses)
   {
      sFrobActivate frob_info;
      frob_info.frobber = ai;
      frob_info.ms_down = 1; // since we dont have time data...

      for (int i = 0; i < nAccesses; i++)
      {
         if (accesses[i].frober == OBJ_NULL)
         {
            frob_info.src_obj = accesses[i].frobee;
            frob_info.src_loc = kFrobLocWorld;
            frob_info.dst_obj = OBJ_NULL;
            frob_info.dst_loc = kFrobLocNone;
         }
         else
         {
            frob_info.src_obj = accesses[i].frober;
            frob_info.src_loc = kFrobLocTool;
            frob_info.dst_obj = accesses[i].frobee;
            frob_info.dst_loc = kFrobLocWorld;
         }

         FrobInstantExecute(&frob_info);

         if (accesses[i].flags&kObjAccessOr)
            return TRUE; // it is an or, and we have done one of them, head home
      }
      return TRUE;
   }
   return FALSE;
}

///////////////////////////////////////

void AIOpenDoor(ObjID ai, ObjID obj)
{
   if (!IsDoor(obj))
      return;

   if ((GetDoorStatus(obj) == kDoorOpen) ||
       (GetDoorStatus(obj) == kDoorOpening) ||
       (GetDoorStatus(obj) == kDoorHalt)) 
      return;

   if (!AIUseDoorControllers(ai,obj,FALSE))
      Warning(("AI %s called AIOpenDoor() on %s which it cant close\n",ObjWarnName(ai),ObjWarnName(obj)));
}

///////////////////////////////////////

void AICloseDoor(ObjID ai, ObjID obj, BOOL all)
{
   if (!IsDoor(obj))
      return;

   if ((GetDoorStatus(obj) == kDoorClosed) ||
       (GetDoorStatus(obj) == kDoorClosing))
      return;

   if (!AIUseDoorControllers(ai,obj,all)) //all defaults to TRUE, old behaviour.
      Warning(("AI %s called AICloseDoor() on %s which it cant close\n",ObjWarnName(ai),ObjWarnName(obj)));
}

///////////////////////////////////////

void AIGetDoorFloorBBoxes(cDynArray<sAIDoorBBox> * pResult)
{
   sPropertyObjIter iter;
   ObjID            obj;
   sRotDoorProp *   pRotDoor;
   sTransDoorProp * pTransDoor;
   sAIDoorBBox      current;
   BOOL             pathable;

   g_pRotDoorProperty->IterStart(&iter);
   while (g_pRotDoorProperty->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;
      
      if (g_pObjPathableProperty->Get(obj, &pathable) && pathable)
         continue;

      current.object = obj;
      g_pRotDoorProperty->Get(obj, &pRotDoor);
      AIGetObjFloorBBox(obj,
                        &current.bbox,
                        &pRotDoor->base_closed_location,
                        &pRotDoor->base_closed_facing);
      pResult->Append(current);
   }
   g_pRotDoorProperty->IterStop(&iter);

   g_pTransDoorProperty->IterStart(&iter);
   while (g_pTransDoorProperty->IterNext(&iter, &obj))
   {
      if (OBJ_IS_ABSTRACT(obj))
         continue;

      if (g_pObjPathableProperty->Get(obj, &pathable) && pathable)
         continue;

      current.object = obj;
      g_pTransDoorProperty->Get(obj, &pTransDoor);

      tAIFloorBBox & bbox = current.bbox;
      AIGetObjFloorBBox(obj,
                        &bbox,
                        &pTransDoor->base_closed_location);
      pResult->Append(current);
   }
   g_pTransDoorProperty->IterStop(&iter);
}

///////////////////////////////////////////////////////////////////////////////

void cAITimer::Save(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &m_Expiration);
   AITagMove(pTagFile, &m_Period);
}

///////////////////////////////////////

void cAITimer::Load(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &m_Expiration);
   AITagMove(pTagFile, &m_Period);
}


///////////////////////////////////////

void cAIRandomTimer::Save(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &m_Expiration);
   AITagMove(pTagFile, &m_Low);
   AITagMove(pTagFile, &m_High);
}

///////////////////////////////////////

void cAIRandomTimer::Load(ITagFile * pTagFile)
{
   AITagMove(pTagFile, &m_Expiration);
   AITagMove(pTagFile, &m_Low);
   AITagMove(pTagFile, &m_High);
}

///////////////////////////////////////////////////////////////////////////////

void CalcArc(mxs_vector* pDir, float initVelocity, float gravity)
{
   // Compensate for gravity
   if (gravity > 0)
   {
      mxs_real slipallowed = 0.05;
      mxs_real initpz = pDir->z;
      //downward gravity in feet/second/second
      mxs_real gravfactor = gravity * kGravityDir.z * kGravityAmt/100.0;
      //height lost to gravity
      mxs_real gravloss;
      mxs_real timetotarget;
      mxs_real range;
      int loopbreak;
     
      gravloss = slipallowed * 10; //guaranteed to fail first for loop iteration
      //successive approximation;
      for (loopbreak=0;(loopbreak<20)&&(fabs(pDir->z + gravloss - initpz)>slipallowed);loopbreak++)
      {
         // compensate.  Done here from last time's gravloss.
         pDir->z = initpz - gravloss;
         //get range
         range = mx_mag_vec(pDir);
         //t = r/v
         timetotarget = range/initVelocity;
         // (0.5)at^2
         gravloss = (0.5) * gravfactor * sq(timetotarget);  
      }
      //if we still didn't get it, use the previous CalcArc method
      if (loopbreak == 20)
      {
         mxs_real angle;            
         mxs_real z_comp;      
         mxs_real arcsin_val;

         pDir->z = initpz;
         range = mx_mag_vec(pDir);
         arcsin_val = (gravity * kGravityDir.z * kGravityAmt * range / 100.0) / sq(initVelocity);      
         
         if (abs(arcsin_val) >= 1.0)         
            angle = 3.1416 / 4;      
         else         
            angle = asin(arcsin_val) / 2;

         z_comp = tan(angle) * range * 1.5;
      
         if (pDir->z < 0)
            z_comp *= 1 + (-pDir->z / 25);      
         else         
            z_comp /= 1 + (pDir->z / 15);
         
         pDir->z += fabs(z_comp);
      }

   }
}
