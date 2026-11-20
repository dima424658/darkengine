#pragma once

#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <types.h>

EXTERN FILE *PlatformFopen(const char *path, const char *mode);
