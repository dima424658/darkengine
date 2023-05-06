/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/resapp.c,v 1.21 2000/02/19 13:16:31 toml Exp $

#include <lg.h>
#include <comtools.h>

#include <config.h>
#include <res.h>
#include <aggmemb.h>
#include <appagg.h>

#include <resapp.h>
#include <resagg.h>
#include <resguid.h>
#include <init.h>

// namedres stuff:
#include <storeapi.h>
#include <resapi.h>
#include <respaths.h>
#include <palrstyp.h>
#include <imgrstyp.h>
#include <strrstyp.h>
#include <sndrstyp.h>
#include <fonrstyp.h>
#include <mdlrstyp.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

ISearchPath *gContextPath = NULL;

//////////////////////////////////////////////////////////////
// INIT FUNC
//

static void register_res_types()
{
   IResMan * pResMan = AppGetObj(IResMan);
   IResType *pResType;

   pResType = MakePaletteResourceType();
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);
   pResType = MakeImageResourceType();
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);
   pResType = MakeStringResourceType(); 
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);
   pResType = MakeSoundResourceType(); 
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);
   pResType = MakeFontResourceType();
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);
   pResType = MakeModelResourceType();
   IResMan_RegisterResType(pResMan, pResType);
   SafeRelease(pResType);   
}


#define LRES_SIZE 256

void InitAppData();

#pragma off(unreferenced)
static STDMETHODIMP InitFunc(IUnknown* goof)
{
   char path[256]; 
   int mem;
   IResMan *pResMan;

   // Prep the original resource system:
   ResAddPath(".\\data");
   
   if (config_get_raw("data_path",path,sizeof(path)))
      ResAddPath(path);
   else
      path[0] = '\0';

   if (config_get_int("mem_cap",&mem))
      ResMemSetCap(mem*1024*1024); // units of megs

   // Then prep the new resource system:
   // (Geez, enough resource subsystems, already...)
   pResMan = AppGetObj(IResMan);

   //
   // Set up the variant path for language 
   if (config_get_raw("language",path,sizeof(path)))
   {
      char extra[80] = ""; 
      ISearchPath* vpath = NULL; 
      char buf[64]; 

      config_get_raw("variant_path",extra,sizeof(extra));
      sprintf(buf,"%s;%s;.",path,extra); 
      vpath = IResMan_NewSearchPath(pResMan,buf);
      IResMan_SetDefaultVariants(pResMan,vpath); 
      SafeRelease(vpath); 
   }

   gContextPath = IResMan_NewSearchPath(pResMan, "");

   // PATCH, add a patch path
   {
      char install[80] = "."; 
      config_get_raw("install_path",install,sizeof(install));
      sprintf(path,"%s\\patch",install); 
      ISearchPath_AddPath(gContextPath, path);
   }

   ISearchPath_AddPath(gContextPath,"."); 

   if (path[0])
      ISearchPath_AddPath(gContextPath, path);

   // This is picked up from the ResName system; 
   // we might consider changing the name:
   if (config_get_raw("resname_base", path, sizeof(path)))
      ISearchPath_AddPath(gContextPath, path);

   // With this in place, you generally shouldn't need to get at
   // gContextPath directly, unless you're doing something really fancy...
   IResMan_SetGlobalContext(pResMan, gContextPath);
   IResMan_SetDefaultPath(pResMan, gContextPath);
   register_res_types(); 

   // Prep the statistic reporting, if desired:
   if (config_is_defined("report_resource_stats")) {
      IResStats *pResStats;
      Verify(SUCCEEDED(COMQueryInterface(pResMan,
                                         IID_IResStats,
                                         (void **) &pResStats)));
      IResStats_SetMode(pResStats, kResStatMode_Summary, TRUE);
      IResStats_SetMode(pResStats, kResStatMode_Types, TRUE);
      SafeRelease(pResStats);
   }
   SafeRelease(pResMan);

   InitAppData();

   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//


#pragma off(unreferenced)
STDMETHODIMP ShutdownFunc(IUnknown* goof)
{
   SafeRelease(gContextPath);

   if (config_is_defined("report_resource_stats")) {
      IResMan *pResMan = AppGetObj(IResMan);
      IResStats *pResStats;
      Verify(SUCCEEDED(COMQueryInterface(pResMan,
                                         IID_IResStats,
                                         (void **) &pResStats)));
      IResStats_Dump(pResStats, NULL);
      SafeRelease(pResStats);
      SafeRelease(pResMan);
   }

   return kNoError;
}
#pragma on(unreferenced)


#pragma off(unreferenced)
static STDMETHODIMP NullFunc(IUnknown* goof)
{
   return kNoError;
}
#pragma on(unreferenced)

#pragma off(unreferenced)
static void STDMETHODCALLTYPE FinalReleaseFunc(IUnknown* goof)
{
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// ResSysSysCreate()
//
// Called during AppCreateObjects, adds the ResSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject; 


extern const GUID UUID_Res;

static sRelativeConstraint ResSysConstraints[] = 
{
   { kConstrainAfter, &UUID_Res},
   { kConstrainAfter, &IID_IResMan},
   { kNullConstraint, }
}; 


void LGAPI ResSysCreate(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ; 
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect   
               InitFunc,   // init
               ShutdownFunc,  // end
               NullFunc);     // disconnect
   add_info.pID = &IID_Res;
   add_info.pszName = "Engine-specific Res"; 
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityLibrary; 
   add_info.pControlConstraints = ResSysConstraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}

