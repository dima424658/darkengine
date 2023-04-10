// $Header: x:/prj/tech/winsrc/input/RCS/inperr.cpp 1.1 1997/10/06 20:43:38 JON Exp $
// DirectInput error spew

#include <dinput.h>
#include <dbg.h>

void DIErrorSpew(HRESULT hRes)
{
   switch (hRes)
   {
      case DIERR_INPUTLOST:
         Warning(("DI Error: input lost\n"));
         break;
      case DIERR_INVALIDPARAM:
         Warning(("DI Error: Invalid parameter or bad object state\n"));
         break;
      case DIERR_NOTACQUIRED:
         Warning(("DI Error: Object not acquired\n"));
         break;
      case DIERR_NOTINITIALIZED:
         Warning(("DI Error: Object not initialized\n"));
         break;
      case E_PENDING:
         Warning(("DI Error: Data not yet available\n"));
         break; 
      case DIERR_OTHERAPPHASPRIO:
         Warning(("DI Error: Other app has priority\n"));
         break;
      default:
         Warning(("DI Error: unknown type\n"));
   }
}
