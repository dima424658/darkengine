/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkamap.cpp,v 1.24 2000/03/05 17:50:23 patmac Exp $

#include <drkamap.h>
#include <autoprop.h>
#include <propert_.h>
#include <dataops_.h>
#include <keydefs.h>

#include <sdesbase.h>
#include <sdestool.h>

#include <room.h>
#include <rooms.h>
#include <playrobj.h>

#include <config.h>

#include <command.h>
#include <ctype.h>

#include <dev2d.h>
#include <drkmiss.h>
#include <resapi.h>
#include <respaths.h>
#include <drkuires.h>
#include <drkdebrf.h>
#include <drkpanl.h>
#include <questapi.h>
#include <gcompose.h>
#include <filevar.h>
#include <palmgr.h>
#include <mprintf.h>
#include <fonrstyp.h>

//
// INCLUDE THESE LAST
// 
#include <dbmem.h>
#include <initguid.h>
#include <amapguid.h>

////////////////////////////////////////////////////////////
//
// Automap Annotation class
//
#define MAX_ANNOT_CHARS 128

typedef struct {
   int               x;
   int               y;
   int               page;
   char              text[ MAX_ANNOT_CHARS ];
} sAmapAnnotation;

typedef cDynArray<sAmapAnnotation> cAmapAnnotationArray;

static cAmapAnnotationArray    gAnnotations;

////////////////////////////////////////////////////////////


static IAutomapProperty* gAutoMapProp = NULL; 

// We allow up to 8 map pages, and up to 32 decals per page

#define MAX_AMAP_PAGES 8
// MAX_AMAP_DECALS_PER_PAGE should be a multiple of 32
#define MAX_AMAP_DECALS_PER_PAGE 64
#define AMAP_LONGS_PER_PAGE (MAX_AMAP_DECALS_PER_PAGE >> 5)
#define MAX_MISSION_MAPS 32

struct sVisited
{
   ulong visited[MAX_MISSION_MAPS][MAX_AMAP_PAGES][AMAP_LONGS_PER_PAGE];
};

// Here's my descriptor, which identifies my stuff to the tag file & editor
sFileVarDesc gVisitedDesc = 
{
   kCampaignVar,              // Where do I get saved?
   "VISITED",                // Tag file tag
   "Rooms Visited",          // friendly name
   FILEVAR_TYPE(sVisited),   // Type (for editing)
   { 1, 0},                  // version
   { 1, 0},                  // last valid version 
   "dark",                   // optional: what game am I in NULL means all 
};

// The actual global variable
cFileVar<sVisited,&gVisitedDesc> gVisited; 


// -1 - display all decals as if they have been visited
// 0 - normal
// 1 - highlight all decals as if they are current player pos
static int gImEverywhere = 0;
#define AMAP_IM_EVERYWHERE 1
#define AMAP_IVE_BEEN_EVERYWHERE -1

static BOOL gbAutomapSpew = FALSE;

// map revealed source mission

//
// SDESC
//

static sFieldDesc mapsource_fields[] =
{
  { "Source Mission", kFieldTypeInt, FieldLocation(sMapSourceData,sourcemiss) },
  { "Compass Offset", kFieldTypeFloat, FieldLocation(sMapSourceData,compassdiff) },
};

static sStructDesc mapsource_sdesc = StructDescBuild(sMapSourceData,kStructFlagNone,mapsource_fields);

//
// VAR DESCRIPTOR
//

sFileVarDesc gAutomapInfoDesc =
{
  kMissionVar,              //where do I get saved?
  "MAPISRC",                //tag file tag
  "Automap Info",           //friendly name
  FILEVAR_TYPE(sMapSourceData), //type (for editing)
  { 1, 0},                     //verion
  { 1, 0},                     // last valid version
  "dark"
};

//
// Actually use the defaults when resetting
//

class cMapSourceData : public cFileVar<sMapSourceData,&gAutomapInfoDesc>
{
};

// 
// the variable itself
//

static cMapSourceData gMapSourceData;

int GetMapSourceNum()
{
  return gMapSourceData.sourcemiss;
}

float GetCompassOffset()
{
  return gMapSourceData.compassdiff;
}

void TransferMapInfo()
{
  int prevmiss = gMapSourceData.sourcemiss;
  const sMissionData* missdata = GetMissionData();
  Assert_((missdata->num < MAX_MISSION_MAPS) && (missdata->num >= 0));
  Assert_((prevmiss < MAX_MISSION_MAPS));
  int curmiss = missdata->num;

  if ((curmiss==prevmiss) || (prevmiss<=0))  //don't copy in this case.
    return;

  //sigh, memcpy goes the opposite way of what I thought... copies to curmiss now.
  memcpy(&gVisited.visited[curmiss],&gVisited.visited[prevmiss],sizeof(gVisited.visited[curmiss]));
}

void LoadMapSourceInfo(ITagFile* file)
{
  gMapSourceData.DatabaseMsg(kDatabaseLoad|kDBMission,file);
}

void SaveMapSourceInfo(ITagFile* file)
{
  gMapSourceData.DatabaseMsg(kDatabaseSave|kDBMission,file);
}

void MapSourceInfoInit()
{
  AutoAppIPtr_(StructDescTools,pTools);
  pTools->Register(&mapsource_sdesc);
}

void MapSourceInfoTerm()
{
}

////////////////////////////////////////////////////////////
//
// Automap Property
//

typedef cGenericProperty<IAutomapProperty,&IID_IAutomapProperty, sAutomapProperty*> cAutomapPropertyBase;

class cAutomapProperty : public cAutomapPropertyBase
{
   cClassDataOps< sAutomapProperty> mOps; 

public: 
   cAutomapProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cAutomapPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cAutomapProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cAutomapPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE( sAutomapProperty);
};



// Description
static sPropertyDesc AutomapProp_desc = 
{
   PROP_AUTOMAP_NAME,    // Name
   0,    // Flags
   NULL,
   2,                    // Version
   0,                    // Last ok version (0 means none)
   { "Room", "Automap" },
};

void SetupAutomapProperty(void);

void AutomapPropInit(void)
{
   SetupAutomapProperty();
   gAutoMapProp = new cAutomapProperty(&AutomapProp_desc, kPropertyImplDense);
}

void AutomapPropTerm(void)
{
   SafeRelease(gAutoMapProp);
}

// Structure Descriptor
static sFieldDesc AutomapFields[] = 
{
   { "Page",  kFieldTypeInt,  FieldLocation(sAutomapProperty, page)  },
   { "Location", kFieldTypeInt,  FieldLocation(sAutomapProperty, location) },
};

static sStructDesc AutomapDesc = StructDescBuild(sAutomapProperty, kStructFlagNone, AutomapFields);

static void SetupAutomapProperty(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&AutomapDesc);
}


typedef cDynClassArray<cStr> cStringArray; 

////////////////////////////////////////

static sAutomapProperty* map_player_pos()
{

   cRoom* player_room = g_pRooms->GetObjRoom(PlayerObject()); 
   if (player_room) 
   {
      ObjID room = player_room->GetObjID(); 

      sAutomapProperty* prop; 
      if (room != OBJ_NULL && gAutoMapProp->Get(room,&prop))
      {
         AutoAppIPtr(QuestData); 
         if (prop->page >= pQuestData->Get(MIN_PAGE_QVAR) 
             && prop->page <= pQuestData->Get(MAX_PAGE_QVAR))
            return prop; 

         static sAutomapProperty oob_pos; 
   
         oob_pos.page = pQuestData->Get(OOB_PAGE_QVAR); 
         oob_pos.location = pQuestData->Get(OOB_LOC_QVAR); 
         return &oob_pos; 
      }
   }

   return NULL;
}


//
// tell if the player has visited the specified page & location
//
BOOL
DarkAutomapGetLocationVisited( int page, int location )
{
   const sMissionData* missdata = GetMissionData();
   Assert_((missdata->num < MAX_MISSION_MAPS) && (missdata->num >= 0));
   // page should always be valid, location may be -1
   assert( (page >= 0) && (page < MAX_AMAP_PAGES) );
   if ( (location >= 0) && (location < MAX_AMAP_DECALS_PER_PAGE) ) {
      // tell if the player has visited the room
      return (gVisited.visited[missdata->num][page][location>>5] & (1 << (location & 31))) != 0;
   } else {
      return FALSE;
   }
}


//
// set the visited flag for the specified page & location
//
void
DarkAutomapSetLocationVisited( int page, int location )
{
   AutoAppIPtr(QuestData); 
   int locsVisited, p, l;
   const sMissionData* missdata = GetMissionData();
   Assert_((missdata->num < MAX_MISSION_MAPS) && (missdata->num >= 0));

   // page should always be valid, location may be -1
   assert( (page >= 0) && (page < MAX_AMAP_PAGES) );
   if ( (location >= 0) && (location < MAX_AMAP_DECALS_PER_PAGE) ) {
      // tell the automap the player has visited the room
      gVisited.visited[missdata->num][page][location>>5] |= (1 << (location & 31));
   }

   // set the quest variable that tracks total number of locations visited
   locsVisited = 0;
   for ( p = 0; p < MAX_AMAP_PAGES; p++ ) {
      for ( l = 0; l < MAX_AMAP_DECALS_PER_PAGE; l++ ) {
         if ( gVisited.visited[missdata->num][p][l>>5] & (1 << (l & 31)) ) {
            locsVisited++;
         }
      }
   }
   if ( pQuestData->Exists( LOCS_VISITED_QVAR ) ) {
      pQuestData->Set( LOCS_VISITED_QVAR, locsVisited );
   } else {
      pQuestData->Create( LOCS_VISITED_QVAR, locsVisited, kQuestDataMission );
   }
#ifndef SHIP
   if ( gbAutomapSpew ) {
      mprintf( "Automap visited page %d, location %d.  Total visited: %d\n",
               page, location, locsVisited );
   }
#endif
}

//------------------------------------------------------------
// flag that an area has been visited
//
static BOOL
DarkAutomapRoomCB( ObjID roomEntered )
{

   // get the decal index & map page properties of the room
   sAutomapProperty* prop = map_player_pos();
   if ( prop ) {
      DarkAutomapSetLocationVisited( prop->page, prop->location );
   }
   return TRUE;
}


void
DarkAutomapFirstFrameInit( void )
{
   cRoom* player_room = g_pRooms->GetObjRoom(PlayerObject()); 
   if (player_room) {
      DarkAutomapRoomCB( player_room->GetObjID() );
   }
}


//------------------------------------------------------------
// Automap button panel
//

enum mapMode { kMapModeNormal, kMapModeAddText, kMapModeEraseText };

class cAutomap: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cAutomap(): cDarkPanel(&gDesc),mpQuestData(AppGetObj(IQuestData)) {}; 
   ~cAutomap() { SafeRelease(mpQuestData); }; 

   enum eRects
   {
      kGoals,
      kDone,
      kStringButts, 

      kNext = kStringButts, 
      kPrev, 
      kNumButts, 
      kMap = kNumButts,

      kNumRects
   }; 

   bool MapRegionEventHandler( uiEvent *pEvent, Region *pReg, void *pState );


protected:
   void RedrawDisplay(); 
   void OnButtonList(ushort action, int button); 
   void OnLoopMsg(eLoopMessage, tLoopMessageData ); 
   void InitUI(); 
   void TermUI(); 

   void RedrawButtons(); 

   static mapMode mMapMode;
   int mPageNum; 
   enum { kNumArrowImages = 4 }; 
   IImageSource* mArrows[kNumArrowImages]; // 4 arrow images 

   IQuestData* mpQuestData; 

   Region      mMapRegion;
   int         mMapRegionEventHandlerID;
   sAmapAnnotation         mCurAnnotation;
};

////////////////////////////////////////

mapMode cAutomap::mMapMode;

static bool
regionCB( uiEvent  *pEvent,
           Region   *pReg,
           void     *pState )
{
   cAutomap    *pAmap = (cAutomap *) pState;

   return pAmap->MapRegionEventHandler( pEvent, pReg, pState );
}


void cAutomap::InitUI()
{
   cDarkPanel::InitUI();   
   int i; 
   Rect& mapr = mRects[(int)kMap]; 

   for (i = 0; i < kNumArrowImages; i++)
   {
      char buf[16]; 
      sprintf(buf,"arrow%03d",i); 
      mArrows[i] = FetchUIImage(buf); 
   }

   int n = 0; 
   for (i = kNext; i < kNumButts; i++)
   {
      DrawElement& elem = mElems[i]; 
      memset(&elem,0,sizeof(elem));
      elem.draw_type = DRAWTYPE_BITMAP; 
      elem.draw_data  = mArrows[n++]->Lock(); 
      elem.draw_data2 = mArrows[n++]->Lock(); 
      elem.draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 
   }

   // create region & handler for map window - will be used
   //  to do annotations
   region_create( LGadBoxRegion(LGadCurrentRoot()),
                  &mMapRegion,
                  &(mRects[ (int)kMap]),
                  0, 0, 0, NULL, NULL, NULL, NULL );
   uiInstallRegionHandler( &mMapRegion, ALL_EVENTS, regionCB, this,
                           &mMapRegionEventHandlerID );

   // initialize annotation stuff
   mMapMode = kMapModeNormal;
   mCurAnnotation.text[0] = 0;

   // TBD - init annotations from ???
}

void cAutomap::TermUI()
{
   int i;

   for (i = 0; i < kNumArrowImages; i++)
   {
      mArrows[i]->Unlock(); 
      SafeRelease(mArrows[i]); 
   }
   uiRemoveRegionHandler( &mMapRegion, mMapRegionEventHandlerID );

   if ( mMapMode == kMapModeAddText ) {
      // add annotation-in-progress if not 0 length
      if ( strlen( mCurAnnotation.text ) ) {
         gAnnotations.Append( mCurAnnotation );
      }
   }
   mMapMode = kMapModeNormal;
   // TBD: save annotations to ???
   for ( i = 0; i < gAnnotations.Size(); i++ ) {
   }
   // TBD: free any annotation stuff

   cDarkPanel::TermUI(); 
}

////////////////////////////////////////
//
// DrawAnnotation & OverlapsAnnotation support multi-line auto-centered annotations
//

#define EXTRA_VERTICAL_SPACING 0

static void
DrawAnnotation( sAmapAnnotation  *pAnnotation,
                BOOL             bWithCursor )
{
   int x, y, endX, endY;
   short w, h;
   char txtCopy[ MAX_ANNOT_CHARS ];
   char *pToken;
   BOOL bLastLineEmpty;

   strcpy( txtCopy, pAnnotation->text );
   bLastLineEmpty = (txtCopy[strlen(txtCopy) - 1] == '\n');
   pToken = strtok( txtCopy, "\n" );
   if ( pToken == NULL ) {
      // if we want cursor for empty string, we at least need to know height of font
      if ( bWithCursor ) {
         gr_string_size( "A", &w, &h );
         endX = pAnnotation->x;
         endY = pAnnotation->y;
      }
   } else {
      y = pAnnotation->y;
      while ( pToken ) {
         gr_string_size( pToken, &w, &h );
         x = pAnnotation->x - (w/2);
         gr_string( pToken, x, y );
         endX = x + w;
         endY = y;
         y += (h + EXTRA_VERTICAL_SPACING);
         pToken = strtok( NULL, "\n" );
      }
   }
   if ( bLastLineEmpty ) {
      // this is here so that after the user hits return, the cursor
      //  will appear on the new line, not at the end of the last line
      // this is due to strtok refusing to return an empty token
      endX = pAnnotation->x;
      endY += h;
   }
   if ( bWithCursor ) {
      gr_int_line( endX, endY, endX, endY + h );
   }
}


static BOOL
OverlapsAnnotation( sAmapAnnotation    *pAnnotation,
                    int                x,
                    int                y )
{
   short w, h;
   char txtCopy[ MAX_ANNOT_CHARS ];
   char *pToken;
   int xLeft, yTop, xRight, yBottom;

   strcpy( txtCopy, pAnnotation->text );
   pToken = strtok( txtCopy, "\n" );
   yTop = pAnnotation->y;
   while ( pToken ) {
      gr_string_size( pToken, &w, &h );
      xLeft = pAnnotation->x - (w / 2);
      xRight = xLeft + w;
      yBottom = yTop + h;
      if ( (y >= yTop) && (y < yBottom) &&
           (x >= xLeft) && (x < xRight) ) {
         // x,y is within string extents 
         return TRUE;
      }
      yTop += (h + EXTRA_VERTICAL_SPACING);
      pToken = strtok( NULL, "\n" );
   }

   return FALSE;
}

////////////////////////////////////////

// enhance contrast
// make values less than 128 smaller,
//   values greater than 128 bigger

static float brightenRGB[3] = { 0.7, 0.7, 0.3 };
static float darkenRGB[3] =   { 0.1, 0.1, 0.1 };
static float dimRGB[3] =      { 0.2, 0.2, 0.7 };
static int splitPoint = 100;

static void
mungeColor( uchar    *pDst,
            uchar    *pSrc,
            BOOL     bYouAreHere )
{
   int r, g, b;

   r = *pSrc++;
   g = *pSrc++;
   b = *pSrc++;

   if ( bYouAreHere ) {

      // enhance contrast in decal where player is
      if ( ((r + g + b) / 3) >= splitPoint ) {
         // make color brighter
         r = 255 - ((255 - r) * (1.0 - brightenRGB[0]));
         g = 255 - ((255 - g) * (1.0 - brightenRGB[1]));
         b = 255 - ((255 - b) * (1.0 - brightenRGB[2]));
      } else {
         // make color darker
         r *= (1.0 - darkenRGB[0]);
         g *= (1.0 - darkenRGB[1]);
         b *= (1.0 - darkenRGB[2]);
      }

   } else {

      // this decal is where player has visited
      // reduce the contrast of the red and green components, but
      //  increase the contrast of the blue component.  This color
      //  shifts the palette toward blue, to make the visited areas
      //  stand out, without losing so much contrast text is illegible
      r = (r >= splitPoint) ? splitPoint + ( (r - splitPoint) * (1.0 - dimRGB[0]) )
         :                    splitPoint - ( (splitPoint - r) * (1.0 - dimRGB[0]) );
      g = (g >= splitPoint) ? splitPoint + ( (g - splitPoint) * (1.0 - dimRGB[1]) )
         :                    splitPoint - ( (splitPoint - g) * (1.0 - dimRGB[1]) );
      b = (b >= splitPoint) ? 255 - ((255 - b) * (1.0 - dimRGB[2]))
         :                    b * (1.0 - dimRGB[2]);
   }

   *pDst++ = r;
   *pDst++ = g;
   *pDst++ = b;
}


static void
DrawDecal( IImageSource    *pDecal,
           Rect            *pRect,
           BOOL            bYouAreHere )
{
   int i, oldPalSlot;
   uchar *pNewPal, *pSrc, *pDst;

   grs_bitmap *pBM = (grs_bitmap *) (pDecal->Lock());

   // muck with pallette then display
   oldPalSlot = pBM->align;
   assert( pBM->w == pBM->row );

   pNewPal = (uchar *) Malloc( 256 * 3 );

   pSrc = palmgr_get_pal( oldPalSlot );
   pDst = pNewPal;
   for ( i = 0; i < 256; i++ ) {
      mungeColor( pDst, pSrc, bYouAreHere );
      pSrc += 3;
      pDst += 3;
   }
   pBM->align = palmgr_alloc_pal( pNewPal );
   gr_bitmap( pBM, pRect->ul.x, pRect->ul.y );
   palmgr_release_slot( pBM->align );
   pBM->align = oldPalSlot;

   Free( pNewPal );

  pDecal->Unlock();
  SafeRelease( pDecal );
}


////////////////////////////////////////

void cAutomap::RedrawDisplay()
{
   RedrawButtons(); 
   const sMissionData* missdata = GetMissionData(); 
   char buf[64]; 
   sprintf(buf,"%s/page%03d",missdata->path,mPageNum); 
   IImageSource* mapsrc = FetchUIImage(buf); 
   cRectArray tmpRects; 
   int i;
   BOOL bIAmEverywhere = FALSE;
   IRes* pFontRes = NULL; 
   AutoAppIPtr(ResMan);

   //to prevent (no)map crash -- AMSD
   if (mapsrc == NULL)
     return;
   grs_bitmap* map = (grs_bitmap*)mapsrc->Lock(); 
   int numDecals, decal;
   ulong *pVisitedDecals;

   Rect& mapr = mRects[(int)kMap]; 

   GUIcompose c;
   GUIsetup(&c,&mapr,ComposeFlagRead,GUI_CANV_ANY); 
   
   gr_bitmap(map,0,0); 

   Assert_((missdata->num < MAX_MISSION_MAPS) && (missdata->num >= 0));
   sAutomapProperty* prop = map_player_pos(); 

   // hack - for some reason, the location the player has just entered is sometimes
   //  not marked as visited - here force mark that location as visited
   if ( prop ) {
      DarkAutomapSetLocationVisited( prop->page, prop->location );
   }

   pVisitedDecals = &(gVisited.visited[missdata->num][mPageNum][0]);
   sprintf(buf,"%s/p%03dra",missdata->path,mPageNum); 
   FetchUIRects(buf, tmpRects);
   numDecals = tmpRects.Size();

   //////////////////////////////////////////////////////////////////////////
   //
   // draw all the decals for areas the player has visited, EXCEPT for the
   //  decal for the current player position
   //
   //////////////////////////////////////////////////////////////////////////
   int youAreHere = -1;
   IImageSource* img;
   for ( decal = 0; decal < numDecals; decal++ ) {
      BOOL bYouAreHere = prop && (prop->page == mPageNum) && (prop->location == decal);
      // is decal at current player position
      if ( bYouAreHere && (gImEverywhere == 0) ) {

         // remember that we are really somewhere
         youAreHere = decal;

      } else {

         if ( (pVisitedDecals[decal>>5] & (1 << (decal & 31))) || (gImEverywhere != 0) ) {
            // Get the image
            sprintf(buf,"%s/p%03dr%03d",missdata->path,mPageNum,decal);

            img = FetchUIImage(buf);
            if ( img ) {
               DrawDecal( img, &(tmpRects[decal]), (gImEverywhere == AMAP_IM_EVERYWHERE) );
            }
         }

      }
   }

   //////////////////////////////////////////////////////////////////////////
   //
   // lastly, draw the decal where player is, so that it is always on top
   //
   //////////////////////////////////////////////////////////////////////////
   if ( youAreHere != -1 ) {
      // Get the image
      sprintf( buf, "%s/p%03dr%03d", missdata->path, mPageNum, youAreHere );

      img = FetchUIImage(buf);
      if ( img ) {
         DrawDecal( img, &(tmpRects[youAreHere]), TRUE );
      }
   }

   GUIdone(&c); 

   mapsrc->Unlock();
   SafeRelease(mapsrc); 

   //////////////////////////////////////////////////////////////////////////
   //
   // draw all annotations
   //
   //////////////////////////////////////////////////////////////////////////
   grs_clip    oldClip;

   // save old clip window, clip to map window
   gr_save_cliprect( &oldClip.i );
   gr_set_cliprect( mapr.ul.x, mapr.ul.y, mapr.lr.x, mapr.lr.y );

   // set annotation font
   strcpy( buf, "f_scrp12" );
   config_get_raw( "automap_note_font", buf, sizeof(buf)-1 );
   pFontRes = pResMan->Bind( buf, RESTYPE_FONT, NULL, mResPath );
   if ( pFontRes ) {
      grs_font* pFont = (grs_font*) pFontRes->Lock(); 
      gr_set_font( pFont );
   }

   gr_set_fcolor( 0 );
   for ( i = 0; i < gAnnotations.Size(); i++ ) {
      if ( gAnnotations[i].page == mPageNum ) {
         DrawAnnotation( &(gAnnotations[i]), FALSE );
      }
   }

   // draw the current annotation
   if ( mMapMode == kMapModeAddText ) {
      DrawAnnotation( &mCurAnnotation, TRUE );
   }

   if ( pFontRes ) {
      pFontRes->Unlock(); 
   }

   // restore old clip window
   gr_restore_cliprect( &oldClip.i );
}


////////////////////////////////////////

void cAutomap::RedrawButtons()
{
   if (mPageNum == mpQuestData->Get(MIN_PAGE_QVAR))
   {
      mElems[(int)kPrev].draw_type = DRAWTYPE_NONE; 
      GUIErase(&mRects[(int)kPrev]); 
   }
   else
   {
      mElems[(int)kPrev].draw_type = DRAWTYPE_BITMAP; 
      region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kPrev]);      
   }

   if (mPageNum == mpQuestData->Get(MAX_PAGE_QVAR))
   {
      mElems[(int)kNext].draw_type = DRAWTYPE_NONE; 
      GUIErase(&mRects[(int)kNext]); 
   }
   else
   {
      mElems[(int)kNext].draw_type = DRAWTYPE_BITMAP; 
      region_expose(LGadBoxRegion(LGadCurrentRoot()),&mRects[(int)kNext]);      
   }

}

////////////////////////////////////////

void cAutomap::OnButtonList(ushort action , int button)
{
   if (action & (BUTTONGADG_LCLICK|BUTTONGADG_RCLICK))
   {
      switch (button)
      {
         case kDone:
         {
            IPanelMode* pMode = GetPanelMode(); 
            pMode->Exit(); 
            SafeRelease(pMode); 
            break; 
         }
         case kGoals:
            SwitchToObjectivesMode(FALSE); 
            break; 

         case kNext:
         {
            int max = mpQuestData->Get(MAX_PAGE_QVAR); 
            if (mPageNum < max)
            {
               // if an annotation is in progress, complete it
               if ( strlen( mCurAnnotation.text ) ) {
                  gAnnotations.Append( mCurAnnotation );
               }
               mCurAnnotation.text[0] = 0;
               mMapMode = kMapModeNormal;
               mPageNum++; 
               RedrawDisplay(); 
            }
         }
         break; 

         case kPrev:
         {
            int min = mpQuestData->Get(MIN_PAGE_QVAR); 
            if (mPageNum > min)
            {
               // if an annotation is in progress, complete it
               if ( strlen( mCurAnnotation.text ) ) {
                  gAnnotations.Append( mCurAnnotation );
               }
               mCurAnnotation.text[0] = 0;
               mMapMode = kMapModeNormal;
               mPageNum--; 
               RedrawDisplay(); 
            }
         }
         break; 
      }
   }
}

////////////////////////////////////////

void cAutomap::OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
{
   switch (msg)
   {
      case kMsgEnterMode:
      {
         sAutomapProperty* pos = map_player_pos(); 

         if (pos)
            mPageNum = pos->page; 
         else // default to first page 
            mPageNum = mpQuestData->Get(MIN_PAGE_QVAR); 

      }
      break; 
   }
   cDarkPanel::OnLoopMsg(msg,data);
}


bool
cAutomap::MapRegionEventHandler( uiEvent  *pEvent,
                                 Region   * /*pReg*/,
                                 void     * /*pState*/ )
{
   uiCookedKeyEvent *pKeyEvent;
   uiMouseEvent *pMouseEvent;
   char c;
   bool retVal = FALSE;
   bool doRedraw = FALSE;
   int x, y, len, i;

   switch( pEvent->type ) {
      case UI_EVENT_KBD_COOKED:
         if ( (pEvent->subtype & KB_FLAG_DOWN) && (mMapMode == kMapModeAddText) ) {
            pKeyEvent = (uiCookedKeyEvent *) pEvent;
            c = kb2ascii( pKeyEvent->code );

            switch ( c ) {
#if 0
               case KEY_ENTER:
                  // TBD - user hit enter, annotation is complete
                  mMapMode = kMapModeNormal;
                  if ( strlen( mCurAnnotation.text ) ) {
                     gAnnotations.Append( mCurAnnotation );
                     mCurAnnotation.text[0] = 0;
                  }
                  break;
#endif
               case KEY_ESC:
                  // TBD - user hit escape, annotation is complete
				   mMapMode = kMapModeNormal;				
				   if ( strlen( mCurAnnotation.text ) ) {                     				
					   gAnnotations.Append( mCurAnnotation );                     				
					   mCurAnnotation.text[0] = 0;                					  
				   }
                  break;
               case KEY_BS:
               case KEY_DEL:
                  // delete a char
                  len = strlen( mCurAnnotation.text );
                  if ( len ) {
                     mCurAnnotation.text[ len - 1 ] = 0;
                  }
                  break;
               case KEY_ENTER:
                  c = '\n';
               default:
                  // 
                  // if key is in range, add it to end of string and redisplay
                  //
                  len = strlen( mCurAnnotation.text );
                  if ( len < (MAX_ANNOT_CHARS - 1) ) {
                     mCurAnnotation.text[ len ] = c;
                     mCurAnnotation.text[ len + 1 ] = 0;
                  }
                  break;
            }
            doRedraw = TRUE;
         } //end if KB_DOWN and ADD TEXT
		 else
			 if ( (pEvent->subtype & KB_FLAG_DOWN) && (mMapMode == kMapModeNormal) ){            
				 pKeyEvent = (uiCookedKeyEvent *) pEvent;            
				 c = kb2ascii( pKeyEvent->code );            			
				 switch ( c ) {
				   case KEY_ESC:                  
					   IPanelMode* pMode = GetPanelMode(); 					  					
					   pMode->Exit(); 					  					
					   SafeRelease(pMode); 				  
					 break;			
				 } //switch c
			 } //if KB_DOWN and NORMAL
         retVal = TRUE;
         break;

      case UI_EVENT_MOUSE:
         //
         // mouse click - 
         //
         pMouseEvent = (uiMouseEvent *) pEvent;
         x = pMouseEvent->pos.x;
         y = pMouseEvent->pos.y;         

         if ( pMouseEvent->action & MOUSE_LUP ) {
            // TBD - change into text entry mode
            switch( mMapMode ) {

               case kMapModeNormal:
                  // change into text entry mode
                  mMapMode = kMapModeAddText;
                  mCurAnnotation.text[0] = 0;
                  mCurAnnotation.x = x;
                  mCurAnnotation.y = y;
                  mCurAnnotation.page = mPageNum;
                  break;

               case kMapModeAddText:
                  // end current annotation, start new one
                  if ( strlen( mCurAnnotation.text ) ) {
                     gAnnotations.Append( mCurAnnotation );
                     mCurAnnotation.text[0] = 0;
                  }
                  mCurAnnotation.x = x;
                  mCurAnnotation.y = y;
                  mCurAnnotation.page = mPageNum;
                  break;

               case kMapModeEraseText:
                  // find annotation which was clicked on & delete it
                  grs_font* pFont = (grs_font*) mpFontRes->Lock();
                  for ( i = 0; i < gAnnotations.Size(); i++ ) {
                     // get string extents...
                     if ( (gAnnotations[i].page == mPageNum) &&
                          OverlapsAnnotation( &(gAnnotations[i]), x, y ) ) {
                        gAnnotations.DeleteItem( i );
                        break;
                     }
                  }
                  mpFontRes->Unlock(); 
                  mMapMode = kMapModeNormal;
                  break;
            }
            doRedraw = TRUE;
         }

         if ( pMouseEvent->action & MOUSE_RUP ) {
            // if there was an annotation in progress, finish it
            if ( strlen( mCurAnnotation.text ) ) {
               gAnnotations.Append( mCurAnnotation );
               mCurAnnotation.text[0] = 0;
            }
            // find annotation which was clicked on & delete it
            grs_font* pFont = (grs_font*) mpFontRes->Lock();
            for ( i = 0; i < gAnnotations.Size(); i++ ) {
               if ( (gAnnotations[i].page == mPageNum) &&
                    OverlapsAnnotation( &(gAnnotations[i]), x, y ) ) {
                  gAnnotations.DeleteItem( i );
                  break;
               }
            }
            mMapMode = kMapModeNormal;
            mpFontRes->Unlock(); 
            doRedraw = TRUE;
         }
         retVal = TRUE;
         break;
   }

   if ( doRedraw ) {
      uiHideMouse( LGadCurrentRoot()->box.r.r );
      RedrawDisplay();
      uiShowMouse( LGadCurrentRoot()->box.r.r );
   }
   return retVal;
}

////////////////////////////////////////

static const char* butt_names[] = 
{
   "goals",
   "done", 
};

sDarkPanelDesc cAutomap::gDesc = 
{
   "map",
   cAutomap::kNumButts,
   cAutomap::kNumRects,
   cAutomap::kStringButts, 
   butt_names, 
}; 

////////////////////////////////////////

static cAutomap* gpAutomap = NULL; 

void SwitchToDarkAutomapMode(BOOL push) 
{
   if (gpAutomap)
   {
      IPanelMode* panel = gpAutomap->GetPanelMode(); 

      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}


static void create_panel_mode()
{
   cAutomap* automap = new cAutomap; 
   
   gpAutomap = automap; 
   
}



void init_commands(void);

void DarkAutomapInit(void)
{
   AutomapPropInit();
   create_panel_mode(); 
   init_commands();
   cRooms::AddAutomapCallback( DarkAutomapRoomCB );
}


void DarkAutomapTerm()
{
   AutomapPropTerm(); 
   delete gpAutomap; 
   cRooms::RemoveAutomapCallback( DarkAutomapRoomCB );
}

//
//  load/save annotations stuff
//
static TagFileTag MyTag = { "AMAPANNO" };
static TagVersion MyVersion = { 0, 0 };

static BOOL setup_tagfile(ITagFile* file)
{
   if (file->OpenMode() == kTagOpenRead && file->BlockSize(&MyTag) == 0)
      return FALSE; 

   TagVersion v = MyVersion;
   HRESULT result = file->OpenBlock(&MyTag,&v);
   if (FAILED(result))
      return FALSE; 

   if (!VersionNumsEqual(&v,&MyVersion))
   {
      file->CloseBlock(); 
      return FALSE;
   }

   return TRUE; 
}


void DarkAutomapDatabaseNotify( ulong       msg,
                                ITagFile    *pFile )
{
   int numAnnotations, i;

   switch(DB_MSG(msg))
   {
      case kDatabaseReset:
         gAnnotations.SetSize( 0 );
         break; 

      case kDatabaseLoad:
         if ( setup_tagfile(pFile) ) {
            numAnnotations = 0;
            pFile->Read( (char *) (&numAnnotations), sizeof(numAnnotations) );
            gAnnotations.SetSize( numAnnotations );
            for ( i = 0; i < numAnnotations; i++ ) {
               pFile->Read( (char *) (&gAnnotations[i]), sizeof(sAmapAnnotation) );
            }
            pFile->CloseBlock();
         }
         break;

      case kDatabaseSave:
         if ( setup_tagfile(pFile) ) {
            numAnnotations = gAnnotations.Size();
            pFile->Write( (char *) (&numAnnotations), sizeof(numAnnotations) );
            for ( i = 0; i < numAnnotations; i++ ) {
               pFile->Write( (char *) (&gAnnotations[i]), sizeof(sAmapAnnotation) );
            }
            pFile->CloseBlock();
         }
         break;

   }
}


//
// Commands
//


static void do_automap()
{
   SwitchToDarkAutomapMode(TRUE); 
}

static void
automap_set_visited( int visitedDecal )
{
   sAutomapProperty *pPlayerPos;
   pPlayerPos = map_player_pos();

   if ( pPlayerPos && (pPlayerPos->page >= 0) && (pPlayerPos->page < MAX_AMAP_PAGES) ) {
      if ( visitedDecal == -1 ) {
         for ( visitedDecal = 0; visitedDecal < MAX_AMAP_DECALS_PER_PAGE; visitedDecal++ ) {
            DarkAutomapSetLocationVisited( pPlayerPos->page, visitedDecal );
         }
      } else {
         DarkAutomapSetLocationVisited( pPlayerPos->page, visitedDecal );
      }
   }
}


static void
automap_im_everywhere( int onOff )
{
   gImEverywhere = onOff;
}


static Command commands [] = 
{
   { "automap", FUNC_STRING, do_automap, "Display the automap", HK_ALL },
   { "amap_visited", FUNC_INT, automap_set_visited, "mark an automap location on current page visited", HK_ALL },
   { "amap_im_everywhere", FUNC_INT, automap_im_everywhere, "make all decals display as current location", HK_ALL },
   { "amap_spew", TOGGLE_BOOL, &gbAutomapSpew },
};

static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}

