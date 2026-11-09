#ifndef DARK_RENDER_EVENT_BRIDGE_H
#define DARK_RENDER_EVENT_BRIDGE_H

bool DarkRenderBeginEventPump(void);
void DarkRenderWindowResized(void);
void DarkRenderWindowMinimized(void);
void DarkRenderWindowRestored(void);
void DarkRenderApplicationSuspended(void);
void DarkRenderApplicationResumed(void);
void DarkRenderEndEventPump(void);

#endif
