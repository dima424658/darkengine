/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/antxscpt.cpp,v 1.8 1999/03/23 19:21:30 XEMU Exp $

#include <string.h>
#include <stdlib.h>

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <antxscpt.h>
#include <antxtype.h>
#include <animtext.h>
#include <dyntex.h>

#include <port.h>
#include <wrdbrend.h>
#include <wrfunc.h>
#include <objpos.h>
#include <texmem.h>

#include <propface.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>
#include <matrix.h>
#include <appagg.h>

// Must be last header
#include <dbmem.h>

static BOOL _txt_name_parse(const char *srcfam, const char *srctx, char *dstfam, char *dsttx)
{
   if (srcfam==NULL)
   {
      const char *pfront=strrchr(srctx,'/');  // find front or back slash
      const char *pback=strrchr(srctx,'\\');
      const char *p=(pfront>pback)?pfront:pback;
      if (p==NULL)
         return FALSE;
      strcpy(dsttx,p+1);
      strncpy(dstfam,(char *)srctx,p-srctx);
      dstfam[p-srctx]='\0';
   }
   else
   {
      strcpy(dstfam,(char *)srcfam);
      strcpy(dsttx,(char *)srctx);
   }
   return TRUE;
}

DECLARE_SCRIPT_SERVICE_IMPL(cAnimTextureSrv, AnimTexture)
{
public:
   // Within range of the reference object, change all of texture 1 to texture 2
   // maybe someday should be room based instead of obj based?  Right now room DB 
   // not really set up to provide the right bbox kind of data.
   STDMETHOD(ChangeTexture)(object refobj, const char *fam1, const char *tx1, 
      const char *fam2, const char *tx2)
   {
      int id1,id2;
      ObjID objid = ScriptObjID(refobj);
      char usefam1[_MAX_PATH], usefam2[_MAX_PATH];
      char usetx1[_MAX_PATH], usetx2[_MAX_PATH];  // since people seem to use LFN's

      if (!_txt_name_parse(fam1,tx1,usefam1,usetx1))
         return E_INVALIDARG;
      if (!_txt_name_parse(fam2,tx2,usefam2,usetx2))
         return E_INVALIDARG;

      id1 = texmemGetSinglebyName((char *)usefam1,(char *)usetx1);
      id2 = texmemGetSinglebyName((char *)usefam2,(char *)usetx2);

      HRESULT retval;
      AutoAppIPtr(DynTexture);
      retval = pDynTexture->ChangeTexture(objid,id1,id2);
      return(retval);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cAnimTextureSrv, AnimTexture);

