//		DBGCFG.C		Debug/reporting system, config routines
//		Rex E. Bradford (REX)
//
/*
* $Header: x:/prj/tech/libsrc/lg/RCS/dbgcfg.c 1.18 1996/08/15 14:59:50 TOML Exp $
* $Log: dbgcfg.c $
 * Revision 1.18  1996/08/15  14:59:50  TOML
 * Backed out previous attempts at Windows
 * 
 * Revision 1.17  1996/02/06  09:32:09  TONY
 * First stab at making SPEW system functional under Windows.
 *
 * Revision 1.16  1994/02/28  10:53:20  rex
 * Moved DbgGetKey() to dbg.c
 *
 * Revision 1.15  1993/11/15  11:42:08  rex
 * Changed lglibdir to 'lib' (for finding debug config files)
 *
 * Revision 1.14  1993/07/09  09:33:48  rex
 * Encapsulated everything with #ifdef DBG_ON
 *
 * Revision 1.13  1993/04/22  13:59:12  rex
 * Changed mono config key install thingy from flag to func ptr
 *
 * Revision 1.12  1993/04/22  11:39:37  rex
 * Modified so can optionally use kblib for key input
 *
 * Revision 1.11  1993/03/25  15:40:14  rex
 * Fixed DbgSaveConfig bug where it didn't write a bank properly which had
 * some DBG bits on and some off
 *
 * Revision 1.10  1993/03/25  15:31:55  matt
 * Allowed bank and slot names to start with a digit by checking to make
 * sure all chars in a token are digits before assuming the token is a number.
 *
 * Revision 1.9  1993/03/25  13:02:53  rex
 * Fixed last install
 * /.
 *
 * Revision 1.8  1993/03/25  10:50:01  rex
 * Took out signoff message, made TAB key return from slots page in MonoCfg
 *
 * Revision 1.7  1993/03/18  12:39:56  rex
 * Added fclose() to DbgLoadConfig() - whoops!
 *
 * Revision 1.6  1993/02/25  12:52:46  rex
 * Changed init & exit code
 *
 * Revision 1.5  1993/02/04  13:25:34  rex
 * Fixed bug in last item (whoops)
 *
 * Revision 1.4  1993/02/04  13:17:38  rex
 * Added F5 key to repeat last logfile
 *
 * Revision 1.3  1993/02/02  17:07:53  rex
 * Fixed logpath (".\", not "..\\")
 *
 * Revision 1.2  1993/02/02  16:47:06  rex
 * Fixed logpath bug in DbgSaveConfig()
 *
 * Revision 1.1  1993/01/29  09:47:58  rex
 * Initial revision
 *
*/

#ifdef DBG_ON

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

#include <dbg.h>
#include <datapath.h>
#include <kb.h>
#include <kbcook.h>
#include <mprintf.h>

#include <keydefs.h>

#define DbgIsDbgSame(pb) ((pb->gate[DG_DBG] == 0) || (pb->gate[DG_DBG] == DBG_SLOT_MASK))
#define DbgIsMonoSame(pb) ((pb->gate[DG_MONO] == 0) || (pb->gate[DG_MONO] == DBG_SLOT_MASK))
#define DbgIsFuncSame(pb) ((pb->gate[DG_FUNC] == 0) || (pb->gate[DG_FUNC] == DBG_SLOT_MASK))

void DbgMonoHandleInput(short page);
bool DbgMonoReadInput(char *prompt, short x, short y, char *buff);
void DbgMonoClearInput();
void DbgMonoWriteMasterPage();
void DbgMonoWriteLogPath();
void DbgMonoWriteBank();
void DbgMonoWriteBankItem();
void DbgMonoWriteBankPage();
void DbgMonoWriteSlot();
void DbgMonoWriteSlotItem();

int DbgGetKey();

bool DbgAreSlotsSame(DbgBank *pbank);
bool DbgAreFilesSame(DbgBank *pbank);
bool DbgReadConfigToken(FILE *fp, char *buff);
char *DbgSkip(char *p);

#define HALF_DBG_SLOTS (NUM_DBG_SLOTS / 2)

#define DG_DSPNUM (DG_NUM-1)
#define NUM_EDIT_ITEMS (DG_DSPNUM+1)

static short currEntry;
static short currItem;

static short editEntry;
static short editItem;

static short currBank;
static void (*f_writeItem)();
static short numEntries;
static short halfEntries;

static char editEntryBank[NUM_DBG_BANKS];
static char editItemBank[NUM_DBG_BANKS];

static Datapath dpConfig;
static char lastLogFile[14];

static bool loadedConfig;

extern bool exitMsgInstalled;	// has exit message been installed

//	------------------------------------------------------------
//		INIT ROUTINE
//	------------------------------------------------------------
//
//	DbgInit() is needed only to autoload config settings.

void DbgInit()
{
	if (!loadedConfig)
		{
		DatapathAddEnv(&dpConfig, "LIB");
		DatapathAddEnv(&dpConfig, "PROJDIR");
		DbgLoadConfig("libdbg.dbg");
		DbgLoadConfig("projdbg.dbg");
		DbgLoadConfig("debug.dbg");
		loadedConfig = TRUE;
		}

	AtExit(PrintExitMsg);
	exitMsgInstalled = TRUE;

	AtExit(DbgCloseLogFiles);
}

//	------------------------------------------------------------
//		MONO CONFIG ROUTINE
//	------------------------------------------------------------
//
//	DbgMonoConfig() lets operator config system on mono screen.

void DbgMonoConfig()
{
#define MONO_SIZE (80*25*2)
	uchar *p;
	int oldx,oldy;

//	Load config stuff if not already

	if (!loadedConfig)
		{
		DbgInit();
		loadedConfig = TRUE;
		}

//	Save state of old mono screen (SHOULD USE PAGE 2 INSTEAD OF THIS NONSENSE)

	mono_getxy(&oldx,&oldy);
	p = malloc(MONO_SIZE);
	if (p)
		memcpy(p, (uchar *) 0xB0000, MONO_SIZE);

//	Do config screen

	DbgMonoWriteMasterPage();
	DbgMonoHandleInput(0);

//	Clean up, restore old screen

	if (p)
		{
		memcpy((uchar *) 0xB0000, p, MONO_SIZE);
		free(p);
		}
	mono_setxy(oldx,oldy);
}

//	---------------------------------------------------------------
//
//	DbgMonoHandleInput() handles input for mono config screen.
//
//		page = 0 for master page, 1 for bank page

void DbgMonoHandleInput(short page)
{
	short saveEntry;
	short saveItem;
	int c,gate,index;
	ulong slotmask;
	DbgBank *pbank;
	char buff[80];

	while (TRUE)
		{
		c = DbgGetKey();

  		c = toupper(c);

		switch (c)
			{
			case 'L':
				if (page == 0)
					{
					if (DbgMonoReadInput("Load config file: ", 0, 24, buff))
						{
						if (buff[0])
							{
							DbgLoadConfig(buff);
							DbgMonoWriteMasterPage();
							}
						}
					DbgMonoClearInput();
					}
				break;

			case 'S':
				if (page == 0)
					{
					if (DbgMonoReadInput("Save config file: ", 0, 24, buff))
						{
						if (buff[0])
							DbgSaveConfig(buff);
						}
					DbgMonoClearInput();
					}
				break;

			case 'P':
				if (page == 0)
					{
					if (DbgMonoReadInput("New LogPath: ", 0, 24, buff))
						{
						if (buff[0])
							{
							DbgSetLogPath(buff);
							DbgMonoWriteLogPath();
							currItem = DG_DSPNUM;
							for (currEntry = 0; currEntry < NUM_DBG_BANKS;
								currEntry++)
								{
								(*f_writeItem)();
								}
							currEntry = editEntry;
							currItem = editItem;
							}
						}
					DbgMonoClearInput();
					}
				break;

			case KEY_TAB:
				if (page == 0)
					{
					saveEntry = editEntry;
					saveItem = editItem;

					currBank = saveEntry;
					editEntry = editEntryBank[currBank];
					editItem = editItemBank[currBank];

					DbgMonoWriteBankPage();
					DbgMonoHandleInput(1);

					editEntryBank[currBank] = editEntry;
					editItemBank[currBank] = editItem;

					editEntry = saveEntry;
					editItem = saveItem;
					DbgMonoWriteMasterPage();
					}
				else
					return;
				break;

			case KEY_ENTER:
			case KEY_ESC:
				return;

			case ' ':
				if (page == 0)
					{
					pbank = &dbgBank[editEntry];
					if (editItem < DG_DSPNUM)
						{
						if (pbank->gate[editItem])
							pbank->gate[editItem] = 0;
						else
							pbank->gate[editItem] = DBG_SLOT_MASK;
						}
					else
						{
						if (DbgMonoReadInput("LogFile: ", 0, 24, buff))
							{
							strncpy(lastLogFile, buff, sizeof(lastLogFile) - 1);
							DbgSetLogFile(DBGSRC(currEntry,DBG_SLOT_MASK), buff);
							}
						DbgMonoClearInput();
						}
					}
				else
					{
					pbank = &dbgBank[currBank];
					if (editItem < DG_DSPNUM)
						{
						slotmask = 1 << editEntry;
						if (pbank->gate[editItem] & slotmask)
							pbank->gate[editItem] &= ~slotmask;
						else
							pbank->gate[editItem] |= slotmask;
						}
					else
						{
						if (DbgMonoReadInput("LogFile: ", 0, 24, buff))
							{
							strncpy(lastLogFile, buff, sizeof(lastLogFile) - 1);
							DbgSetLogFile(DBGSRC(currBank,1<<editEntry), buff);
							}
						DbgMonoClearInput();
						}
					}
				(*f_writeItem)();
				break;

			case KEY_LEFT:
				if (editItem > 0)
					{
					--editItem; (*f_writeItem)();
					--currItem; (*f_writeItem)();
					}
				else if (editEntry >= halfEntries)
					{
					editEntry -= halfEntries; editItem = NUM_EDIT_ITEMS - 1;
					(*f_writeItem)();
					currEntry -= halfEntries; currItem = NUM_EDIT_ITEMS - 1;
					(*f_writeItem)();
					}
				break;

			case KEY_RIGHT:
				if (editItem < (NUM_EDIT_ITEMS - 1))
					{
					++editItem; (*f_writeItem)();
					++currItem; (*f_writeItem)();
					}
				else if ((editEntry + halfEntries) < numEntries)
					{
					editEntry += halfEntries; editItem = 0;
					(*f_writeItem)();
					currEntry += halfEntries; currItem = 0;
					(*f_writeItem)();
					}
				break;

			case KEY_UP:
				if (--editEntry < 0)
					editEntry = numEntries - 1;
				(*f_writeItem)();
				currEntry = editEntry;
				(*f_writeItem)();
				break;

			case KEY_DOWN:
				if (++editEntry >= numEntries)
					editEntry = 0;
				(*f_writeItem)();
				currEntry = editEntry;
				(*f_writeItem)();
				break;

			case KEY_F5:
				if (editItem == DG_DSPNUM)
					{
					if (page == 0)
						DbgSetLogFile(DBGSRC(currEntry, DBG_SLOT_MASK),
							lastLogFile);
					else
						DbgSetLogFile(DBGSRC(currBank, 1 << editEntry),
							lastLogFile);
					(*f_writeItem)();
					}
				break;

			case KEY_F1:
				if (page == 0)
					{
					pbank = &dbgBank[editEntry];
					if (editItem < DG_DSPNUM)
						{
						if ((pbank->gate[editItem] == DBG_SLOT_MASK) ||
							(pbank->gate[editItem] == 0))
							{
							gate = pbank->gate[editItem];
							saveEntry = currEntry;
							for (currEntry = 0; currEntry < NUM_DBG_BANKS;
								currEntry++)
								{
								dbgBank[currEntry].gate[editItem] = gate;
								(*f_writeItem)();
								}
							currEntry = saveEntry;
							}
						}
					else
						{
						pbank = &dbgBank[currEntry];
						if (DbgAreFilesSame(pbank))
							{
							index = pbank->file_index[0];
							if (index)
								strcpy(buff, dbgLogFile[index].name);
							else
								buff[0] = 0;
							saveEntry = currEntry;
							for (currEntry = 0; currEntry < NUM_DBG_BANKS;
								currEntry++)
									{
									DbgSetLogFile(DBGSRC(currEntry, DBG_SLOT_MASK),
										buff);
									(*f_writeItem)();
									}
							currEntry = saveEntry;
							}
						}
					}
				else
					{
					pbank = &dbgBank[currBank];
					if (editItem < DG_DSPNUM)
						{
						if (pbank->gate[editItem] & (1 << editEntry))
							pbank->gate[editItem] = DBG_SLOT_MASK;
						else
							pbank->gate[editItem] = 0;
						saveEntry = currEntry;
						for (currEntry = 0; currEntry < NUM_DBG_SLOTS;
							currEntry++)
								{
								(*f_writeItem)();
								}
						currEntry = saveEntry;
						}
					else
						{
						pbank = &dbgBank[currBank];
						if (!DbgAreFilesSame(pbank))
							{
							index = pbank->file_index[currEntry];
							if (index)
								strcpy(buff, dbgLogFile[index].name);
							else
								buff[0] = 0;
							DbgSetLogFile(DBGSRC(currBank, DBG_SLOT_MASK), buff);
							saveEntry = currEntry;
							for (currEntry = 0; currEntry < NUM_DBG_BANKS;
								currEntry++)
									{
									(*f_writeItem)();
									}
							currEntry = saveEntry;
							}
						}
					}
				break;
			}
		}
}

//	------------------------------------------------------------
//
//	DbgMonoReadInput() reads a line of input
//
//		prompt = prompt string
//		x      = xpos of prompt
//		y      = ypos of prompt
//		buff   = buffer to fill with chars
//
//	Returns: TRUE if valid input, FALSE if canceled

bool DbgMonoReadInput(char *prompt, short x, short y, char *buff)
{
	int xcurr,ycurr,ibuff,c;

	mono_setxy(x,y);
	mprint(prompt);
	mono_getxy(&xcurr,&ycurr);

	ibuff = 0;

	while (TRUE)
		{
		c = DbgGetKey();
        if (! c)
            continue;

		if (c == 0x08)
			{
			if (ibuff > 0)
				{
				--ibuff;
				mono_setxy(xcurr + ibuff, ycurr);
				mprint(" ");
				*(buff + ibuff) = 0;
				mono_setxy(xcurr, ycurr);
				mprint(buff);
				}
			}
		else if (c == 0x0D)
			{
			*(buff + ibuff) = 0;
			return TRUE;
			}
		else if (c == 0x1B)
			{
			*buff = 0;
			return FALSE;
			}
		else if (xcurr < 80)
			{
			*(buff + ibuff) = c;
			++ibuff;
			*(buff + ibuff) = 0;
			mono_setxy(xcurr, ycurr);
			mprint(buff);
			}
		}
}

//	-------------------------------------------------------------
//
//	DbgMonoClearInput() clears input line.

void DbgMonoClearInput()
{
	char buff[80];

	memset(buff, ' ', 79);
	buff[79] = 0;

	mono_setxy(0,24);
	mprintf(buff);
	mono_setxy(0,24);
}

//	-------------------------------------------------------------
//
//	DbgMonoWriteMasterPage() writes the master page to mono screen.

void DbgMonoWriteMasterPage()
{
	f_writeItem = DbgMonoWriteBankItem;
	numEntries = NUM_DBG_BANKS;
	halfEntries = (numEntries + 1) / 2;

	mono_clear();
	mono_setxy(0,0);
	mprint("DEBUG SYSTEM:  Master Control    LogPath:");
	DbgMonoWriteLogPath();
	mono_setxy(0,2);
	mprint("BANK        DBG MONO FUNC FILE          BANK        DBG MONO FUNC FILE");

	mono_setxy(0,21);
	mprint("ARROWS: select   SPACE: change    F1: set column   F5: last file   TAB: slots");
	mono_setxy(0,22);
	mprint("L: load config   S: save config   P: set logpath   ENTER/ESC: done");

	for (currEntry = 0; currEntry < NUM_DBG_BANKS; currEntry++)
		DbgMonoWriteBank();

	mono_setxy(0,24);

	currEntry = editEntry;
	currItem = editItem;
}

//	------------------------------------------------------------
//
//	DbgMonoWriteLogPath() writes the logpath.

void DbgMonoWriteLogPath()
{
#define LP_XPOS 43
#define LP_WIDTH (79-LP_XPOS)

	char buff[LP_WIDTH + 1];

	memset(buff, ' ', LP_WIDTH);
	buff[LP_WIDTH] = 0;

	mono_setxy(LP_XPOS,0);
	mprint(buff);
	mono_setxy(LP_XPOS,0);
	mprint(dbgLogPath);

	mono_setxy(0,24);
}

//	------------------------------------------------------------
//
//	DbgMonoWriteBank() writes a bank's info to the master page

void DbgMonoWriteBank()
{
	short x,y;

	x = (currEntry < (NUM_DBG_BANKS / 2)) ? 0 : 40;
	y = (currEntry & ((NUM_DBG_BANKS /2) - 1)) + 4;

	mono_setxy(x, y);
	mprintf("%2d ", currEntry);
	if (dbgBank[currEntry].bank_name[0])
		mprintf("%s", dbgBank[currEntry].bank_name);

	for (currItem = 0; currItem < NUM_EDIT_ITEMS; currItem++)
		DbgMonoWriteBankItem();
}

//	------------------------------------------------------------
//
//	DbgMonoWriteBankItem() writes a bank item to the master page.

void DbgMonoWriteBankItem()
{
static short offset[] = {12,16,21,26};
static bool attr;
	short x,y;
	DbgBank *pbank;

	x = (currEntry < (NUM_DBG_BANKS / 2)) ? 0 : 40;
	y = (currEntry & ((NUM_DBG_BANKS /2) - 1)) + 4;

	mono_setxy(x + offset[currItem], y);
	if ((currEntry == editEntry) && (currItem == editItem))
		{
		attr = 1;
		mono_setattr(MA_REV);
		}

	pbank = &dbgBank[currEntry];

	if (currItem < DG_DSPNUM)
		{
		if (pbank->gate[currItem] == DBG_SLOT_MASK)
			mprint(" * ");
		else if (pbank->gate[currItem])
			mprint(" ? ");
		else
			mprint("   ");
		}
	else
		{
		mprint("            ");
		mono_setxy(x + offset[currItem], y);
		if (DbgAreFilesSame(pbank))
			{
			if (pbank->file_index[0])
				mprintf("%s", dbgLogFile[pbank->file_index[0]].name);
			else
				mprint("---");
			}
		else
			mprint("[MULTIPLE]");
		}

	if (attr)
		{
		mono_setattr(MA_NORMAL);
		attr = 0;
		}
	mono_setxy(0,24);
}

//	-----------------------------------------------------------
//
//	DbgMonoWriteBankPage() writes the bank page to the mono screen.

void DbgMonoWriteBankPage()
{
	f_writeItem = DbgMonoWriteSlotItem;
	numEntries = NUM_DBG_SLOTS;
	halfEntries = (numEntries + 1) / 2;

	mono_clear();
	mono_setxy(0,0);
	mprintf("DEBUG SYSTEM:  Bank slots control     Bank: %d  ", currBank);
	if (dbgBank[currBank].bank_name[0])
		mprintf("(%s)", dbgBank[currBank].bank_name);

	mono_setxy(0,2);
	mprint("SLOT        DBG MONO FUNC FILE          SLOT        DBG MONO FUNC FILE");

	mono_setxy(0,21);
	mprint("ARROWS: select   SPACE: change   F1: set column   F5: last file");
	mono_setxy(0,22);
	mprint("ENTER/ESC: done");

	for (currEntry = 0; currEntry < NUM_DBG_SLOTS; currEntry++)
		DbgMonoWriteSlot();

	mono_setxy(0,24);

	currEntry = editEntry;
	currItem = editItem;
}

//	------------------------------------------------------------
//
//	DbgMonoWriteSlot() writes a slot to the bank page.

void DbgMonoWriteSlot()
{
	DbgBank *pbank;
	short x,y;

	x = (currEntry <= HALF_DBG_SLOTS) ? 0 : 40;
	y = (currEntry <= HALF_DBG_SLOTS) ? currEntry + 4 : ((currEntry + 4) - (HALF_DBG_SLOTS + 1));

	pbank = &dbgBank[currBank];
	mono_setxy(x, y);
	mprintf("%2d ", currEntry);
	if (pbank->ppBankSlotNames &&
		(pbank->ppBankSlotNames[currEntry]))
			mprintf("%s", pbank->ppBankSlotNames[currEntry]);
	for (currItem = 0; currItem < NUM_EDIT_ITEMS; currItem++)
		DbgMonoWriteSlotItem();
}

//	-------------------------------------------------------------
//
//	DbgMonoWriteSlotItem() writes a slot item.

void DbgMonoWriteSlotItem()
{
static short offset[] = {12,16,21,26};
static bool attr;
	DbgBank *pbank;
	ulong slotmask;
	short x,y;

	x = (currEntry <= HALF_DBG_SLOTS) ? 0 : 40;
	y = (currEntry <= HALF_DBG_SLOTS) ? currEntry + 4 : ((currEntry + 4) - (HALF_DBG_SLOTS + 1));

	mono_setxy(x + offset[currItem], y);
	if ((currEntry == editEntry) && (currItem == editItem))
		{
		attr = 1;
		mono_setattr(MA_REV);
		}

	pbank = &dbgBank[currBank];
	slotmask = 1 << currEntry;

	if (currItem < DG_DSPNUM)
		{
		if (pbank->gate[currItem] & slotmask)
			mprint(" * ");
		else
			mprint("   ");
		}
	else
		{
		mprint("            ");
		mono_setxy(x + offset[currItem], y);
		if (pbank->file_index[currEntry])
			mprintf("%s", dbgLogFile[pbank->file_index[currEntry]].name);
		else
			mprint("---");
		}

	if (attr)
		{
		mono_setattr(MA_NORMAL);
		attr = 0;
		}
	mono_setxy(0,24);
}

//	------------------------------------------------------------
//		LOAD/SAVE CONFIG
//	------------------------------------------------------------
//
//	DbgAddConfigPath() adds a directory to the path for finding
//	config files.
//
//		path = directory to add
//
//	Returns: TRUE if added, FALSE if not

bool DbgAddConfigPath(char *path)
{
	return(DatapathAdd(&dpConfig, path));
}


//returns true if string is all digits
isnumber(char *p)
{
	for (;*p && isdigit(*p);p++);

	return (*p == 0);
}


//	-----------------------------------------------------------
//
//	DbgLoadConfig() loads config information from a file.
//
//		fname = name of config file
//
//	Returns: 0 if ok, -1 if error

int DbgLoadConfig(char *fname)
{
#define TOKEN_BANKNAME 0
#define TOKEN_SLOTNAME 1
#define TOKEN_LOGPATH 2
#define TOKEN_BANK 3
#define TOKEN_SLOT 4
#define TOKEN_DBG 5
#define TOKEN_MONO 6
#define TOKEN_FUNC 7
#define TOKEN_FILE 8
#define TOKEN_CR 9
#define NUM_TOKENS 10
static char *token[] = {"BANKNAME","SLOTNAME","LOGPATH",
	"BANK","SLOT","DBG","MONO","FUNC","FILE","\n"};

	FILE *fp;
	int i,j;
	int currBank,currSlot;
	ulong slotmask;
	DbgBank *pbank;
	char buff[128];

	fp = DatapathOpen(&dpConfig, fname, "r");
	if (fp == NULL)
		return(-1);

	currBank = currSlot = -1;

	while (TRUE)
		{
		if (!DbgReadConfigToken(fp, buff))
			break;
		for (i = 0; i < NUM_TOKENS; i++)
			{
			if (strcmpi(buff, token[i]) == 0)
				break;
			}
		switch (i)
			{
			case TOKEN_BANKNAME:
				DbgReadConfigToken(fp, buff);
				currBank = atoi(buff);
				DbgReadConfigToken(fp, buff);
				DbgSetBankName(currBank, buff);
				break;

			case TOKEN_SLOTNAME:
				DbgReadConfigToken(fp, buff);
				currSlot = atoi(buff);
				DbgReadConfigToken(fp, buff);
				if (currBank >= 0)
					DbgSetSlotName(currBank, currSlot, buff);
				break;

			case TOKEN_LOGPATH:
				DbgReadConfigToken(fp, buff);
				DbgSetLogPath(buff);
				break;

			case TOKEN_BANK:
				DbgReadConfigToken(fp, buff);
				if (isnumber(buff))
					currBank = atoi(buff);
				else
					currBank = DbgFindBankName(buff);
				if (currBank >= 0)
					{
					currSlot = -1;
					pbank = &dbgBank[currBank];
					for (j = 0; j < DG_NUM; j++)
						pbank->gate[j] = 0;
					DbgSetLogFile(DBGSRC(currBank,DBG_SLOT_MASK), NULL);
					}
				break;

			case TOKEN_SLOT:
				DbgReadConfigToken(fp, buff);
				if (currBank >= 0)
					{
					if (isnumber(buff))
						currSlot = atoi(buff);
					else
						currSlot = DbgFindSlotName(currBank, buff);
					if (currSlot >= 0)
						{
						pbank = &dbgBank[currBank];
						slotmask = 1 << currSlot;
						for (j = 0; j < DG_NUM; j++)
							pbank->gate[j] &= ~slotmask;
						DbgSetLogFile(DBGSRC(currBank,slotmask), NULL);
						}
					}
				break;

			case TOKEN_DBG:
			case TOKEN_MONO:
			case TOKEN_FUNC:
				if (currSlot >= 0)
					dbgBank[currBank].gate[i - TOKEN_DBG] |= (1 << currSlot);
				else if (currBank >= 0)
					dbgBank[currBank].gate[i - TOKEN_DBG] = DBG_SLOT_MASK;
				break;

			case TOKEN_FILE:
				DbgReadConfigToken(fp, buff);
				if (currSlot >= 0)
					DbgSetLogFile(DBGSRC(currBank,1<<currSlot), buff);
				else if (currBank >= 0)
					DbgSetLogFile(DBGSRC(currBank,DBG_SLOT_MASK), buff);
				break;

			case TOKEN_CR:
				break;

			default:
//				printf("error on token: %s\n", buff);
				break;

			}
		}

	fclose(fp);
	return(0);
}

//	------------------------------------------------------------
//
//	DbgSaveConfig() saves config information into a file.
//
//		fname = name of config file
//
//	Returns: 0 if ok, -1 if error

int DbgSaveConfig(char *fname)
{
static char *gateName[] = {"DBG","MONO","FUNC"};
	FILE *fp;
	int bank,slot,gate;
	ulong slotmask;
	DbgBank *pbank;

	fp = fopen(fname, "w");
	if (fp == NULL)
		return(-1);

	fprintf(fp, "//\tDBG config file: %s\n\n", fname);
	fprintf(fp, "LOGPATH %s\n\n", dbgLogPath[0] ? dbgLogPath : ".\\");

	for (bank = 0, pbank = &dbgBank[bank]; bank < NUM_DBG_BANKS;
		bank++, pbank++)
		{
		if (pbank->bank_name[0])
			fprintf(fp, "BANK %s ", pbank->bank_name);
		else
			fprintf(fp, "BANK %d ", bank);
		if (DbgAreSlotsSame(pbank))
			{
			for (gate = 0; gate < DG_DSPNUM; gate++)
				{
				if (pbank->gate[gate])
					fprintf(fp, "%s ", gateName[gate]);
				}
			if (pbank->file_index[0])
				fprintf(fp, "FILE %s",
					dbgLogFile[pbank->file_index[0]].name);
			fprintf(fp, "\n");
			}
		else
			{
			fprintf(fp, "\n");
			for (slot = 0; slot < NUM_DBG_SLOTS; slot++)
				{
				slotmask = 1 << slot;
				if ((pbank->gate[DG_DBG] & slotmask) ||
					(pbank->gate[DG_MONO] & slotmask) ||
					(pbank->gate[DG_FUNC] & slotmask) ||
					(pbank->gate[DG_FILE] & slotmask) ||
					(pbank->file_index[slot]))
						{
						if (pbank->ppBankSlotNames && pbank->ppBankSlotNames[slot])
							fprintf(fp, "\tSLOT %s ", pbank->ppBankSlotNames[slot]);
						else
							fprintf(fp, "\tSLOT %d ", slot);
						for (gate = 0; gate < DG_DSPNUM; gate++)
							{
							if (pbank->gate[gate] & slotmask)
								fprintf(fp, "%s ", gateName[gate]);
							}
						if (pbank->file_index[slot])
							fprintf(fp, "FILE %s",
								dbgLogFile[pbank->file_index[slot]].name);
						fprintf(fp, "\n");
						}
				}
			}
		}

	fclose(fp);
	return(0);
}

//	------------------------------------------------------------------
//		MISCELLANEOUS ROUTINES
//	------------------------------------------------------------------
//
//	DbgAreSlotsSame() checks to see if all slots in a bank are the same.
//
//		pbank = ptr to bank
//
//	Returns: TRUE if all banks same, FALSE if not

bool DbgAreSlotsSame(DbgBank *pbank)
{
	if (!DbgIsDbgSame(pbank))
		return FALSE;
	if (!DbgIsMonoSame(pbank))
		return FALSE;
	if (!DbgIsFuncSame(pbank))
		return FALSE;
	if (!DbgAreFilesSame(pbank))
		return FALSE;

	return TRUE;
}

//	-----------------------------------------------------------------
//
//	DbgAreFilesSame() checks to see if all files in a bank are the same.
//
//		pbank = ptr to bank
//
//	Returns: TRUE if all banks same, FALSE if not

bool DbgAreFilesSame(DbgBank *pbank)
{
	int findex,i;

	findex = pbank->file_index[0];
	for (i = 1; i < NUM_DBG_SLOTS; i++)
		{
		if (pbank->file_index[i] != findex)
			return FALSE;
		}

	return TRUE;
}

//	-----------------------------------------------------------------
//
//	DbgReadConfigToken() reads next token from config file.
//
//		fp   = file ptr
//		buff = token buffer
//
//	Returns: TRUE if valid token, FALSE if end of file

bool DbgReadConfigToken(FILE *fp, char *buff)
{
static char lineBuff[128];
static char *pLineBuff = NULL;
	char *p;

READMORE:

	if (pLineBuff == NULL)
		{
		if (fgets(lineBuff, sizeof(lineBuff), fp) == NULL)
			return FALSE;
		pLineBuff = lineBuff;
		}

	pLineBuff = DbgSkip(pLineBuff);
	if (pLineBuff == NULL)
		goto READMORE;

	for (p = pLineBuff + 1; ; p++)
		{
		if ((*p == 0) || (*p == '\n') || (*p == ' ') || (*p == '\t'))
			break;
		}

	memcpy(buff, pLineBuff, p - pLineBuff);
	buff[p - pLineBuff] = 0;
	pLineBuff = p;
	return TRUE;
}

//	----------------------------------------------------------------
//
//	DbgSkip() skips over whitespace.
//
//		p = ptr into text

char *DbgSkip(char *p)
{
	while ((*p == ' ') || (*p == '\t'))
		p++;
	if (*p == 0)
		return NULL;
	if ((*p == '/') && (*(p+1) == '/'))
		{
		while (*p != '\n')
			p++;
		}
	return(p);
}

#endif

