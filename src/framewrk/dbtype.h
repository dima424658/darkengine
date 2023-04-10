/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/dbtype.h,v 1.3 2000/01/29 13:20:37 adurant Exp $
#pragma once

#ifndef __DBTYPE_H
#define __DBTYPE_H

typedef ulong edbFiletype;

#define DB_PARTITION_SHF 16
#define DB_PARTITION_MASK 0xFFFF0000

#define OBJ_PARTITION_SHF 8
#define OBJ_PARTITION_MASK  0x0000FF00

#define FILE_TYPE_MASK (DB_PARTITION_MASK|OBJ_PARTITION_MASK) 

#define DB_MSG(m) ((m) & ~FILE_TYPE_MASK)
#define DB_PARTITION(m) ((m) & DB_PARTITION_MASK)
#define DB_OBJ_PARTITION(m) ((m) & OBJ_PARTITION_MASK)


#endif // __DBTYPE_H

