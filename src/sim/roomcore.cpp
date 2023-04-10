/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomcore.cpp,v 1.33 2000/03/18 15:25:33 ccarollo Exp $
//
// Room System base functions
//

#include <lg.h>

#include <command.h>
#include <config.h>
#include <comtools.h>
#include <appagg.h>
#include <traitman.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <objpos.h>
#include <objedit.h>
#include <posprop.h>
#include <propman.h>
#include <propbase.h>
#include <wr.h>
#include <port.h>
#include <math.h>

#include <rand.h> //for new room brushing I think. AMSD
                  //(undefed RandRange without pch).

#ifdef EDITOR
#include <brlist.h>
#include <brinfo.h>
#include <brrend.h>
#include <editbr_.h>
#endif
#include <r3d.h>
#include <lgd3d.h>
#include <rendobj.h>

#include <roombase.h>
#include <rooms.h>
#include <room.h>
#include <roomutil.h>
#include <rpaths.h>

#include <ged_room.h>
#include <rpathfnd.h>
#include <psnd.h>
#include <doorphys.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>


////////////////////////////////////////////////////////////////////////////////

cRooms *g_pRooms = NULL;

void SpewRooms(void);
void SpewRoomObj(ObjID obj);
void FixRooms();
void SpewBadRoomObjs();

BOOL g_show_player_room = FALSE;
BOOL g_show_bad_rooms = FALSE;

static cDynArray<mxs_vector> gBadRoomPts;

#ifndef SHIP
static Command g_RoomCommands[] = 
{
   {"rooms_spew",    FUNC_VOID,  SpewRooms,           "Spew entire room database",      HK_ALL},
   {"spew_room_obj", FUNC_INT,   SpewRoomObj,         "List room for specified object", HK_ALL},
   {"show_player_room", TOGGLE_BOOL, &g_show_player_room, "Draw the player's room obj", HK_ALL},
   {"show_bad_rooms",   TOGGLE_BOOL, &g_show_bad_rooms, "Draw areas outside the room db", HK_ALL},
   {"spew_bad_room_objs", FUNC_VOID, SpewBadRoomObjs, "List all rooms with bad room objIDs", HK_ALL},
   {"door_slam_open", FUNC_INT,  DoorSlamOpen, "", HK_ALL},
   {"door_slam_closed", FUNC_INT, DoorSlamClosed, "", HK_ALL},
};
#endif

//
// Listen to object position changes
//
static void obj_pos_cb(ObjID obj, const ObjPos* pos, void* data)
{
   cRooms* pRooms = (cRooms*)data;
   mxs_vector loc = pos->loc.vec; 
   pRooms->UpdateRoomData(obj,loc); 
}

////////////////////////////////////////////////////////////////////////////////
//
// Initialize empty room database
//
void InitRooms(void)
{
   AssertMsg(g_pRooms == NULL, "Attempt to initialize non-NULL room system");

   #ifndef SHIP
   COMMANDS(g_RoomCommands, HK_ALL);
   #endif

   g_pRooms = new cRooms;
   ObjPosListen(obj_pos_cb,g_pRooms); 

   g_RoomsOK = FALSE;
}

////////////////////////////////////////
//
// Delete all rooms and room data (watches, callbacks, etc)
//
void TermRooms(void)
{
   AssertMsg(g_pRooms != NULL, "Attempt to terminate NULL room system");

   delete g_pRooms;
   g_pRooms = NULL;
}

////////////////////////////////////////
//
// Delete all rooms
//
void ClearRooms(void)
{
   if (g_pRooms)
      g_pRooms->ClearRooms();

   g_RoomsOK = FALSE;

   gBadRoomPts.SetSize(0);
}

////////////////////////////////////////
//
// Load all rooms from disk
//

void LoadRooms(RoomReadWrite movefunc)
{
   movefunc(&g_RoomsOK, sizeof(BOOL), 1);

   if (g_RoomsOK)
   {
      if (g_pRooms)
         g_pRooms->Read(movefunc);
   }

   g_RoomsOK = TRUE;
}

void PostLoadRooms(void)
{

   for (int i=0; i<g_pRooms->GetNumWatches(); i++)
      g_pRooms->ClearWatch(i);

   // Grab the position property and iterate across it

   AutoAppIPtr_(PropertyManager,pPropMan); 

   IProperty* pprop = pPropMan->GetPropertyNamed(PROP_POSITION_NAME); 
   IPositionProperty* prop; 
   Verify(SUCCEEDED(pprop->QueryInterface(IID_IPositionProperty,(void**)&prop))); 
   sPropertyObjIter iter; 
   ObjID obj; 
   Position* pos; 
   prop->IterStart(&iter); 

   while (prop->IterNextValue(&iter,&obj,&pos))
      if (OBJ_IS_CONCRETE(obj))
      {
         g_pRooms->FindObjRoom(obj,pos->loc.vec); 
      }
   prop->IterStop(&iter);
   SafeRelease(prop);
   SafeRelease(pprop); 
} 

void InitObjRooms(void)
{
   AutoAppIPtr_(PropertyManager,pPropMan); 

   IProperty* pprop = pPropMan->GetPropertyNamed(PROP_POSITION_NAME); 
   IPositionProperty* prop; 
   Verify(SUCCEEDED(pprop->QueryInterface(IID_IPositionProperty,(void**)&prop))); 
   sPropertyObjIter iter; 
   ObjID obj; 
   Position* pos; 
   prop->IterStart(&iter); 

   while (prop->IterNextValue(&iter,&obj,&pos))
   {
      if (OBJ_IS_CONCRETE(obj))
      {
         g_pRooms->FindObjRoom(obj,pos->loc.vec);
         if (g_pRooms->GetObjRoom(obj))
            SendRoomMessages(OBJ_NULL, g_pRooms->GetObjRoom(obj)->GetObjID(), obj);
      }
   }
   prop->IterStop(&iter);
   SafeRelease(prop);
   SafeRelease(pprop); 
} 

////////////////////////////////////////
//
// Save all rooms to disk
//
void SaveRooms(RoomReadWrite movefunc)
{
   movefunc(&g_RoomsOK, sizeof(BOOL), 1);

   if (g_RoomsOK)
   {
      if (g_pRooms)
         g_pRooms->Write(movefunc);
   }
}

////////////////////////////////////////
//
// Spew everything about the room database
//
void SpewRooms(void)
{
   cRoom       *curRoom;
   cRoomPortal *curPortal;
   ObjID donor;
   int i, j, k;

   AutoAppIPtr_(TraitManager, pTraitMan);

   for (i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      curRoom = g_pRooms->GetRoom(i);

      donor = pTraitMan->GetArchetype(curRoom->GetObjID());

      mprintf("\n");
      mprintf("[ROOM %d    Obj %d   Name \"%s\"]\n", curRoom->GetRoomID(), curRoom->GetObjID(), curRoom->GetName());
      mprintf("   Archetype: %s\n", ObjWarnName(donor)) ;
      mprintf("   Center: (%g %g %g)\n", curRoom->GetCenterPt().x, curRoom->GetCenterPt().y, curRoom->GetCenterPt().z);

      for (j=0; j<6; j++)
         mprintf("     Plane %d: (%g %g %g) %g\n", j, curRoom->GetPlane(j).normal.x, curRoom->GetPlane(j).normal.y,
                                                        curRoom->GetPlane(j).normal.z, curRoom->GetPlane(j).d);

      for (j=0; j<curRoom->GetNumPortals(); j++)
      {
         curPortal = curRoom->GetPortal(j);

         mprintf("   [Portal %d] (id %d)\n", j, curPortal->GetPortalID());
         mprintf("     Plane:  (%g %g %g) %g\n", curPortal->GetPlane().normal.x, curPortal->GetPlane().normal.y,
                                                   curPortal->GetPlane().normal.z, curPortal->GetPlane().d);
         mprintf("     Center: (%g %g %g)\n", curPortal->GetCenter().x, curPortal->GetCenter().y, 
                                              curPortal->GetCenter().z);
         mprintf("     %d to %d\n", curPortal->GetNearRoom()->GetRoomID(), curPortal->GetFarRoom()->GetRoomID());

         mprintf("     Edges:\n");
         for (k=0; k<curPortal->GetNumEdges(); k++)
         {
            mprintf("       [%d] (%g %g %g) %g\n", k, curPortal->GetEdgePlane(k).normal.x,
                                                        curPortal->GetEdgePlane(k).normal.y,
                                                        curPortal->GetEdgePlane(k).normal.z,
                                                        curPortal->GetEdgePlane(k).d);
         }
      }
   }
}


////////////////////////////////////////////////////////////////////////////////

void SpewBadRoomObjs()
{
   AutoAppIPtr_(TraitManager, pTraitMan);
   AutoAppIPtr_(ObjectSystem, pObjSys);

   ObjID defaultObj = pObjSys->GetObjectNamed("Default Room");
   if (defaultObj == OBJ_NULL)
   {
      mprintf("Unable to find Default Room!\n");
      return;
   }

   for (int i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      cRoom *pRoom = g_pRooms->GetRoom(i);

      if (pRoom)
      {
         ObjID roomObj = pRoom->GetObjID();
         BOOL  valid = FALSE;

         while (roomObj != OBJ_NULL)
         {
            if (roomObj == defaultObj)
            {
               valid = TRUE;
               break;
            }

            roomObj = pTraitMan->GetArchetype(roomObj);
         }

         if (!valid)
         {
            mprintf("Room %d (%s) not valid room obj\n", pRoom->GetRoomID(), ObjWarnName(pRoom->GetObjID()));
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void DrawRecRoom(cRoom *pRoom, ObjID objID, int depth, int color)
{
   int draw_room_depth = 2;
   config_get_int("show_room_depth", &draw_room_depth);

   // If we're not the full depth, draw all portal lines and recurse to
   // draw adjacent rooms
   if (depth < draw_room_depth)
   {
      mxs_vector line_start, line_end;
      Location start, end, hit;
         
      // Draw lines to all portal centers
      for (int i=0; i<pRoom->GetNumPortals(); i++)
      {
         cRoomPortal *pPortal = pRoom->GetPortal(i);
         cRoom *pOtherRoom = pRoom->GetPortalFarRoom(i);

         // Draw the far room
         int far_color;

         if (depth > 4)
            far_color = 0x00004000;
         else
            far_color = ((int)(255.0 * (1.0 - (0.15 * depth)))) << 8;
   
         DrawRecRoom(pOtherRoom, objID, depth + 1, far_color);

         // Draw the portal->center lines
 
         line_start = pRoom->GetCenterPt();
         line_end = pPortal->GetCenter();

         // Raycast to see if it's potentially a problem
         MakeHintedLocationFromVector(&start, &line_start, &ObjPosGet(objID)->loc);
         MakeLocationFromVector(&end, &line_end);
     
         if (!PortalRaycast(&start, &end, &hit, TRUE))
            r3_set_color(0x00FF0000);
         else
            r3_set_color(0x000000FF);

         draw_line(&line_start, &line_end);

         line_start = line_end;
         line_end = pOtherRoom->GetCenterPt();

         // Raycast to see if it's potentially a problem
         MakeHintedLocationFromVector(&start, &line_start, &ObjPosGet(objID)->loc);
         MakeLocationFromVector(&end, &line_end);
     
         if (!PortalRaycast(&start, &end, &hit, TRUE))
            r3_set_color(0x00FF0000);
         else
            r3_set_color(0x000000FF);

         draw_line(&line_start, &line_end);
      }
   }

   // Get room dimensions and facing
   mxs_vector size = pRoom->GetSize();
   mxs_vector pos = pRoom->GetCenterPt();

   mxs_vector bmax, bmin;
   mx_scale_vec(&bmax, &size, 1.0);
   mx_scale_vec(&bmin, &size, -1.0);

   mxs_matrix mat;
   mxs_angvec fac;
   mat.vec[0] = pRoom->GetPlane(0).normal;
   mat.vec[1] = pRoom->GetPlane(1).normal;
   mat.vec[2] = pRoom->GetPlane(2).normal;
   mx_mat2ang(&fac, &mat);

   // Set color
   r3_set_color(color);

   // Draw room
   draw_bbox(&pos, &fac, &bmin, &bmax);
}

///////////////////////////////////////

void DrawRoom(ObjID objID)
{
   if (!g_show_player_room)
      return;

   cRoom *pBaseRoom;

   if ((pBaseRoom = g_pRooms->GetObjRoom(objID)) == NULL)
   {
      Warning(("DrawRoom: obj %d not in room!\n", objID));
      return;
   }

   lgd3d_set_zcompare(FALSE);
   lgd3d_disable_palette();

   DrawRecRoom(pBaseRoom, objID, 0, 0x00FFFFFF);

   lgd3d_enable_palette();
   lgd3d_set_zcompare(TRUE);
}

///////////////////////////////////////////////////////////////////////////////

void SpewRoomObj(ObjID obj)
{
   const cRoom *room;

   room = g_pRooms->GetObjRoom(obj);
   if (room != NULL)
      mprintf(" [room %d (obj %d)]\n", room->GetRoomID(), room->GetObjID());
   else
      mprintf(" no room\n");

   room = g_pRooms->RoomFromPoint(ObjPosGet(obj)->loc.vec);
   if (room != NULL)
      mprintf("*[room %d (obj %d)]\n", room->GetRoomID(), room->GetObjID());
   else
      mprintf("*no room\n");

}

///////////////////////////////////////////////////////////////////////////////

void DoRoomTest2(ObjID objID);
void DoRoomTest(ObjID obj)
{
   if (!g_show_bad_rooms)
      return;

   DoRoomTest2(obj);
   return;

   int nTests = 1000;
   config_get_int("num_room_tests", &nTests);

   float range = 20.0;
   config_get_float("room_test_range", &range);

   int i;
   Location start, hit;
   start = ObjPosGet(obj)->loc;

   const cRoom *pLikelyRoom = g_pRooms->GetObjRoom(obj);
   if (pLikelyRoom == NULL)
      return;

   // Try to find more BadRoomPts
   for (i=0; i<nTests; i++)
   {
      // Pick a random direction
      mxs_vector dir;
      dir.x = ((float)(RandRange(1, 100) - 50)) / 100;
      dir.y = ((float)(RandRange(1, 100) - 50)) / 100;
      dir.z = ((float)(RandRange(1, 100) - 50)) / 100;
      mx_normeq_vec(&dir);

      // Raycast
      PortalRaycastVector(&start, &dir, &hit, FALSE);

      // Take some random amount along the raycast (50%-95% of the way along)
      mxs_vector hit_vec;
      mxs_vector real_hit;

      float amt = ((float)RandRange(50, 95)) / 100.0;

      mx_sub_vec(&hit_vec, &hit.vec, &start.vec);
      mx_scaleeq_vec(&hit_vec, amt);
      mx_add_vec(&real_hit, &start.vec, &hit_vec);

      if (!pLikelyRoom->PointInside(real_hit) && (g_pRooms->RoomFromPoint(real_hit) == NULL))
      {
         if (gBadRoomPts.Size() >= 1000)
            gBadRoomPts.FastDeleteItem(0);

         gBadRoomPts.Append(real_hit);
      }
   }

   // Remove any BadRoomPts that are out of range
   for (i=0; i<gBadRoomPts.Size(); i++)
   {    
      if (mx_dist2_vec(&start.vec, &gBadRoomPts[i]) > (range * range))
      {
         gBadRoomPts.FastDeleteItem(i);
         i--;
      }
   }

   // And draw the rest
   lgd3d_set_zcompare(FALSE);
   lgd3d_disable_palette();

   r3_set_color(0x00FF00FF);

   mxs_vector line_start, line_end;

   for (i=0; i<gBadRoomPts.Size(); i++)
   {
      line_start = line_end = gBadRoomPts[i];
      line_start.x += 0.1;
      line_end.x   -= 0.1;

      draw_line(&line_start, &line_end);

      line_start = line_end = gBadRoomPts[i];
      line_start.y += 0.1;
      line_end.y   -= 0.1;

      draw_line(&line_start, &line_end);

      line_start = line_end = gBadRoomPts[i];
      line_start.z += 0.1;
      line_end.z   -= 0.1;

      draw_line(&line_start, &line_end);
   }

   lgd3d_enable_palette();
   lgd3d_set_zcompare(TRUE);
}

////////////////////////////////////////////////////////////////////////////////

typedef mxs_vector vec3f;

#define ONE_EPS   (0.995)
#define ZERO_EPS  (0.005)

#define mx_set   mx_mk_vec

// Compute polygon using Newell's method (see Graphics Gems)
vec3f PolygonNormal(int n, vec3f *vert)
{
   float oneEps = ONE_EPS;
   float zeroEps = ZERO_EPS;
   vec3f norm = { 0,0,0 };
   int i,j;

   for (i=0, j=n-1; i < n; j=i++) {
      norm.x += (vert[i].y-vert[j].y)*(vert[i].z+vert[j].z);
      norm.y += (vert[i].z-vert[j].z)*(vert[i].x+vert[j].x);
      norm.z += (vert[i].x-vert[j].x)*(vert[i].y+vert[j].y);
   }
   mx_normeq_vec(&norm);

        if (norm.x >=  oneEps) mx_set(&norm, 1, 0, 0);
   else if (norm.x <= -oneEps) mx_set(&norm,-1, 0, 0);
        if (norm.y >=  oneEps) mx_set(&norm, 0, 1, 0);
   else if (norm.y <= -oneEps) mx_set(&norm, 0,-1, 0);
        if (norm.z >=  oneEps) mx_set(&norm, 0, 0, 1);
   else if (norm.z <= -oneEps) mx_set(&norm, 0, 0,-1);

   if (fabs(norm.x) <= zeroEps) norm.x = 0;
   if (fabs(norm.y) <= zeroEps) norm.y = 0;
   if (fabs(norm.z) <= zeroEps) norm.z = 0;

   return norm;
}

vec3f *PolyAlloc(int n)
{
   return (vec3f *) malloc(sizeof(vec3f) * n);
}

void PolyFree(vec3f *p)
{
   free(p);
}

typedef mxs_plane vec4f;

static float DistancePointFromPlane(vec4f *plane,  vec3f *pt)
{
   return pt->x*plane->x + pt->y*plane->y + pt->z*plane->z + plane->d;
}

static int SideClassify(float dist)
{
   if (dist < -ZERO_EPS) return -1;
   if (dist >  ZERO_EPS) return  1;
   return 0;
}

#if 0
#define PairCrossesPlane(a,b)  ((a == -1 && b == 1) || (a == 1 && b == -1))
#else
#define PairCrossesPlane(a,b)  (((a)^(b)) == ((1)^(-1)))
#endif

#define MAX_STACK_DATA  64

tResult PolyClip(vec4f *plane,         bool keep_on,
                 int  src_n    ,       vec3f  *src_vert,
                 int *inside_n ,       vec3f **inside_vert,
                 int *outside_n,       vec3f **outside_vert)
{
   float point_dist_data[MAX_STACK_DATA];
   int point_side_data[MAX_STACK_DATA];
   float *point_dist;
   int *point_side;
   int inside_count=0, outside_count=0, on_count=0;
   int last_side, this_side;

   if (src_n < 3) return E_FAIL;

   // Allocate room to store per-vertex data; store on the stack if it's small enough
   
   if (src_n > MAX_STACK_DATA) {
      point_dist = (float *) malloc(sizeof(float) * src_n);
      point_side = (int *) malloc(sizeof(int) * src_n);
   } else {
      point_dist = point_dist_data;
      point_side = point_side_data;
   }

   // Determine the distance of each point from the plane, and classify which side it's on

   int i;
   for (i=0; i < src_n; ++i) {
      point_dist[i] = DistancePointFromPlane(plane, &src_vert[i]);
      point_side[i] = SideClassify(point_dist[i]);
   }

   // Compute the number of points on each side of the plane

   last_side = point_side[src_n-1];

   for (i=0; i < src_n; i++) {
      this_side = point_side[i];
      // if we crossed sides, we'll add a point in between
      if (PairCrossesPlane(this_side, last_side)) {
         ++on_count;
      }
      if (this_side > 0) ++inside_count;
      else if (this_side < 0) ++outside_count;
      else ++on_count;

      last_side = this_side;
   }

   // If the polygon lies along the plane and keep_on is requested,
   // determine which side it lies on, and cheat all the data so we
   // don't have to special case copying the vertices. @TODO: This
   // cheating is too clever and should be rewritten to do it the
   // hard way for clarity.

   if (on_count == src_n && keep_on) {
      // determine which way the polygon is facing
      vec3f normal = PolygonNormal(src_n, src_vert);
      float facing = mx_dot_vec(&normal, (vec3f *) &plane);
//      Assert(fabs(facing) > 0.5);
      on_count = 0;
      if (facing > 0.5) {
         inside_count = src_n;
         facing = 1;
      } else {
         outside_count = src_n;
         facing = -1;
      }
      for (i=0; i < src_n; ++i)
         point_dist[i] = point_side[i] = facing;
   }

   // Now that we know the number of points on each side, allocate
   // arrays that are exactly the right size.

   bool do_in=FALSE;
   if (inside_n != NULL && inside_vert != NULL) {
      if (inside_count > 0) {
         *inside_n = inside_count + on_count;
         *inside_vert = PolyAlloc(*inside_n);
         do_in = TRUE;
      } else {
         *inside_n = 0;
         *inside_vert = NULL;
      }
   }

   bool do_out=FALSE;
   if (outside_n != NULL && outside_vert != NULL) {
      if (outside_count > 0) {
         *outside_n = outside_count + on_count;
         *outside_vert = PolyAlloc(*outside_n);
         do_out = TRUE;
      } else {
         *outside_n = 0;
         *outside_vert = NULL;
      }
   }

   // Iterate through the edges, generating the vertices for each side

   int j,in=0,out=0;

   for (j=src_n-1, i=0; i < src_n; j=i++) {
      if (PairCrossesPlane(point_side[i], point_side[j])) {
         float dist = point_dist[i] - point_dist[j];
         vec3f new_pt;
         mx_interpolate_vec(&new_pt, &src_vert[j], &src_vert[i], (-point_dist[j])/dist);
         if (do_in ) { (* inside_vert)[ in++] = new_pt; }
         if (do_out) { (*outside_vert)[out++] = new_pt; }
      }
      if (do_in  && point_side[i] >= 0) { (* inside_vert)[ in++] = src_vert[i]; }
      if (do_out && point_side[i] <= 0) { (*outside_vert)[out++] = src_vert[i]; }
   }

//   if (do_in) Assert(in == *inside_n);
//   if (do_out) Assert(out == *outside_n);

   // Free up temporary storage

   if (src_n > MAX_STACK_DATA) {
      free(point_dist);
      free(point_side);
   }

   return S_OK;
}

tResult PolyComputePlane(int num_vert, vec3f *verts, vec4f *plane)
{
   vec3f normal;
   normal = PolygonNormal(num_vert, verts);
   if (num_vert < 3) return E_FAIL;
   plane->x = normal.x;
   plane->y = normal.y;
   plane->z = normal.z;
   plane->d = -mx_dot_vec(&normal, &verts[0]);
   return S_OK;
}

///////////////////////////////////////
//
// Render the cdb opaquely
//

void DrawClippedPoly(cRoom *pRoom, int num_vert, vec3f *verts, int cell, int face)
{
   if (cell >= pRoom->GetNumPortals()) {
#if 0
      int i,j;
      for (i=num_vert-1,j=0; j < num_vert; i=j++)
         draw_line(verts+i,verts+j);
#else
      draw_poly(num_vert, verts);
#endif
   } else {
      cRoom *pOtherRoom = pRoom->GetPortalFarRoom(cell);
      if (face == 6) {
         // this fragment was entirely inside some cell, so it's a portal boundary, so don't draw anything
//         r3_set_color(((rand()*rand()) & 0x330033)+0xCC00CC);
//         draw_poly(num_vert, verts);
         return;
      }

      vec4f plane = *(vec4f *) &pOtherRoom->GetPlane(face);
      int inside_n, outside_n;
      vec3f *inside_verts, *outside_verts;
      plane.x = -plane.x;
      plane.y = -plane.y;
      plane.z = -plane.z;
      plane.d = -plane.d;

      // clip this polygon by the face of this cell
      //  TRUE == keep around polygons that lie along the plane... we should never throw them away!
      if (E_FAIL == PolyClip(&plane, TRUE, num_vert, verts, &inside_n, &inside_verts, &outside_n, &outside_verts))
//      if (E_FAIL == PolyClip(&plane, TRUE, num_vert, verts, &outside_n, &outside_verts, &inside_n, &inside_verts))
         return;

      // If the fragment fell outside this face, go on to next cell
      if (outside_n != 0) {
         DrawClippedPoly(pRoom, outside_n, outside_verts, cell+1, 0);
         PolyFree(outside_verts);
      }

      // If the fragment fell inside the face, go on to next face
      if (inside_n != 0) {
         DrawClippedPoly(pRoom, inside_n, inside_verts, cell, face+1);
         PolyFree(inside_verts);
      }
   }
}

void DrawRoomPoly(cRoom *pRoom, mxs_vector *verts, int p0, int p1, int p2, int p3, int w)
{
   mxs_vector vert[4];
   vert[0] = verts[p3];
   vert[1] = verts[p2];
   vert[2] = verts[p1];
   vert[3] = verts[p0];
   int r = ((int) pRoom + w*10) & 63;
   r3_set_color(0xC000C0+r+r*65536);
   DrawClippedPoly(pRoom, 4, vert, 0, 0);
}

#define GROW_SIZE 0.03

void DrawRoomBoundaries(cRoom *pRoom)
{
   static int el[3][8] = { { 0,1,0,1,0,1,0,1 },
                           { 0,0,1,1,0,0,1,1 },
                           { 0,0,0,0,1,1,1,1 } };
   // generate polygonal representation of room so we can draw polygons

   // Get room dimensions and facing
   mxs_vector size = pRoom->GetSize();
   size.x += GROW_SIZE;
   size.y += GROW_SIZE;
   size.z += GROW_SIZE;

   mxs_matrix M;
   mxs_vector offset = pRoom->GetCenterPt();

   M.vec[0] = pRoom->GetPlane(0).normal;
   M.vec[1] = pRoom->GetPlane(1).normal;
   M.vec[2] = pRoom->GetPlane(2).normal;

   mxs_vector vlist[8];
   for (int i=0; i<8; i++)
   {
      mxs_vector v;
      int x_sign = el[0][i]*2-1;
      int y_sign = el[1][i]*2-1;
      int z_sign = el[2][i]*2-1;

      // Generate object-space vertex locations
      mx_mk_vec(&v, size.x * x_sign, 
                    size.y * y_sign, 
                    size.z * z_sign);

      // Rotate into world space
      mx_mat_mul_vec(&vlist[i], &M, &v);

      // Translate into world space
      mx_addeq_vec(&vlist[i], &offset);
   }

   // Draw room
   DrawRoomPoly(pRoom, vlist, 0,1,3,2, 0);
   DrawRoomPoly(pRoom, vlist, 4,6,7,5, 1);
   DrawRoomPoly(pRoom, vlist, 1,0,4,5, 2);
   DrawRoomPoly(pRoom, vlist, 3,7,6,2, 3);
   DrawRoomPoly(pRoom, vlist, 3,1,5,7, 4);
   DrawRoomPoly(pRoom, vlist, 2,6,4,0, 5);
}

void DrawRecRoom2(cRoom *pRoom, int depth)
{
   DrawRoomBoundaries(pRoom);
   if (depth)
   {
      // Visit all adjacent rooms
      for (int i=0; i<pRoom->GetNumPortals(); i++)
      {
         cRoom *pOtherRoom = pRoom->GetPortalFarRoom(i);
         DrawRecRoom2(pOtherRoom, depth-1);
      }
   }
}

void DoRoomTest2(ObjID objID)
{
   int draw_room_depth = 2;
   config_get_int("show_room_depth", &draw_room_depth);

   cRoom *pBaseRoom;

   if ((pBaseRoom = g_pRooms->GetObjRoom(objID)) == NULL)
   {
      Warning(("DrawRoom2: obj %d not in room!\n", objID));
      return;
   }

   lgd3d_disable_palette();
   //lgd3d_set_zcompare(FALSE);
   r3_set_color(0xFF00FF);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_UNLIT | R3_PL_SOLID);
   DrawRecRoom2(pBaseRoom, draw_room_depth);
   lgd3d_enable_palette();
   //lgd3d_set_zcompare(TRUE);
}

