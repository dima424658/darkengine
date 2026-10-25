#pragma once

#ifndef __APPMENU_H
#define __APPMENU_H

// Dispatch a native application-menu command when the current executable
// supplies one. The game runners have no native application menu.
EXTERN void ApplicationMenuCommand(unsigned commandId);

#endif // __APPMENU_H
