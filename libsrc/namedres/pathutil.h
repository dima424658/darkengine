#pragma once

void GetNormalizedPath(const char* pOldPath, char** ppNewPath);
void GetPathTop(const char* pFullPath, char* pTopLevel, const char** ppPathRest);
bool PathAndName(const char* pPathName, char* pPath, char* pName);