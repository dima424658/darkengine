/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbshut.cpp $
 * $Revision: 1.7 $
 * $Author: TOML $
 * $Date: 1996/11/05 14:28:58 $
 *
 * Keyboard system shutdown routine.
 *
 * This file is part of the input library.
 */

#include <kb.h>
#include <defehand.h>
#include <recapi.h>
#include <inpcompo.h>

EXTERN bool kb_inited;
int kb_shutdown(void)
{
    // @TBD (toml 05-16-96): This is a poor way to handle this. Should correct by creating
    // proper COM interface to the input libraries.
    DisconnectInput();
    ReleaseInputComponents();

    kb_inited=0;
    return 0;
}



