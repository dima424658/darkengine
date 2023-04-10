///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/comtools/RCS/constrid.h $
// $Author: TOML $
// $Date: 1996/06/26 12:17:04 $
// $Revision: 1.1 $
//

#ifndef __CONSTRID_H
#define __CONSTRID_H

///////////////////////////////////////
//
// Fully specified constraint
//

struct sAbsoluteConstraint
    {
    const GUID * pIDBefore;
    const GUID * pIDAfter;
    };

typedef struct sAbsoluteConstraint sAbsoluteConstraint;


///////////////////////////////////////
//
// Context-relative constraint kind
//

enum eConstraintKind
   {
   kNullConstraint,
   kConstrainBefore,
   kConstrainAfter
   };

typedef enum eConstraintKind eConstraintKind;


///////////////////////////////////////
//
// Context-relative constraint
//

struct sRelativeConstraint
   {
   eConstraintKind   kind;
   const GUID *      pIDAgainst;
   };

typedef struct sRelativeConstraint sRelativeConstraint;


///////////////////////////////////////

#ifdef __cplusplus
inline void MakeAbsolute(const sRelativeConstraint & relative,
                         const GUID * pRelativeID,
                         sAbsoluteConstraint & absolute)
    {
    if (relative.kind == kConstrainBefore)
        {
        absolute.pIDBefore = pRelativeID;
        absolute.pIDAfter = relative.pIDAgainst;
        }
    else
        {
        absolute.pIDBefore = relative.pIDAgainst;
        absolute.pIDAfter = pRelativeID;
        }
    }
#endif

///////////////////////////////////////

#endif /* !__CONSTRID_H */
