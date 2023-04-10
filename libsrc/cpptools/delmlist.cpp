///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/delmlist.cpp $
// $Author: TOML $
// $Date: 1996/04/29 17:54:15 $
// $Revision: 1.2 $

#include <lg.h>
#include <delmlist.h>
#include <str.h>
#include <ctype.h>

//
// Set the path list from a string
//
BOOL cDelimList::FromText(const char *pList, char Delimiter)
    {
    char szDelim[2];
    szDelim[0] = Delimiter;
    szDelim[1] = '\0';

    return FromText(pList, szDelim);
    }


//
// Set the path list from a string
//
BOOL cDelimList::AppendText(const char *pList, char Delimiter)
    {
    char szDelim[2];
    szDelim[0] = Delimiter;
    szDelim[1] = '\0';

    return AppendText(pList, szDelim);
    }


//
// Set the path list from a string
//
BOOL cDelimList::FromText(const char *pList, const char *pszDelimiters)
    {
    unsigned long i = ValidateText(FALSE, pList, pszDelimiters);
    if (i == 0)
        return FALSE;

    DestroyAll();

    ValidateText(TRUE, pList, pszDelimiters);

    return TRUE;
    }


//
// Set the path list from a string
//
BOOL cDelimList::AppendText(const char *pList, const char *pszDelimiters)
    {
    unsigned long i = ValidateText(FALSE, pList, pszDelimiters);
    if (i == 0)
        return FALSE;

    ValidateText(TRUE, pList, pszDelimiters);

    return TRUE;
    }


//
// Validate the path list
//

BOOL cDelimList::IsDelimiter(char c1, const char *pszDelimiters)
    {
    char c2;
    while ((c2 = *pszDelimiters) != '\0')
        {
        if (isspace(c2))
            continue;
        if (c2 == c1)
            return TRUE;
        pszDelimiters++;
        }
    return FALSE;
    }


unsigned long cDelimList::ValidateText(BOOL fSave, const char *pList, const char *pszDelimiters)
    {
    BEGIN_DEBUG_MSG_TRUE(fSave, "cDelimList::ValidateText()");
    if (!pszDelimiters || !*pszDelimiters)
        return 0;

    unsigned long i = 0;
    register const char *p = pList;
    for (;; p++)
        {
        // Skip the leading spaces
        for (; *p; p++)
            if (!isspace(*p))
                break;

        // If we are at the end of the string...
        if (*p == 0)
            break;

        // Remember the start of the file
        const char *pFile = p;

        const char *pTrailSpace = 0;
        // Scan until a delimiter is found...
        for (; *p; p++)
            {
            if (isspace(*p) && !pTrailSpace)
                pTrailSpace = p;
            else
                pTrailSpace = 0;

            if (IsDelimiter(*p, pszDelimiters))
                break;
            }

        // If we found a trailing space...
        if (pTrailSpace)
            p = pTrailSpace;

        // If the path name is null...
        if (pFile == p)
            continue;

        // If saving...
        if (fSave)
            {
            // ...then save the path name
            int Len = p - pFile;
            cStr Item(Len);
            memcpy(Item.GetBufferSetLength(Len), pFile, Len);
            DebugMsg3("Extracted \"%s\" from \"%s\" using \"%s\"", Item.operator const char *(), pList, pszDelimiters);
            Append(Item);
            }

        // Bump the path count
        i++;

        // Skip the spaces...
        for (; *p; p++)
            if (!isspace(*p))
                break;

        // If we aren't at a delimiter...
        if (!IsDelimiter(*p, pszDelimiters))
            break;
        }

    return *p == 0 ? i : 0;
    END_DEBUG;
    }
