#include "pathutil.h"
#include <lg.h>

#include <cstring>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

void GetNormalizedPath(const char* pOldPath, char** ppNewPath)
{
	if (!pOldPath)
	{
		*ppNewPath = nullptr;
		return;
	}

	auto nameLen = strlen(pOldPath);
	auto lastChar = pOldPath[nameLen - 1];
	auto needSlash = false;

	if (lastChar != '\\' && lastChar != '/')
	{
		++nameLen;
		needSlash = true;
	}

	*ppNewPath = static_cast<char*>(Malloc(nameLen + 1));
	auto* pNewPath = *ppNewPath;

	for (int i = 0; pOldPath[i] != '\0'; ++i)
	{
		if (pOldPath[i] == '/')
			pNewPath[i] = '\\';
		else
			pNewPath[i] = pOldPath[i];
	}

	if (needSlash)
		pNewPath[nameLen - 1] = '\\';

	pNewPath[nameLen] = '\0';
}

void GetPathTop(const char* pFullPath, char* pTopLevel, const char** ppPathRest)
{
	if (!pFullPath || !pTopLevel || !ppPathRest)
		return;

	auto i = 0;
	for (auto c = *pFullPath; c != '\0' && c != '\\' && c != '/'; c = pFullPath[i])
		++i;

	memmove(pTopLevel, pFullPath, i);
	pTopLevel[i] = '\\';
	pTopLevel[i + 1] = '\0';

	if (pFullPath[i] != '\0') // TODO ??
		*ppPathRest = &pFullPath[i + 1];
	else
		*ppPathRest = &pFullPath[i];
}

bool PathAndName(const char* pPathName, char* pPath, char* pName)
{
	if (!pPathName || !pPath || !pName)
		return false;

	auto nSize = strlen(pPathName);
	*pPath = '\0';
	*pName = '\0';

	if (nSize > 544)
	{
		Warning(("PathAndName: path looks incredibly bogus: %s\n", pPathName));
		return false;
	}

	int i;
	for (i = nSize - 1; i >= 0 && pPathName[i] != '\\' && pPathName[i] != '/'; --i)
		;
	if (nSize - i > 32)
	{
		Warning(("PathAndName: name too long: %s\n", &pPathName[i]));
		return false;
	}

	memmove(pName, &pPathName[i + 1], nSize - i + 1);
	if (i < 0)
		return false;

	memmove(pPath, pPathName, i + 1);
	pPath[i + 1] = '\0';

	return true;
}