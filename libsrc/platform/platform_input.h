#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

EXTERN int PlatformReadKey(void);

EXTERN BOOL PlatformSetClipboardText(const char *text);

EXTERN int PlatformGetClipboardText(char *text, int capacity);

EXTERN void PlatformSetTextInputActive(BOOL active);
