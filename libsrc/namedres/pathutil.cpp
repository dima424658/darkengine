#include "pathutil.h"
#include <lg.h>
#include <storcnst.h>

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
	auto lastChar = nameLen ? pOldPath[nameLen - 1] : '\0';
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

bool GetPathTop(const char* pFullPath, char* pTopLevel, size_t topLevelSize,
	const char** ppPathRest)
{
	if (!pFullPath || !pTopLevel || !topLevelSize || !ppPathRest)
		return false;

	auto i = 0;
	for (auto c = *pFullPath; c != '\0' && c != '\\' && c != '/'; c = pFullPath[i])
		++i;
	if (static_cast<size_t>(i) + 2 > topLevelSize)
	{
		Warning(("Storage path component is too long: %s\n", pFullPath));
		pTopLevel[0] = '\0';
		*ppPathRest = pFullPath;
		return false;
	}

	memmove(pTopLevel, pFullPath, i);
	pTopLevel[i] = '\\';
	pTopLevel[i + 1] = '\0';

	if (pFullPath[i] != '\0') // TODO ??
		*ppPathRest = &pFullPath[i + 1];
	else
		*ppPathRest = &pFullPath[i];

	return true;
}

bool PathAndName(const char* pPathName, char* pPath, char* pName)
{
	if (!pPathName || !pPath || !pName)
		return false;

	auto pathLength = strlen(pPathName);
	*pPath = '\0';
	*pName = '\0';

	if (pathLength > 544)
	{
		Warning(("PathAndName: path looks incredibly bogus: %s\n", pPathName));
		return false;
	}

	int nSize = static_cast<int>(pathLength);
	int i;
	for (i = nSize - 1; i >= 0 && pPathName[i] != '\\' && pPathName[i] != '/'; --i)
		;
	if (nSize - i > MAX_STORENAME)
	{
		Warning(("PathAndName: name too long: %s\n", &pPathName[i]));
		return false;
	}

	// Copy the characters after the final separator plus the terminator.  The
	// old length was two bytes too large: for a root-level ZIP entry it read
	// past both the filename and its terminator, corrupting the heap long before
	// a large mission made the damage visible.
	memmove(pName, &pPathName[i + 1], nSize - i);
	if (i < 0)
		return false;

	memmove(pPath, pPathName, i + 1);
	pPath[i + 1] = '\0';

	return true;
}
