///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/delmlist.h $
// $Author: TOML $
// $Date: 1996/03/25 10:16:00 $
// $Revision: 1.1 $

#ifndef __DELMLIST_H
#define __DELMLIST_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

////////////////////////////////////////////////////////////
//
// CLASS: cDelimList
//
// This class stores a list of items given a delimited string.

class cDelimList
    {
public:
    BOOL FromText(const char *, char = ';');
    BOOL AppendText(const char *, char = ';');

    BOOL FromText(const char *, const char *);
    BOOL AppendText(const char *, const char *);

    virtual void DestroyAll() = 0;

protected:
    virtual BOOL Append(const char *) = 0;
    unsigned long ValidateText(BOOL, const char *, const char *);
    BOOL IsDelimiter(char, const char *);
    };

#endif
