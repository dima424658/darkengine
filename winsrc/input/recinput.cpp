///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/recinput.cpp $
// $Author: JAEMZ $
// $Date: 1997/05/13 18:17:20 $
// $Revision: 1.6 $
//
// Recinput.c - The recording layer for all keyboard and mouse inputs.
// Bodisafa (Jeff)
//
// Updated & simplified (toml 10-16-96)
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <mprintf.h>

#include <recapi.h>
#include <error.h>
#include <kb.h>
#include <mouse.h>

#include <recapi.h>
#include <mouse.h>
#include <initguid.h>
#include <inputimp.h>
#include <inpcompo.h>

///////////////////////////////////////////////////////////////////////////////

static const char pszKeyStateTag[]      = "key state";
static const char pszKbsEventTag[]      = "kbs_event";
static const char pszKeyFlagsTag[]      = "key flags";
static const char pszKeyCookedTag[]     = "cooked key";
static const char pszMouseCoordTag[]    = "mouse coord";
static const char pszErrTypeTag[]       = "errtype";
static const char pszBoolTag[]          = "bool";
static const char pszShortTag[]         = "short";
static const char pszMouseEventTag[]    = "lgMouseEvent";
static const char pszMouseNextTag[]     = "lgMouseNextEvent";

///////////////////////////////////////////////////////////////////////////////

uchar kb_state(uchar code)
{
   uchar returnState = _kb_state(code);

   RecStreamAddOrExtract(g_pInputRecorder, &returnState, sizeof(returnState), pszKeyStateTag);

   return returnState;
}

///////////////////////////////////////

kbs_event kb_next(void)
{
   kbs_event returnEvent;
   _kb_next(returnEvent);

   RecStreamAddOrExtract(g_pInputRecorder, &returnEvent, sizeof(returnEvent), pszKbsEventTag);

   return returnEvent;
}

///////////////////////////////////////

kbs_event kb_look_next(void)
{
   kbs_event returnEvent;
   _kb_look_next(returnEvent);

   RecStreamAddOrExtract(g_pInputRecorder, &returnEvent, sizeof(returnEvent), pszKbsEventTag);

   return returnEvent;
}

///////////////////////////////////////

uchar kb_get_state(uchar code)
{
   uchar returnState = _kb_get_state(code);

   RecStreamAddOrExtract(g_pInputRecorder, &returnState, sizeof(returnState), pszKeyStateTag);

   return returnState;
}

///////////////////////////////////////

int kb_get_flags()
{
   int returnFlags = _kb_get_flags();

   RecStreamAddOrExtract(g_pInputRecorder, &returnFlags, sizeof(returnFlags), pszKeyFlagsTag);

   return returnFlags;
}

///////////////////////////////////////

bool kb_get_cooked(ushort* key)
{
   bool returnBool = _kb_get_cooked(key);

   RecStreamAddOrExtract(g_pInputRecorder, key, sizeof(ushort), pszKeyCookedTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnBool, sizeof(returnBool), pszBoolTag);

   return returnBool;
}

///////////////////////////////////////////////////////////////////////////////

errtype mouse_check_btn(int button, bool* result)
{
   errtype returnErr = _mouse_check_btn(button, result);

   RecStreamAddOrExtract(g_pInputRecorder, result, sizeof(bool), pszBoolTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnErr, sizeof(returnErr), pszErrTypeTag);

   return returnErr;
}

///////////////////////////////////////

errtype mouse_extremes( short *xmin, short *ymin, short *xmax, short *ymax )
{
   errtype returnErr = _mouse_extremes(xmin, ymin, xmax, ymax);

   return returnErr;
}

///////////////////////////////////////

errtype mouse_get_rate(short* xr, short* yr, short* thold)
{
   errtype returnErr = _mouse_get_rate(xr, yr, thold);

   RecStreamAddOrExtract(g_pInputRecorder, xr, sizeof(short), pszShortTag);
   RecStreamAddOrExtract(g_pInputRecorder, yr, sizeof(short), pszShortTag);
   RecStreamAddOrExtract(g_pInputRecorder, thold, sizeof(short), pszShortTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnErr, sizeof(returnErr), pszErrTypeTag);

   return returnErr;
}

///////////////////////////////////////

errtype mouse_get_xy(short* x, short* y)
{
   errtype returnErr = _mouse_get_xy(x, y);

   RecStreamAddOrExtract(g_pInputRecorder, x, sizeof(short), pszMouseCoordTag);
   RecStreamAddOrExtract(g_pInputRecorder, y, sizeof(short), pszMouseCoordTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnErr, sizeof(returnErr), pszErrTypeTag);

   return returnErr;
}

///////////////////////////////////////

errtype mouse_look_next(lgMouseEvent* result)
{
   errtype returnErr = _mouse_look_next(result);

   RecStreamAddOrExtract(g_pInputRecorder, result, sizeof(lgMouseEvent), pszMouseEventTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnErr, sizeof(returnErr), pszErrTypeTag);

   return returnErr;
}

///////////////////////////////////////

errtype mouse_next(lgMouseEvent* result)
{
   errtype returnErr = _mouse_next(result);

   RecStreamAddOrExtract(g_pInputRecorder, result, sizeof(lgMouseEvent), pszMouseNextTag);
   RecStreamAddOrExtract(g_pInputRecorder, &returnErr, sizeof(returnErr), pszErrTypeTag);

   return returnErr;
}

///////////////////////////////////////////////////////////////////////////////
