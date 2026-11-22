#ifndef DARK_RENDER_EVENT_BRIDGE_H
#define DARK_RENDER_EVENT_BRIDGE_H

union SDL_Event;

bool DarkRenderBeginEventPump(void);
bool DarkRenderProcessOverlayEvent(const SDL_Event &event);
void DarkRenderWindowResized(void);
void DarkRenderWindowMinimized(void);
void DarkRenderWindowRestored(void);
void DarkRenderApplicationSuspended(void);
void DarkRenderApplicationResumed(void);
void DarkRenderEndEventPump(void);

#endif
