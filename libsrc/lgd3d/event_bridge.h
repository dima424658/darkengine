#ifndef DARK_RENDER_EVENT_BRIDGE_H
#define DARK_RENDER_EVENT_BRIDGE_H

#include <SDL3/SDL.h>

bool DarkRenderBeginEventPump(void);
void DarkRenderHandleEvent(const SDL_Event *event);
bool DarkRenderEndEventPump(void);

#endif
