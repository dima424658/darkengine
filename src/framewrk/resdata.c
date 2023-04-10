/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////
//
// $Header: r:/t2repos/thief2/src/framewrk/resdata.c,v 1.3 1998/09/29 20:43:27 dc Exp $
//

//
// Interface for application-specific data about resources.
//
// Dark has several places in which it quietly "associates" its own data
// with the resources from the Resource System. A lot of this ought to be
// in application-specific types, but the new resource system got there too
// late for that to be practical.
//
// Therefore, this section exists as a way to "patch" our own data into the
// resources that we are getting from the Resource System. The mechanism is
// fairly crude. We maintain an array here, with up to one element per
// resource. The mapping is not strictly one-to-one; not all resources have
// to have an array element. If a resource *does* have local data, then the
// resource is told which array element it has.
//
// The mechanism here is designed to be reusable, but the actual content
// of the array elements is up to the application. In the case of Dark, an
// element currently may contain an index into the texture array, and/or
// an index into the animation array.
//
// Note that most of the usual interface "functions" are actually macros
// in the .h file, for speed.
//

#include <mprintf.h>

// Headers for the application data:
#include <texmem.h>

#include <resdata.h>

#include <dbmem.h>

// The number of the current top element:
int cur_num_appdata;
// The head of a "linked list" of free elements:
int free_head;

appdataelem appdata[MAX_APP_RESOURCES];

void InitAppData()
{
   cur_num_appdata = 0;
   free_head = NO_RES_APP_DATA;
}

#ifdef NEED_TESTING
static void _show_list(void)
{
   int i;
   mprintf("Free head %d cur num %d\n",free_head,cur_num_appdata);
   for (i=0; i<cur_num_appdata; i++)
      if (appdata[i].pRes!=NULL)
         mprintf("ad %d free %d is %s\n",i,appdata[i].next_free,IRes_GetName(appdata[i].pRes));
      else
         mprintf("ad %d free %d\n",i,appdata[i].next_free);
}
#endif

int GetFreeAppDataElem()
{
   int idx;
   if (free_head != NO_RES_APP_DATA) {
      idx = free_head;
      free_head = appdata[free_head].next_free;
      appdata[idx].next_free = NO_RES_APP_DATA;
   } else {
      AssertMsg1((cur_num_appdata < MAX_APP_RESOURCES),
                "Out of application data handles (%d)!!!",cur_num_appdata);
      idx = cur_num_appdata++;
   }
   return idx;
}

int GetAppDataElem(IRes *pRes)
{
   int idx = IRes_GetAppData(pRes);
   if (idx == NO_RES_APP_DATA)
   {
      // This resource doesn't have any application data assigned yet,
      // so initialize a new element and assign it
      idx = GetFreeAppDataElem();
      appdata[idx].pRes = pRes;
      COMAddRef(pRes);
      appdata[idx].texidx = TEXMEM_NO_HND;
      appdata[idx].animdata = NULL;
      appdata[idx].next_free = NO_RES_APP_DATA;
      IRes_SetAppData(pRes, idx);
   }
   return idx;
}

void FreeAppRes(IRes *pRes)
{
   int idx = IRes_GetAppData(pRes);
   if (idx != NO_RES_APP_DATA)
   {
      IRes_SetAppData(pRes, NO_RES_APP_DATA);
      COMRelease(pRes);
      appdata[idx].pRes = NULL;
      appdata[idx].next_free = free_head;
      free_head = idx;
   }
}

// i hate everyone
// returns if it was final _APP-SIDE_ release
// ie. if ending count is 1, then we are done, res itself has the last lock
BOOL ReleaseAppRes(IRes *pRes)
{
   int idx    = IRes_GetAppData(pRes);
   int relcnt = COMRelease(pRes);
   if (idx != NO_RES_APP_DATA)
   {
      if (relcnt==2)
      {  // so here we decide we are gonna "be done"
         IRes_SetAppData(pRes, NO_RES_APP_DATA);
         appdata[idx].pRes = NULL;
         appdata[idx].next_free = free_head;
         free_head = idx;
         COMRelease(pRes);  // if relcnt was 2, this was final app-side release
      }
      return relcnt==2;
   }
   return relcnt==1;
}
