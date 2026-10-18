#pragma once

#include <types.h>

typedef void (*SndTimerCallback)(void *user);

uint32 SndTimeMs(void);
uint32 SndStartPeriodicTimer(uint32 periodMs, SndTimerCallback callback, void *user);
void SndStopPeriodicTimer(uint32 timerId, uint32 periodMs);
void SndPrioritizeStreamingThread(void);
