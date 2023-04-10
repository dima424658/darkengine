//		HUFFEN.C		Huffman compression routines
//		Rex E. Bradford

/*
* $Header: x:/prj/tech/libsrc/dstruct/RCS/huffen.c 1.3 1996/10/10 13:16:20 TOML Exp $
* $Log: huffen.c $
 * Revision 1.3  1996/10/10  13:16:20  TOML
 * msvc
 * 
 * Revision 1.2  1995/04/17  17:57:15  dfan
 * I am checking in Rex's code which fixes the huffman encoder
 * 
 * Revision 1.1  1994/08/22  17:12:56  rex
 * Initial revision
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dbg.h>
#include <huff.h>

void HuffWalk1(HuffNode *htree, HuffNode *hnode, ulong *pcode, int *pnumBits,
	void (*func)(HuffNode *htree, HuffNode *hnode, ulong code, int numBits));

//	--------------------------------------------------------------
//		BUILD A HUFFMAN TREE
//	--------------------------------------------------------------
//
//	HuffBuild() builds a huffman tree.  Make sure to supply an array
//	which is twice as big as the part of it which holds terminal nodes.
//
//		htree = ptr to huffman tree (terminal nodes initialized with freq)
//		numTerminals = number of terminal nodes
//		pNumNodes = ptr to int, filled in with total # nodes by this routine
//
//	Returns: root node of new tree

HuffNode *HuffBuild(HuffNode *htree, int numTerminals, int *pNumNodes)
{
	int numNodes,i;
	HuffNode *h1,*h2,*root;

//	Set current tree count, clear non-terminal area

	numNodes = numTerminals;
	memset(htree + numTerminals, 0, sizeof(HuffNode) * numTerminals);

//	Do forever

	while (TRUE)
		{

//	Find the two smallest frequencies

		h1 = h2 = NULL;
		for (i = 0; i < numNodes; i++)
			{
			if ((htree + i) != h1)
				{
				if (htree[i].freq > 0 && htree[i].parent == NULL)
					{
					if (h1 == NULL || htree[i].freq < h1->freq)
						{
						if (h2 == NULL || h1->freq < h2->freq)
							h2 = h1;
						h1 = htree + i;
						}
					else if (h2 == NULL || htree[i].freq < h2->freq)
						h2 = htree + i;
					}
				}
			}
		if (h2 == NULL)
			{
			root = h1;
			break;
			}

//	Combine these two nodes and make new non-terminal node

		h1->parent = htree + numNodes;
		h2->parent = htree + numNodes;
		htree[numNodes].freq = h1->freq + h2->freq;
		htree[numNodes].right = h1;
		htree[numNodes].left = h2;
		numNodes++;
		}

//	Return total # nodes & root of tree

	if (pNumNodes)
		*pNumNodes = numNodes;
	return(root);
}

//	--------------------------------------------------------------
//		DUMP OUT HUFFMAN TREE
//	--------------------------------------------------------------
//
//	HuffDump() dumps out huffman tree
//
//		htree    = base of tree (NOT ROOT!)
//		hroot    = root of tree
//		numNodes = number nodes returned by HuffBuild

#ifdef DBG_ON

void HuffDump(HuffNode *htree, HuffNode *hroot, int numNodes)
{
	HuffNode *h;
	int i;

//	Dump in order

	for (i = 0, h = htree; i < numNodes; i++, h++)
		{
		if (h->right || h->left)
			{
			printf("%d: Token: NONTERM  freq: %d  (par: ", i, h->freq);
			if (h->parent)
				printf("%d  r: ", h->parent - htree);
			else
				printf("ROOT  r: ");
			if (h->right)
				printf("%d  l: ", h->right - htree);
			else
				printf("NONE  l: ");
			if (h->left)
				printf("%d)\n", h->left - htree);
			else
				printf("NONE)\n");
			}
		else
			{
			printf("%d: Token: $%x  freq: %d  (par: %d)\n",
				i, h->token, h->freq, h->parent - htree);
			}
		}
	printf("\n");
	fflush(stdout);

//	Walk tree

	printf("\nHUFFMAN CODE ASSIGNMENTS:\n\n");
	HuffWalk(htree, hroot, HuffPrintNode);
	fflush(stdout);
}

#endif

//	----------------------------------------------------------------
//		WALK HUFFMAN TREE
//	----------------------------------------------------------------
//
//	HuffWalk() walks through huffman tree, calling user function with
//	the current node, its bitcode, and length of code in bits.

void HuffWalk(HuffNode *htree, HuffNode *hroot,
	void (*func)(HuffNode *htree, HuffNode *hnode, ulong code, int numBits))
{
	ulong code;
	int numBits;

	code = numBits = 0;
	HuffWalk1(htree, hroot, &code, &numBits, func);
}

void HuffWalk1(HuffNode *htree, HuffNode *hnode, ulong *pcode, int *pnumBits,
	void (*func)(HuffNode *htree, HuffNode *hnode, ulong code, int numBits))
{
//	Call user function except at root

	if (*pnumBits && (hnode->left == NULL) && (hnode->right == NULL))
		{
		(*func)(htree, hnode, *pcode, *pnumBits);
		return;
		}

//	Walk into right half

	if (hnode->right)
		{
		*pcode <<= 1;
		*pnumBits += 1;
		HuffWalk1(htree, hnode->right, pcode, pnumBits, func);
		}

//	Step back up

	*pcode >>= 1;
	*pnumBits -= 1;

//	Walk into left half

	if (hnode->left)
		{
		*pcode = (*pcode << 1) | 1;
		*pnumBits += 1;
		HuffWalk1(htree, hnode->left, pcode, pnumBits, func);
		}

//	Step back up

	*pcode >>= 1;
	*pnumBits -= 1;
}

#ifdef DBG_ON

void HuffPrintNode(HuffNode *htree, HuffNode *hnode, ulong code, int numBits)
{
	ulong mask;
	int i;
	char buff[32];

	for (i = 0, mask = 1 << (numBits - 1); i < numBits; i++, mask >>= 1)
		buff[i] = (code & mask) ? '1' : '0';
	buff[numBits] = 0;

	printf("%5d: freq: %d  code: %s (%d bits)\n", hnode - htree, hnode->freq,
		buff, numBits);
}

#endif

//	---------------------------------------------------------------
//		CONVERT HUFFMAN TREE TO FLASH-TABLE FORMAT
//	---------------------------------------------------------------
//
//	HuffMakeFlashTables() makes flash-tables from huffman tree
//
//		hroot     = ptr to root of Huffman tree (NOT BASE!)
//		pFlashTab = ptr to flash-table buffer
//		tokSize   = size of tokens in bytes (high 4 bits used for nbits this entry)
//		bitsPri   = # bits per entry of primary table (usually 8 to 12)
//		bitsSec   = # bits per entry of secondary tables (usually 4)
//
//	Returns: ptr to multi table data structure

typedef struct {
	uchar *tab;			// ptr to this table
	short bitdepth;	// # bits this table based on
	short numbits;		// # bits encoded in this table
	short shift;		// # bits to shift token to get into low bits
	ushort mask;		// mask to apply to code
} HTable;

//	Stuff needed for callback routine when building multi-tables

static HTable htab0;				// base table
static uchar *pftBase;			// base of table
static long pftLength;			// length of pFlashTab array
static uchar *pftGrow;			// where we can start growing another table
static int htokSize;				// size of token in bytes
static int hbitsSec;				// size of secondary tables in bits
static int hnumbitsShift;		// # bits to shift numbits field into token
static ulong htokMask;			// mask to get entire token
static ulong hcodeMask;			// mask to get code part of token (w/o nbits)

void HuffGrowFlashTables(HuffNode *htree, HuffNode *hnode, ulong code,
	int numBits);

long HuffMakeFlashTables(HuffNode *htree, HuffNode *hroot, uchar *pFlashTab,
	long length, int tokSize, int bitsPri, int bitsSec)
{
	uchar *pft;

//	Init first entry in stack to master table

	htab0.tab = pFlashTab;
	htab0.bitdepth = 0;
	htab0.numbits = bitsPri;
//	htab0.shift = ((8 * tokSize) - 4) - bitsPri;
   htab0.shift = 32 - bitsPri;
	htab0.mask = 0xFFFF >> (16 - bitsPri);

//	Set up misc stuff

	pftBase = pFlashTab;
	pftLength = length;
	pftGrow = pFlashTab + ((1 << bitsPri) * tokSize);
	htokSize = tokSize;
	hbitsSec = bitsSec;
	hnumbitsShift = (htokSize * 8) - 4;	// where to put 4-bit numbits field
	htokMask = 0xFFFFFFFFL >> (32 - (8 * tokSize));
	hcodeMask = 0xFFFFFFFFL >> ((32 + 4) - (8 * tokSize));

//	Walk thru huffman tree, filling in and growing tables

	HuffWalk(htree, hroot, HuffGrowFlashTables);

//	Check that table has no holes

	pft = pftBase;
	while (pft < pftGrow)
		{
		if ((*(ulong *)pft & 0xFFFFFFL) == 0)
			{
			Warning(("HuffMakeFlashTables: empty entry at: $%x\n",
				(pft - pftBase) / htokSize));
			break;
			}
		pft += htokSize;
		}

//	Return table length

	return(pftGrow - pftBase);
}

#ifdef __WATCOMC__
#pragma off(unreferenced);
#endif

void HuffGrowFlashTables(HuffNode *htree, HuffNode *hnode, ulong code,
	int numBits)
{
	HTable htab;
	int numEntries,entry,i;
	ulong token;
	uchar *p;
	ulong *pl;
	ulong codesh;

//	Assume base table, align codeword in high bits

	htab = htab0;
//	codesh = code << (hnumbitsShift - numBits);	// align high
   codesh = code << (32 - numBits);

//	If code length too large for current table, advance to subtables

	while (numBits > (htab.bitdepth + htab.numbits))
		{

//	Get ptr to entry to branch off from

		entry = (codesh >> htab.shift) & htab.mask;
		pl = (ulong *)(htab.tab + (entry * htokSize));

//	If entry is 0, must build new table there

		if ((*pl & htokMask) == 0)
			{
			token = (pftGrow - pftBase) / htokSize;	// 0 nbits signifies continuation
			memcpy(pl, &token, htokSize);
			pftGrow += ((1 << hbitsSec) * htokSize);
			if (pftGrow > (pftBase + pftLength))
				{
				Warning(("HuffGrowMultiTables: exceeded multi-table area!\n"));
				return;
				}
			}

//	Advance to new table

		htab.tab = pftBase + ((*pl & hcodeMask) * htokSize);
		htab.bitdepth += htab.numbits;
		htab.numbits = hbitsSec;
//		htab.shift = ((8 * htokSize) - 4) - (htab.bitdepth + htab.numbits);
      htab.shift = 32 - (htab.bitdepth + htab.numbits);
		htab.mask = 0xFFFF >> (16 - hbitsSec);
		}

//	Insert one or more entries into current table

	numEntries = 1 << ((htab.bitdepth + htab.numbits) - numBits);
	token = hnode->token | ((numBits - htab.bitdepth) << hnumbitsShift);
	entry = (codesh >> htab.shift) & htab.mask;
	p = htab.tab + (entry * htokSize);
	for (i = 0; i < numEntries; i++, p += htokSize)
		memcpy(p, &token, htokSize);
}

#ifdef __WATCOMC__
#pragma on(unreferenced);
#endif

//	-----------------------------------------------------------------
//		PRINT OUT HUFFMAN FLASH-TABLES
//	-----------------------------------------------------------------

#ifdef DBG_ON

void HuffPrintFlashTables(uchar *pFlashTab, ulong length, int tokSize)
{
	uchar *pft,*pftLast;
	ulong lastTok,token,tokMask;

printf("\nHUFFMAN FLASH-TABLES\n\n");

	pft = pFlashTab;
	tokMask = 0xFFFFFFFFL >> (32 - (8 * tokSize));
	lastTok = 0xFFFFFFFFL;
	pftLast = NULL;

	while (pft != (pFlashTab + length))
		{
		token = *(ulong *)pft & tokMask;
		if (token != lastTok)
			{
			if (pftLast)
				printf("entry @$%x: $%x (num: %d)\n",
					(pftLast - pftBase) / tokSize, lastTok,
					(pft - pftLast) / tokSize);
			pftLast = pft;
			lastTok = token;
			}
		pft += tokSize;
		}

	printf("entry @$%x: $%x (num: %d)\n\n", (pftLast - pFlashTab) / tokSize,
		lastTok, (pft - pftLast) / tokSize);
	fflush(stdout);
}

#endif

//	----------------------------------------------------------------
//		APPLY RUN-LENGTH COMPRESSION TO MULTI-TABLES
//	----------------------------------------------------------------
//
//	HuffCompressFlashTables() compresses huffman flash-tables.

long HuffCompressFlashTables(uchar *pFlashTab, ulong length, int tokSize,
	ulong *pcBuff)
{
	uchar *pft,*pftLast;
	ulong tokMask,lastTok,token,runCount,runMax;
	ulong *pc;
	int runShift;

//	Set up for run-coding

	pc = pcBuff;
	pft = pFlashTab;
	tokMask = 0xFFFFFFFFL >> (32 - (8 * tokSize));
	lastTok = 0xFFFFFFFFL;
	runCount = 0;
	pftLast = NULL;

	if (tokSize == 3)
		{
		runMax = 255;
		runShift = 24;
		}
	else if (tokSize == 2)
		{
		runMax = 65535;
		runShift = 16;
		}
	else
		{
		Warning(("HuffCompressFlashTables: must have token size of 2 or 3!\n"));
		return(0);
		}

//	Do till done

	while (pft != (pFlashTab + length))
		{
		token = *(ulong *)pft & tokMask;

//	If new token, spit out last one

		if (token != lastTok)
			{
			if (pftLast && runCount)
				*pc++ = lastTok | (runCount << runShift);
			pftLast = pft;
			lastTok = token;
			runCount = 1;
			}

//	Else if same one, make sure run count doesn't overflow

		else if (++runCount == runMax)
			{
			*pc++ = (lastTok | (runMax << runShift));
			runCount = 0;
			}
		pft += tokSize;
		}

//	Spit out last token

	*pc++ = lastTok | (runCount << runShift);

//	Compute size & return it

	return((pc - pcBuff) * sizeof(ulong));
}

//	-------------------------------------------------------------
//		OUTPUT HUFFMAN CODES
//	-------------------------------------------------------------

void HuffEncode(HuffPtr *ph, ulong code, int numBits)
{
	while (--numBits >= 0)
		{
		if (code & (1 << numBits))
			*(ph->bits) |= (1 << ph->ibit);
		ph->ibit -= 1;
		if (ph->ibit < 0)
			{
			ph->ibit = 7;
			ph->bits += 1;
			*(ph->bits) = 0;
			}
		}
}

void HuffEncodeNode(HuffPtr *ph, HuffNode *hnode)
{
	ulong code;
	int nbit;

	nbit = 0;
	code = 0;
	while (hnode->parent)
		{
		if (hnode->parent->left == hnode)
			code |= (1 << nbit);
		++nbit;
		hnode = hnode->parent;
		}
	HuffEncode(ph, code, nbit);
}
