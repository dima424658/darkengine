//		SYM.H		Symbol table header file
//		Rex E. Bradford (REX)
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/symtab.h 1.4 1996/10/21 09:35:06 TOML Exp $
* $Log: symtab.h $
 * Revision 1.4  1996/10/21  09:35:06  TOML
 * msvc
 * 
 * Revision 1.3  1996/01/22  15:39:14  DAVET
 *
 * Added cplusplus stuff.
 *
 * Revision 1.2  1994/02/10  15:58:56  eric
 * Fixed unbound parenthesis in SymSetUserSymDel.
 *
 * Revision 1.1  1993/05/03  10:53:49  rex
 * Initial revision
 *
*/

#ifndef SYM_H
#define SYM_H

#include <types.h>
#include <fix.h>
#include <slist.h>
#include <llist.h>
#include <hheap.h>

//	Symbol types, extensible by user

#ifdef __cplusplus
extern "C"  {
#endif

#define TSYM_NULL		0		// invalid symbol type
#define TSYM_INTEGER	1		// 32-bit integer
#define TSYM_FIX		2		// 32-bit fixed-point number
#define TSYM_STRING	3		// 32-bit string
#define TSYM_USER		4		// user-defined types start here

typedef ushort SymType;

//	Symbol layout

typedef struct {
	char *name;				// symbol name (allocated with StringAlloc)
	SymType type;			// symbol type (TSYM_XXX)
	union {
		long integer;		// integer value
		fix fixval;			// fixed-point value
		char *string;		// string value
		void *data;			// generic data ptr (for user types convenience)
		};
} Symbol;

//	Symbol table format.  A symbol table allocates its big data structures
//	on the heap, so the symbol table is small enough to put in static
//	or stack storage, or embedded in some other structure (82 bytes last
//	time I counted).

typedef struct {
	slist_head *hashTable;					// hash table of symbol slist headers
	short numSymHashEntries;				// size of hash table
	HheapHead symHeap;						// symbol heap header
	llist_head sortedList;					// sorted list header
	ushort flags;								// SYMTABF_XXX
	int (*f_cmp)(char *s1, char *s2);	// compare func
	ushort (*f_hash)(char *s, short hashSize);	// hash func
	void (*f_userSymDel)(Symbol *psym);	// user symbol delete func
} Symtab;

//	Symbol table flags, set at SymInitTable() time

#define SYMTABF_CASESENSE	0x01	// use case-sensitive comparison
#define SYMTABF_ALPHABETIC	0x02	// sort table alphabetically

//	Handy constants for numHashEntries argument to SymInitTable().
//	If some other constant is used, it should be prime.

#define SYMTAB_HASHSIZE_SMALL	101			// these are all prime
#define SYMTAB_HASHSIZE_STD	401
#define SYMTAB_HASHSIZE_LARGE	2039

//	Initialize a symbol table.  Set flags from SYMTABF_XXX, and set
//	numHashEntries to SYMTAB_HASHSIZE_XXX or your own prime number size

void SymInitTable(Symtab *psymtab, ushort flags, short numHashEntries);

//	Free all memory allocated by a symbol table.  It may now be
//	discarded or re-initialized.

void SymFreeTable(Symtab *psymtab);

//	Find a name in the table, return symbol ptr or NULL if not found

Symbol *SymFind(Symtab *pymstab, char *nm);

//	Add to symbol table.  If matching name is found and replace argument
//	is true, current entry will be replaced, irrespective of symbol types.
//	If matching name is found and replace argument is false, will warn and
//	return NULL.  After SymAdd(), you must fill in type and data field.

Symbol *SymAdd(Symtab *psymtab, char *nm, bool replace);

//	The next 4 are convenience functions which use SymAdd() and then
//	fill in type and data fields.  The returned symbol is fully formed.

Symbol *SymAddInt(Symtab *psymtab, char *nm, bool replace, long value);
Symbol *SymAddFix(Symtab *psymtab, char *nm, bool replace, fix value);
Symbol *SymAddString(Symtab *psymtab, char *nm, bool replace, char *str);
Symbol *SymAddUser(Symtab *psymtab, char *nm, bool replace, SymType type,
	void *data);

//	Remove symbol from table, freeing all its allocated memory.  This
//	includes calling "user symbol delete" function for user data types.

bool SymDel(Symtab *psymtab, char *nm);

//	Get pointer to first symbol in table in sorted order, or NULL
//	if table is empty.

Symbol *SymFirst(Symtab *psymtab);

//	Get ptr to next symbol after supplied one.  SymFirst() and SymNext()
//	can be used to traverse the symbol table in sorted order.  If you
//	plan to delete the current symbol from the table while traversing, get
//	the SymNext() of the current symbol before calling SymDel().

Symbol *SymNext(Symtab *psymtab, Symbol *psym);

//	Dump the symbol table to standard-out.  If sortOrder is true, dumps
//	in sorted order.  If it is false, dumps the linked list of each
//	hash entry.

void SymDumpTable(Symtab *psymtab, bool sortOrder);

//	Prints a symbol to a buffer

void SymPrint(char *buff, Symbol *psym);

//	Set user symbol delete function.  When SymDel() is called for
//	symbol types TSYM_USER and above, this function will be called.
//	It should delete data pointed to by the 'data' field in the symbol
//	appropriately.

#define SymSetUserSymDel(psymtab,f) ( (psymtab)->f_userSymDel = (f))

#ifdef __cplusplus
}
#endif

#endif

