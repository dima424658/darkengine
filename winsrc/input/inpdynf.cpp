///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/winsrc/input/RCS/inpdynf.cpp $
// $Author: JON $
// $Date: 1997/10/06 20:46:01 $
// $Revision: 1.1 $
//

#include <windows.h>
#include <dinput.h>

#include "inpdynf.h"

ImplDynFunc(DirectInputCreateA, "dinput.dll", "DirectInputCreateA", 0);

BOOL LoadDirectInput()
{
    return (DynFunc(DirectInputCreateA).Load());
}
