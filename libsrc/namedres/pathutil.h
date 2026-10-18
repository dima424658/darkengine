#pragma once

#include <cstddef>

void GetNormalizedPath(const char* pOldPath, char** ppNewPath);
bool GetPathTop(const char* pFullPath, char* pTopLevel, size_t topLevelSize,
	const char** ppPathRest);
bool PathAndName(const char* pPathName, char* pPath, char* pName);
