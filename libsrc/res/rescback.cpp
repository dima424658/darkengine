// Resource loading callbacks
// James Fleming
// 
/*
* $Header: x:/prj/tech/libsrc/res/RCS/rescback.cpp 1.1 1997/05/17 17:45:37 JAEMZ Exp $
* $log: $
*/

#include <res.h>
#include <res_.h>
#include <_res.h>

// Installable callback array on type
fResCallback pfresCallbacks[NUM_RESTYPENAMES];

void ResInstallTypeCallback(int type,fResCallback cback)
{
   pfresCallbacks[type] = cback;
}

// Makes the appropriate callbacks
// for each compound ref of type
// or a single res of type...
void ResDoTypeCallbacks(Id id)
{
   fResCallback fcback;
   ResDesc *prd;
   ResDesc2 *prd2;
   int i;

   prd2 = RESDESC2(id);

   fcback = pfresCallbacks[prd2->type];

   if (!fcback) return;

   prd = RESDESC(id);

   if (prd2->flags & RDF_COMPOUND) {
      RefTable *prt;
      prt = (RefTable *)prd->ptr;
      for (i=0;i<prt->numRefs;++i) {
         fcback(REFPTR(prt,i),RefSize(prt,i));
      }
   } else {
      fcback((uchar *)(prd->ptr),prd->size);
   }
}
