#include <lg.h>

#include <appmenu.h>

#ifdef EDITOR
#include <menus.h>
#endif

void ApplicationMenuCommand(unsigned commandId)
{
#ifdef EDITOR
   MenuCommand(commandId);
#else
   (void)commandId;
#endif
}
