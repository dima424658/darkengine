//		SYMTAB.C		Symbol table
//		Rex E. Bradford (REX)
//
//		This module implements a general symbol table, which consists
//		of a set of named symbols, each of which has an associated
//		type and value.  Built-in types include integer, fixed-point,
//		and string; the user may extend these types.
//
//		Symbol names are hashed, and the hash index is used as a lookup
//		index into a singly-linked list of symbols at that hash location.
//		Symbol table performance may be tuned by adjusting the size
//		of the hash table (the size is an argument to SymInitTable()).
//		The hash routine is built-in.
//
//		The symbols themselves are stored in a homogenous heap, since
//		all symbols are the same size.  They have a "link header" stored
//		just before the symbol itself.  A single link connects all symbols
//		at the same hash entry together.  A double link connects all symbols
//		in the entire table together, in sort order.  The order in which
//		symbols are linked depends on whether the flag SYMTABF_ALPHABETIC
//		is set at table init time.  If set, symbols are linked alphabetically.
//		If not set, symbols are linked in order of insertion into the table
//		(new entries are put at the tail of the list).
//
//		Multiple symbol tables may be created and used in the same program.
//		Each has a private hash table for speedy symbol lookup, as well
//		as a private homogenous heap for symbol storage, which grows in
//		increments of about 1K (43 symbols per Malloc()).  Symbol names
//		and the value field of STRING symbols are stored using StringAlloc()
//		and freed with StringFree().  See that module's documentation for
//		more information (it uses a set of homogenous heaps for various
//		string sizes).
//
//		When a symbol table is created, two flags may be set:
//
//			SYMTABF_CASESENSE  = names are case-sensitive (default off)
//			SYMTABF_ALPHABETIC = entries are alphabetized (default off,
//				when off table is traversed from first entred to last).
//
/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/symtab.c 1.5 1996/10/21 09:34:56 TOML Exp $
* $Log: symtab.c $
 * Revision 1.5  1996/10/21  09:34:56  TOML
 * msvc
 * 
 * Revision 1.4  1993/11/04  11:21:49  rex
 * Moved SymDump() out into symdump.c, because it has printf()'s
 *
 * Revision 1.3  1993/05/04  09:53:42  rex
 * Fixed 4 add routines to check for NULL return from base SymAdd()
 *
 * Revision 1.2  1993/05/04  09:30:43  rex
 * Modified SymtabFreeTable() to use HheapFreeAll() to speed up & simplify
 *
 * Revision 1.1  1993/05/03  10:53:54  rex
 * Initial revision
 *
*/

#include <ctype.h>
#include <string.h>

#include <lg.h>
#include <lgsprntf.h>
#include <strall.h>
#include <symtab.h>

//	Allocate symbols in a homogenous heap, in chunks of about 1K

#define NUM_SYMBOLS_PER_CHUNK	(1024/sizeof(Symbol))

//	Symbols are sorted internally.  If SYMTABF_ALPHABETIC is set, they
//	are sorted alphabetically.  If not, they are sorted in order of
//	insertion into table (first ones at head, last at tail)

typedef struct {
	struct _llist;			// links to sorted entries
	Symbol sym;
} SortedSymbol;

//	Symbols are also linked together with others in the same hash table
//	entry.  LinkedSymbol's are what are actually stored in a symbol table.

typedef struct {
	struct _slist;			// link to next item in hash entry
	struct _llist sl;		// the SortedSymbol's _llist
	Symbol sym;				// embedded symbol
} LinkedSymbol;

//	Macros to envision a symbol in different ways

#define LINKED_TO_SORTED(plinksym) (SortedSymbol *)(&plinksym->sl)
#define SORTED_TO_LINKED(psortsym) (LinkedSymbol *)(((char *) psortsym) - sizeof(slist))
#define SYMBOL_TO_SORTED(psym) (SortedSymbol *)(((char *) psym) - sizeof(llist))
#define SYMBOL_TO_LINKED(psym) (SortedSymbol *)(((char *) psym) - (sizeof(slist)+sizeof(llist)))

//	Prototypes of private functions

LinkedSymbol *SymLookup(Symtab *psymtab, char *nm, slist_head **pplist);
void SymDelNoUnlink(Symtab *psymtab, Symbol *psym, bool delName);
ushort SymHash(char *s, short hashSize);
ushort SymHashUpper(char *s, short hashSize);

//	-------------------------------------------------------
//		INITIALIZATION AND DESTRUCTION
//	-------------------------------------------------------
//
//	SymInitTable() initializes a symbol table.
//
//		psymtab = ptr to symbol table
//		flags = SYMTABF_XXX (case-sensitive, alphabetic sorting)
//		numHashEntries = # entries in hash table, should be prime,
//								can use SYMTAB_SMALL, SYMTAB_STD, SYMTAB_LARGE

void SymInitTable(Symtab *psymtab, ushort flags, short numHashEntries)
{
	int i;

//	Allocate hash table of list headers and initialize them

	psymtab->hashTable = Malloc(sizeof(slist_head) * numHashEntries);
	for (i = 0; i < numHashEntries; i++)
		slist_init(&psymtab->hashTable[i]);

//	Initialize homogenous heap for symbol storage (non-shrinking)

	HheapInit(&psymtab->symHeap, sizeof(LinkedSymbol), NUM_SYMBOLS_PER_CHUNK,
		FALSE);

//	Init list of sorted symbols

	llist_init(&psymtab->sortedList);

//	Set some other fields

	psymtab->numSymHashEntries = numHashEntries;
	psymtab->f_userSymDel = NULL;

//	Set flags, use to determine compare & hash functions

	psymtab->flags = flags;
	if (psymtab->flags & SYMTABF_CASESENSE)
		{
		psymtab->f_cmp = strcmp;
		psymtab->f_hash = SymHash;
		}
	else
		{
		psymtab->f_cmp = strcmpi;
		psymtab->f_hash = SymHashUpper;
		}
}

//	--------------------------------------------------------
//
//	SymFreeTable() frees items in a symbol table
//
//		psymtab = ptr to symbol table

void SymFreeTable(Symtab *psymtab)
{
	SortedSymbol *psortsym;

//	Traverse entire sorted list, freeing symbol's stuff but not symbol

	forallinlist(SortedSymbol, &psymtab->sortedList, psortsym)
		SymDelNoUnlink(psymtab, &psortsym->sym, TRUE);

//	Free hashtable of symbol lists

	Free(psymtab->hashTable);

//	Free all symbols in one swell foop

	HheapFreeAll(&psymtab->symHeap);
}

//	--------------------------------------------------------
//		SYMBOL LOOKUP, ADDITION, DELETION
//	--------------------------------------------------------
//
//	SymFind() looks up a symbol, returns symbol ptr or NULL.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//
//	Returns: ptr to symbol, or NULL if not found

Symbol *SymFind(Symtab *psymtab, char *nm)
{
	slist_head *plist;
	LinkedSymbol *plinksym;

	plinksym = SymLookup(psymtab, nm, &plist);
	if (plinksym)
		return(&plinksym->sym);
	else
		return(NULL);
}

//	--------------------------------------------------------
//
//	SymAdd() adds to symbol table, will complain if found.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		replace = if TRUE, replace existing symbol
//
//	Returns: ptr to newly added symbol (must fill in fields yourself),

Symbol *SymAdd(Symtab *psymtab, char *nm, bool replace)
{
	slist_head *plist;
	LinkedSymbol *plinksym;
	SortedSymbol *psortsym;
	SortedSymbol *psortsym2;
	ushort entry;

//	Compute hash table index for string, calc ptr into hash table

	entry = (*psymtab->f_hash)(nm, psymtab->numSymHashEntries);
	plist = &psymtab->hashTable[entry];

//	Search through hash table, see if already there

	forallinslist(LinkedSymbol, plist, plinksym)
		{
		if ((*psymtab->f_cmp)(nm, plinksym->sym.name) == 0)
			{
			if (replace)
				{
				SymDelNoUnlink(psymtab, &plinksym->sym, FALSE);
				plinksym->sym.type = TSYM_NULL;			// for safety
				return(&plinksym->sym);
				}
			else
				{
				Warning(("SymAdd: duplicate symbol: %s\n", nm));
				return(NULL);
				}
			}
		}

//	No, add it

	plinksym = HheapAlloc(&psymtab->symHeap);
	plinksym->sym.name = StringAlloc(nm);
	plinksym->sym.type = TSYM_NULL;			// for safety

//	And link into list at this hash table

	slist_add_head(plist, plinksym);

//	If alphabetic, find place in sorted list, else just put at tail

	psortsym = LINKED_TO_SORTED(plinksym);
	if (psymtab->flags & SYMTABF_ALPHABETIC)
		{
		psortsym2 = llist_head(&psymtab->sortedList);
		while ((psortsym2 != llist_end(&psymtab->sortedList)) &&
			((*psymtab->f_cmp)(nm, psortsym2->sym.name) >= 0))
				psortsym2 = llist_next(psortsym2);
		llist_insert_before(psortsym, psortsym2);
		}
	else
		llist_add_tail(&psymtab->sortedList, psortsym);

//	Return ptr to symbol (not linkedSymbol or sortedSymbol)

	return(&plinksym->sym);
}

//	--------------------------------------------------------
//
//	SymAddInt() adds an integer symbol.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		replace = if TRUE, replace existing symbol
//		value   = integer value
//
//	Returns: ptr to new symbol or NULL (if found & replace FALSE)

Symbol *SymAddInt(Symtab *psymtab, char *nm, bool replace, long value)
{
	Symbol *psym;

	psym = SymAdd(psymtab, nm, replace);
	if (psym)
		{
		psym->type = TSYM_INTEGER;
		psym->integer = value;
		}
	return(psym);
}

//	--------------------------------------------------------
//
//	SymAddFix() adds a fixed-point symbol.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		replace = if TRUE, replace existing symbol
//		value   = fixed-point value
//
//	Returns: ptr to new symbol or NULL (if found & replace FALSE)

Symbol *SymAddFix(Symtab *psymtab, char *nm, bool replace, fix value)
{
	Symbol *psym;

	psym = SymAdd(psymtab, nm, replace);
	if (psym)
		{
		psym->type = TSYM_FIX;
		psym->fixval = value;
		}
	return(psym);
}

//	--------------------------------------------------------
//
//	SymAddString() adds an string symbol.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		replace = if TRUE, replace existing symbol
//		str     = ptr to string
//
//	Returns: ptr to new symbol or NULL (if found & replace FALSE)

Symbol *SymAddString(Symtab *psymtab, char *nm, bool replace, char *str)
{
	Symbol *psym;

	psym = SymAdd(psymtab, nm, replace);
	if (psym)
		{
		psym->type = TSYM_STRING;
		psym->string = StringAlloc(str);
		}
	return(psym);
}

//	--------------------------------------------------------
//
//	SymAddUser() adds a a user symbol.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		replace = if TRUE, replace existing symbol
//		type    = symbol type
//		data    = ptr to user data
//
//	Returns: ptr to new symbol or NULL (if found & replace FALSE)

Symbol *SymAddUser(Symtab *psymtab, char *nm, bool replace, SymType type,
	void *data)
{
	Symbol *psym;

	psym = SymAdd(psymtab, nm, replace);
	if (psym)
		{
		psym->type = type;
		psym->data = data;
		}
	return(psym);
}

//	--------------------------------------------------------
//
//	SymDel() deletes an entry from a symbol table.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//
//	Returns: TRUE if symbol found & deleted, FALSE if not found

bool SymDel(Symtab *psymtab, char *nm)
{
	slist_head *plist;
	LinkedSymbol *plinksym;
	LinkedSymbol *plinkbefore;
	SortedSymbol *psortsym;

//	Look up symbol, return FALSE if not found

	plinksym = SymLookup(psymtab, nm, &plist);
	if (plinksym == NULL)
		return FALSE;

//	Delete symbol's contents without unlinking

	SymDelNoUnlink(psymtab, &plinksym->sym, TRUE);

//	Remove from hash table's linked list

	if (plinksym == slist_head(plist))
		{
		slist_remove(plinksym, plist);
		}
	else
		{
		forallinslist(LinkedSymbol, plist, plinkbefore)
			{
			if (slist_next(plinkbefore) == plinksym)
				{
				slist_remove(plinksym, plinkbefore);
				break;
				}
			}
		}

//	Remove from sorted linked list

	psortsym = LINKED_TO_SORTED(plinksym);
	llist_remove(psortsym);

//	Free symbol's memory

	HheapFree(&psymtab->symHeap, plinksym);

	return TRUE;
}

//	--------------------------------------------------------
//		SYMBOL TABLE TRAVERSAL
//	--------------------------------------------------------
//
//	SymFirst() returns the first sorted symbol or NULL if none.
//
//		psymtab = ptr to symbol table
//
//	Returns: ptr to 1st symbol, or NULL if table empty.

Symbol *SymFirst(Symtab *psymtab)
{
	SortedSymbol *psortsym;

	psortsym = llist_head(&psymtab->sortedList);
	if (psortsym == llist_end(&psymtab->sortedList))
		return(NULL);
	else
		return(&psortsym->sym);
}

//	---------------------------------------------------------
//
//	SymNext() returns the next sorted symbol, NULL if no more.
//
//		psymtab = ptr to symbol table
//		psym = ptr to symbol
//
//	Returns: ptr to next symbol, or NULL

Symbol *SymNext(Symtab *psymtab, Symbol *psym)
{
	SortedSymbol *psortsym;

	psortsym = llist_next(SYMBOL_TO_SORTED(psym));
	if (psortsym == llist_end(&psymtab->sortedList))
		return(NULL);
	else
		return(&psortsym->sym);
}

//	--------------------------------------------------------
//		SYMBOL PRINT ROUTINE
//	--------------------------------------------------------
//
//	SymPrint() prints a symbol into a buffer
//
//		buff = ptr to buffer
//		psym = ptr to symbol

void SymPrint(char *buff, Symbol *psym)
{
	switch (psym->type)
		{
		case TSYM_NULL:
			lg_sprintf(buff, "Name: %s  (NULL SYMBOL)\n", psym->name);
			break;

		case TSYM_INTEGER:
			lg_sprintf(buff, "Name: %s  INTEGER: %d\n", psym->name, psym->integer);
			break;

		case TSYM_FIX:
			lg_sprintf(buff, "Name: %s  FIX: %f\n", psym->name, psym->fixval);
			break;

		case TSYM_STRING:
			lg_sprintf(buff, "Name: %s  STRING: %s\n", psym->name, psym->string);
			break;

		default:
			lg_sprintf(buff, "Name: %s  (USER SYMBOL)\n", psym->name);
			break;
		}
}

//	--------------------------------------------------------
//		INTERNAL ROUTINES
//	--------------------------------------------------------
//
//	SymLookup() looks up name in symbol table.
//
//		psymtab = ptr to symbol table
//		nm      = name string
//		pplist  = ptr to list header ptr (returned by this func)
//
//	Returns: ptr to linkSymbol, or NULL if not found

LinkedSymbol *SymLookup(Symtab *psymtab, char *nm, slist_head **pplist)
{
	LinkedSymbol *plinksym;

//	Compute hash table index for string, look up ptr in hash table

	*pplist = &psymtab->hashTable[(*psymtab->f_hash)(nm,
		psymtab->numSymHashEntries)];

//	Look for match thru any entries

	forallinslist(LinkedSymbol, *pplist, plinksym)
		{
		if ((*psymtab->f_cmp)(nm, plinksym->sym.name) == 0)
			return(plinksym);
		}

//	No match, return NULL

	return(NULL);
}

//	---------------------------------------------------------
//
//	SymDelNoUnlink() deletes a symbol's data without do unlinking.
//
//		psymtab = ptr to symbol table
//		psym    = ptr to symbol
//		delName = if TRUE, delete name too

void SymDelNoUnlink(Symtab *psymtab, Symbol *psym, bool delName)
{
//	If user type, call user del function if there is one

	if (psym->type >= TSYM_USER)
		{
		if (psymtab->f_userSymDel)
			(*psymtab->f_userSymDel)(psym);
		}

//	Else if string type, free string

	else if (psym->type == TSYM_STRING)
		StringFree(psym->string);

//	For all symbols, free name (if delName flag set)

	if (delName)
		StringFree(psym->name);
}

//	----------------------------------------------------------
//
//	SymHash() computes hash index for a string.
//
//	Uses pjwhash algorithm described on p.153 of MKS Lex & Yacc
//	version 3.1 book, hardcoded for 32-bit ints.
//
//		s    = ptr to string
//		size = size of hash table
//
//	Returns: hash table index

ushort SymHash(char *s, short size)
{
	ulong hash,tmp;

	for (hash = 0; *s; ++s)
		{
		hash = (hash << 4) + *s;
		if ((tmp = hash) & 0xF0000000L)
			hash = (hash ^ (tmp >> 24)) & 0x0FFFFFFFL;
		}
	return(hash % size);
}

//	-----------------------------------------------------------
//
//	SymHashUpper() is an uppercase version of SymHash, for case-
//		insensitive symbol tables.

ushort SymHashUpper(char *s, short size)
{
	ulong hash,tmp;

	for (hash = 0; *s; ++s)
		{
		hash = (hash << 4) + toupper(*s);
		if ((tmp = hash) & 0xF0000000L)
			hash = (hash ^ (tmp >> 24)) & 0x0FFFFFFFL;
		}
	return(hash % size);
}

