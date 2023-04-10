/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/uiapp.cpp,v 1.24 2000/03/07 19:55:38 toml Exp $

#include <config.h>
#include <res.h>
#include <aggmemb.h>
#include <appagg.h>
#include <event.h>
#include <region.h>
#include <hotkey.h>
#include <gadget.h>
#include <command.h>
#include <inpinit.h>

#include <uiapp.h>
#include <2dapp.h>
#include <init.h>
#include <gen_bind.h>

#include <mprintf.h>

// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
//  Goofy Root region getter
//

Region* GetRootRegion(void)
{
   uiSlab* slabptr;
   uiGetCurrentSlab(&slabptr);
   return slabptr->creg;
}

static Datapath uibindpath;

void read_key_and_menu_bindings(void)
{
   char path[128]; //DatapathAdd only supports 128.  
                   //Stop killing my stack pointer!  AMSD 2/16/00
                   //Mystery:  Why wasn't this broken before?
   DatapathClear(&uibindpath);
#ifdef PLAYTEST
   DatapathAdd(&uibindpath,"R:\\prj\\thief2\\src");
#endif // PLAYTEST
   path[0] = '\0';  //AIGH!!!! AMSD  This should keep datapath add from changing my EBP.
   config_get_raw("keybind_path",path,sizeof(path));
   DatapathAdd(&uibindpath,path);
#if 0
   path[0] = '\0';
   DatapathFind(&uibindpath,"keybind.cfg",path,sizeof(path));
   config_read_file(path,NULL);
#endif
   path[0] = '\0';
   DatapathFind(&uibindpath,"menus.cfg",path,sizeof(path));
   config_read_file(path,NULL);
}

#ifdef PLAYTEST
void uidefer_testfunc(void* arg)
{
   mprintf("arg is %x\n",arg);
}


void test_uidefer(void)
{
   uiDefer(uidefer_testfunc,(void*)0xDEADBEEF);
}

static Command commands[] =
{
   { "test_defer", FUNC_VOID, test_uidefer, "test func for uiDefer", HK_EDITOR,},
};
#endif  // PLAYTEST

//////////////////////////////////////////////////////////////
// INIT FUNC
//

static uiSlab _empty_slab;

#pragma off(unreferenced)
STDMETHODIMP uiSysInitFunc(IUnknown* goof)
{
   // I have no idea if this works
   uiMakeSlab(&_empty_slab,NULL,NULL);
   uiInit(&_empty_slab);

   uiSetMouseMotionPolling(TRUE);
   uiDoubleClicksOn[MOUSE_LBUTTON] = TRUE;
   //   uiAltDoubleClick = TRUE;  this screws up the editor

   LGadInit();

   read_key_and_menu_bindings();
#ifdef PLAYTEST
   COMMANDS(commands,HK_ALL);
#endif
   return kNoError;
}
#pragma on(unreferenced)

//////////////////////////////////////////////////////////////
// SHUTDOWN FUNC
//

#pragma off(unreferenced)
STDMETHODIMP uiSysShutdownFunc(IUnknown* goof)
{
   LGadTerm();
   uiShutdown();

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
// uiSysSysCreate()
//
// Called during AppCreateObjects, adds the uiSys system anonymously to the
// app aggregate
//

static struct _init_object
{
   DECLARE_C_COMPLEX_AGGREGATE_CONTROL();
} InitObject;


static sRelativeConstraint uiSysConstraints[] =
{
   { kConstrainAfter, &IID_Gr2d},
   { kConstrainAfter, &UUID_InputLib},
   { kNullConstraint, }
};


void LGAPI uiSysCreate(void)
{
   sAggAddInfo add_info = { NULL, NULL, NULL, NULL, 0, NULL} ;
   IUnknown* app = AppGetObj(IUnknown);
   INIT_C_COMPLEX_AGGREGATE_CONTROL(InitObject,
               FinalReleaseFunc,    // on final release
               NullFunc,      // connect
               NullFunc,      // post connect
               uiSysInitFunc,    // init
               uiSysShutdownFunc,   // end
               NullFunc);     // disconnect
   add_info.pID = &IID_UI;
   add_info.pszName = "UI";
   add_info.pControl = InitObject._pAggregateControl;
   add_info.controlPriority = kPriorityLibrary;
   add_info.pControlConstraints = uiSysConstraints;
   _AddToAggregate(app,&add_info,1);
   SafeRelease(InitObject._pAggregateControl);
   SafeRelease(app);
}

