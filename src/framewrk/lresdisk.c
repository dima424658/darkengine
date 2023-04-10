/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//////////
//
// $Header: r:/t2repos/thief2/src/framewrk/lresdisk.c,v 1.17 1998/11/06 01:47:34 dc Exp $
//

//
// Write and read a block of resource pointers into a memory block.
//
// This module (which replaces the old lresdisk.c) deals with the problem
// of having a bunch of pointers to resources, which we want to be able
// to save and load at a shot. BuildResBlock() returns a chunk of memory
// that represents enough information to be able to reload all of these
// resources; ParseResBlock() takes such a chunk of memory, and actually
// reloads them.
//

//
// For the time being, this code is pretty specific to its purpose, which
// is to save and load texture families. ParseResBlock() assumes this
// implicitly, by always loading the resources as type IMAGE. It also
// does some other wierd stuff for AnimTxt.
//
// Further down the line, this code could be generalized without *too*
// much pain. We already have a reserved byte in tok_name (which was
// originally the "where" field in ResName). This could be used for a
// swizzled type name, into a table of type names up in the front of the
// block. However, this will require biting the bullet and making an
// incompatible save type.
//
// It also assumes that all the paths loaded are relative to the default
// path; reasonable, but not necessarily general.
//

//
// The memory block is identical to that from the old ResName system -- it
// was convenient to use the same format, and it's pretty efficient. It
// has the form:
//
//      int     block_size
//      int     count
//      int     tok_count
//      char    tokens[tok_count][MAX_TOK_LEN]
//      tok_name data[count]
//
// The block_size is the total size of this memory chunk. Count is the
// number of tok_names; tok_count is the number of tokens. Tokens is a
// list of path elements -- for example, in the directory "fam/porttest/",
// "fam" is one token and "porttest" is another. Each tok_name is a
// relative pathname of a resource, with the path reduced to one-byte
// tokens and the filename left intact (that is, a tokenized name).
//

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>
#include <string.h>

#include <resapi.h>
#include <imgrstyp.h>

#include <texmem.h>
#include <lresdisk.h>

#include <anim_txt.h>

#include <dbmem.h>

// Maximum number of distinct tokens:
// We assume that the system isn't going to have a lot of tokens -- that
// the number of distinct path elements will be fairly small.
#define MAX_TOKENS       64 
// Maximum string length per token (that is, per path element):
#define MAX_TOK_LEN      16
// Maximum length of a relative path, in elements:
#define MAX_TOKENS_PER   4
// Maximum length of the real name of a resource:
#define MAX_NAME_LEN     15

//
// "No token" being defined as zero means that we have to index our array
// +1 -- that is, we consider the zero'th element of the array as "1". Not
// the way I'd choose it, but that's how all the old data files have it...
//
#define NO_TOKEN         0

typedef struct _tok_name {
   char tokens[MAX_TOKENS_PER];
   char name[MAX_NAME_LEN];
   char reserved;  // The "where" field from rn_name, which we don't use
} tok_name;

typedef char tokarray[MAX_TOKENS][MAX_TOK_LEN];

#define get_header_sz()       (sizeof(int)*3)
#define get_tokens_sz(tokcnt) ((tokcnt)*MAX_TOK_LEN)
#define get_rnname_sz(cnt)    (sizeof(tok_name)*(cnt))

#define get_header_ptr(base)  ((char *)(base))
#define get_tokens_ptr(base) \
   (get_header_ptr(base)+get_header_sz())
#define get_data_ptr(base,tokencnt,which) \
   (((tok_name *)(get_tokens_ptr(base)+get_tokens_sz(tokencnt)))+which)

#define get_block_size(cnt,tokcnt) \
   (get_header_sz()+get_tokens_sz(tokcnt)+get_rnname_sz(cnt))

#define NULL_STR "null"

//
// Given a single path element, return the token number for it.
//
char ResTokenElement(char *tokname, tokarray tok_strs, int *tok_cnt)
{
   int i;

   // First, check whether the token is already there
   for (i = 0;
        i < *tok_cnt;
        i++)
   {
      if (!strnicmp(tokname, tok_strs[i], MAX_TOK_LEN)) {
         return i+1;
      }
   }

   // Nope; add it in
   strncpy(tok_strs[*tok_cnt], tokname, MAX_TOK_LEN);
   (*tok_cnt)++;

   return *tok_cnt;
}

//
// Clear the given tokenized name.
//
void ResTokEmpty(tok_name *tName)
{
   int tok_num = 0;

   strcpy(tName->name, NULL_STR);

   while (tok_num < MAX_TOKENS_PER) {
      tName->tokens[tok_num++] = NO_TOKEN;
   }
}

//
// Given the relative path to a resource, fill in the tok_name structure
// for it.
//
void ResTokenize(const char *pName,
                 tokarray tok_strs,
                 int *tok_cnt,
                 tok_name *tName)
{
   char pathElem[MAX_TOK_LEN+1];
   char *p;
   int tok_num = 0;
   char tok;
   int i;

   pathElem[0] = '\0';

   while (*pName != '\0') {
      AssertMsg((tok_num < MAX_TOKENS_PER),
                "ResTokenize: too many path elements!");

      // Copy out the next path element...
      p = pathElem;
      i = 0;
      while ((i < MAX_TOK_LEN) &&
             (*pName != '\0') &&
             (*pName != '/') &&
             (*pName != '\\'))
      {
         *(p++) = *(pName++);
         i++;
      }
      *p = '\0';
      if (i == MAX_TOK_LEN) {
         // That went to full length. If we're not at the end of the token,
         // then it's too long for us to save...
         if ((*pName != '\0') &&
             (*pName != '/') &&
             (*pName != '\\'))
         {
            Warning(("Element in path too long to tokenize: %s\n", pathElem));
            ResTokEmpty(tName);
            return;
         }
      }

      if (*pName != '\0') {
         // Now tokenize that path element...
         tok = ResTokenElement(pathElem, tok_strs, tok_cnt);
         tName->tokens[tok_num++] = tok;
         pName++;
         pathElem[0] = '\0';
      }
   }

   if (pathElem[0] == '\0') {
      // The name was empty...
      strcpy(tName->name, NULL_STR);
   } else {
      if (i > MAX_NAME_LEN) {
         Warning(("Name is too long to tokenize: %s\n", pathElem));
         ResTokEmpty(tName);
         return;
      }
      // Copy the actual resource name, not tokenized
      strcpy(tName->name, pathElem);
   }

   while (tok_num < MAX_TOKENS_PER) {
      tName->tokens[tok_num++] = NO_TOKEN;
   }
}

void ResDetokenize(tok_name *tok_name, 
                   char *built_name,
                   tokarray tok_strs,
                   int tok_cnt)
{
   int i;
   BOOL first=TRUE;

   for (i = 0;
        (i < MAX_TOKENS_PER) && (tok_name->tokens[i] != NO_TOKEN);
        i++)
   {
      // We need to guard against bogus tokens; the old system was
      // sometimes bad about this:
      if ((tok_name->tokens[i] < 0) || (tok_name->tokens[i] > MAX_TOKENS)) {
         break;
      }
      if (first) {
         strncpy(built_name, tok_strs[tok_name->tokens[i]-1], MAX_TOK_LEN);
         first = FALSE;
      } else {
         strcat(built_name, "\\");
         strncat(built_name, tok_strs[tok_name->tokens[i]-1], MAX_TOK_LEN);
      }
   }

   if (first) {
      // There wasn't any path
      strcpy(built_name, tok_name->name);
   } else {
      strcat(built_name, "\\");
      strcat(built_name, tok_name->name);
   }
}

//
// Make a block of memory that represents the given resources.
//
void *BuildResBlock(IRes **handles, int cnt)
{
   // The actual strings represented by the tokens; the token is the
   // index of the string in the array.
   tokarray tok_strs;
   int tok_cnt = 0;
   tok_name *names;
   void *blk;
   int blk_size;
   int *header;
   char *blk_ptr;
   int i;

   // Temporary buffer, for building the names in:
   names = (tok_name *) Malloc(sizeof(tok_name) * cnt);

   for (i = 0;
        i < cnt;
        i++)
   {
      if (handles[i])
      {
         // Save the resources under their canonical names:
         char *pName;
         IRes_GetCanonPathName(handles[i], &pName);
         ResTokenize(pName, tok_strs, &tok_cnt, (names + i));
         Free(pName);
      } else {
         // Just ignore it. As it happens, handles[0] is always
         // NULL, due to the way texmem works...
         if (i != 0) {
            Warning(("Saving space for empty texture handle.\n"));
         }
         ResTokenize("", tok_strs, &tok_cnt, (names + i));
      }
   }

   // Okay, at this point we've tokenized all of the resource names.
   blk_size = get_block_size(cnt, tok_cnt);
   blk = Malloc(blk_size);
   if (blk == NULL)
      return NULL;
   header=(int *)get_header_ptr(blk);
   *(header+0)=blk_size;
   *(header+1)=cnt;
   *(header+2)=tok_cnt;

   // Next, write out the token names
   blk_ptr = get_tokens_ptr(blk);
   memcpy(blk_ptr, tok_strs, (tok_cnt * MAX_TOK_LEN));

   // Now, write out the actual names
   blk_ptr = (char *) get_data_ptr(blk, tok_cnt, 0);
   memcpy(blk_ptr, names, (sizeof(tok_name) * cnt));

   Free(names);  // ummm... was there a reason we didnt do this?

   return blk;
}

BOOL ParseResBlock(void *blk, IRes **handles, tStoredResCallback storeCb, tNameCallback nameCb)
{
   int *header;
   int blk_size, cnt, tok_cnt;
   int i;
   tokarray *tok_strs;
   tok_name *cur_name;
   char built_name[256];
   IResMan *pResMan = AppGetObj(IResMan);

   // First, find out how big everything is:
   header   = (int *)get_header_ptr(blk);
   blk_size = *(header+0);
   cnt      = *(header+1);
   tok_cnt  = *(header+2);

   tok_strs = (tokarray *) get_tokens_ptr(blk);

   // Now, run through the tok_names, creating a resource for each one
   // as we go.
   for (i = 0;
        i < cnt;
        i++)
   {
      cur_name = get_data_ptr(blk, tok_cnt, i);
      ResDetokenize(cur_name, built_name, *tok_strs, tok_cnt);
      if (nameCb)
         if (!(*nameCb)(built_name))
         {
            handles[i]=NULL;
            continue;  // return FALSE from name CB to say "dont run"
         }
      handles[i] = IResMan_Lookup(pResMan, built_name, RESTYPE_IMAGE, NULL);
      if (handles[i]==NULL)
      {
         ectsSecretLoadIdx=0;   // ectsHacks - 
         handles[i] = IResMan_Bind(pResMan,
                                   built_name,
                                   RESTYPE_IMAGE,
                                   NULL,
                                   NULL,
                                   0);
         if (handles[i])
         {
            if (storeCb) // ???? - and if not???
               (*storeCb) (handles[i], NULL, NULL);
            if (ectsSecretLoadIdx<1)  // umm. this is bad too
            {
               Warning(("LresDisk: What is up, we loaded but didnt load %s?\n",built_name));
               SafeRelease(handles[i]);  // we dont want this/us/it
               handles[i]=NULL;
            }
            else
            {
               int txt_hnd_used=ectsSecretLoadCopy[ectsSecretLoadIdx-1];
               char dirname[120];
               char *lastslash;

               strcpy(dirname, built_name);
               lastslash=strrchr(dirname, '\\');
               if (lastslash)  // Get the path alone to pass to ectsAnim
                  *(lastslash+1) = '\0';
               ectsAnimTxtCheckLoad(handles[i],TRUE,dirname,txt_hnd_used);
            }
         }
      }
   }
   SafeRelease(pResMan);
   return TRUE;
}
