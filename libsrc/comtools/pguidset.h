///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/pguidset.h $
// $Author: TOML $
// $Date: 1997/06/24 18:09:07 $
// $Revision: 1.5 $
//
// Provides a generic data structure for storing things identified by a
// guid, sorted by priority, with optional constraints
//

#ifndef __PGUIDSET_H
#define __PGUIDSET_H

#include <prikind.h>
#include <pdynarr.h>

struct sAbsoluteConstraint;
typedef struct _GUID GUID;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPriGuidSetEntry
//
// Provides the base entity from which clients contain or derive from when
// using cPriGuidSet.
//

struct sPriGuidSetEntry
{
    sPriGuidSetEntry();
    sPriGuidSetEntry(const GUID *, int priority = kPriorityNormal);

    // The unique identifier of the entry
    const GUID *pID;

    // The priority of the entry
    int priority;
};

typedef sPriGuidSetEntry *tPriGuidSetEntryPtr;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPriGuidSetBase
//
// Stores GUID identified objects, sorted by a priority (0-255), optionally
// constrained to resolve equal priority items.
//

// KLC 9/3/96, Made this a normal C function.
int LGAPI GetEntryPtrPriority(const void *p);

class cPriGuidSetBase
{
 public:

    cPriGuidSetBase();

    //
    // Sort the array
    //
    void Sort();

    //
    // Sort the array, applying constraints
    //
    void Sort(const sAbsoluteConstraint *, unsigned nNumConstraints);

    //
    // Query the array szie
    //
    index_t Size() const;

    //
    // Make the array empty
    //
    void Empty();


 protected:

    //
    // Append an item to the array
    //
     index_t Append(sPriGuidSetEntry *);


    // Entries stored in prioritized dynamic array
     cPriDynArray < sPriGuidSetEntry *, GetEntryPtrPriority > m_Entries;

 private:
    void ApplyConstraints(const sAbsoluteConstraint *, unsigned nNumConstraints);
};


///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cPriGuidSet
//

template < class T >
class cPriGuidSet : public cPriGuidSetBase
{
 public:

    T * &operator[] (index_t);
    T *&operator[] (int);
    // const T* & operator[] (index_t) const;

    operator T **();
    // operator const T**() const;

    index_t Append(T *);
    T *Remove(index_t);

};



///////////////////////////////////////////////////////////////////////////////
//
// cPriGuidSetBase inline members
//

inline cPriGuidSetBase::cPriGuidSetBase()
{
}

///////////////////////////////////////

inline void cPriGuidSetBase::Sort()
{
    m_Entries.Sort();
}

///////////////////////////////////////

inline void cPriGuidSetBase::Sort(const sAbsoluteConstraint * pConstraints, unsigned nNumConstraints)
{
    m_Entries.Sort();
    ApplyConstraints(pConstraints, nNumConstraints);
}

///////////////////////////////////////

inline index_t cPriGuidSetBase::Append(sPriGuidSetEntry * pToAdd)
{
    return m_Entries.Append(pToAdd);
}

///////////////////////////////////////

inline index_t cPriGuidSetBase::Size() const
{
    return m_Entries.Size();
}

///////////////////////////////////////

inline void cPriGuidSetBase::Empty()
{
    m_Entries.SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////
//
// cPriGuidSet inline members
//

template < class T >
 inline T * &cPriGuidSet < T > ::operator[] (index_t i)
{
    return *((T **) (&m_Entries[i]));
}

///////////////////////////////////////

template < class T >
 inline T * &cPriGuidSet < T > ::operator[] (int i)
{
    return *((T **) (&m_Entries[i]));
}

///////////////////////////////////////

template < class T >
 inline cPriGuidSet < T > ::operator T ** ()
{
    return (T **) ((sPriGuidSetEntry **) (m_Entries));
}

///////////////////////////////////////

template < class T >
 inline index_t cPriGuidSet < T > ::Append(T * pToAdd)
{
    return cPriGuidSetBase::Append(pToAdd);
}

///////////////////////////////////////

template < class T >
 inline T * cPriGuidSet < T > ::Remove(index_t i)
{
    T *pReturn = (T *) (m_Entries[i]);
    m_Entries.DeleteItem(i);
    return pReturn;
}

///////////////////////////////////////////////////////////////////////////////
//
// Prioritized array info
//

inline sPriGuidSetEntry::sPriGuidSetEntry()
 :
 pID(NULL),
 priority(kPriorityNormal)
{
}

///////////////////////////////////////

inline sPriGuidSetEntry::sPriGuidSetEntry(const GUID * pInitID, int initPriority)
 :
 pID(pInitID),
 priority(initPriority)
{
}

///////////////////////////////////////////////////////////////////////////////

#endif /* !__PGUIDSET_H */
