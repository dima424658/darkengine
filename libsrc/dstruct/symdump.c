//		SYMDUMP.C		Symbol table dump routine
//		Rex E. Bradford (REX)

/*
* $Header: r:/prj/lib/src/dstruct/rcs/symdump.c 1.1 1993/11/04 11:22:29 rex Exp $
* $Log: symdump.c $
 * Revision 1.1  1993/11/04  11:22:29  rex
 * Initial revision
 * 
*/

#include <ctype.h>
#include <string.h>

#include <lg.h>
#include <symtab.h>

//	THIS STUFF COPIED FROM SYMTAB.C - YIKES, MUST BE KEPT IN SYNC IF CHANGED

typedef struct {
	struct _slist;			// link to next item in hash entry
	struct _llist sl;		// the SortedSymbol's _llist
	Symbol sym;				// embedded symbol
} LinkedSymbol;

//	OK, WE'RE DONE WITH THAT NASTY BUSINESS

//	--------------------------------------------------------
//
//	SymDumpTable() dumps the symbol table
//
//		psymtab = ptr to symbol table
//		sorted  = if TRUE, dump in sorted order

void SymDumpTable(Symtab *psymtab, bool sorted)
{
	int i,thisEntry;
	slist_head *plist;
	LinkedSymbol *plinksym;
	Symbol *psym;
	char buff[80];

	if (sorted)
		{
		printf("Sorted dump of Symbol table: $%x\n", psymtab);
		psym = SymFirst(psymtab);
		while (psym)
			{
			SymPrint(buff, psym);
			printf("\t%s", buff);
			psym = SymNext(psymtab, psym);
			}
		}
	else
		{
		printf("Hash entry dump of Symbol table: $%x\n", psymtab);
		for (i = 0; i < psymtab->numSymHashEntries; i++)
			{
			plist = &psymtab->hashTable[i];
			thisEntry = 0;
			forallinslist(LinkedSymbol, plist, plinksym)
				{
				if (thisEntry == 0)
					{
					printf("Hash entry: %d\n", i);
					thisEntry = 1;
					}
				SymPrint(buff, &plinksym->sym);
				printf("\t%s", buff);
				}
			}
		}
}


