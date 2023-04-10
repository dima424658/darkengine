

/* $Source: s:/prj/tech/libsrc/font/RCS/strwrap.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:26:03 $
 * 
 *  String wrapping and unwrapping routines.
 *
 *  This file is part of the font library.
 */

#include <chr.h>
#include <fonts.h>

static short *pCharPixOff;		// ptr to char offset table, with pfont->minch
					// already subtracted out!

#define CHARALIGN(pfont,c) (pCharPixOff[(uchar)c] & 7)
#define CHARPTR(pfont,c) (&pfont->bits[pCharPixOff[(uchar)c] >> 3])
#define CHARWIDTH(pfont,c) (pCharPixOff[(uchar)c+1] - pCharPixOff[(uchar)c])

#define FONT_SETFONT(pfont) (pCharPixOff = &(pfont)->off_tab[0] - (pfont)->min)

//	----------------------------------------------------
//
//	FontWrapText() inserts wrapping codes into text.
//	It inserts soft carriage returns into the text, and
//	returns the number of lines needed for display.
//
//		pfont = ptr to font
//		ps    = ptr to string (soft cr's and soft spaces inserted into it)
//		width = width of area to wrap into, in pixels
//
//	Returns: # lines string wraps into

/* renamed to gr_font_string_wrap in font library */

int gr_font_string_wrap (grs_font *pfont, char *ps, short width)
{
	uchar *p;
	char *pmark;
	short numLines;
	short currWidth;

//	Set up to do wrapping

	FONT_SETFONT(pfont);
	numLines = 0;					// ps = base of current line

//	Do wrapping for each line till hit end

	while (*ps)
		{
		pmark = NULL;				// no SOFTCR insert point yet
		currWidth = 0;				// and zero width so far
		p = (uchar *) ps;

//	Loop thru each word

		while (*p)
			{

//	Skip through to next CR or space or '\0', keeping track of width

			while ((*p != 0) && (*p != '\n') && (*p != ' '))
				{
				currWidth += CHARWIDTH(pfont, *p);
				p++;
				}

//         mprintf("w %d in %d..",currWidth,p-ps);

//	If bypassed width, break out of word loop

			if (currWidth > width)
			{
//            mprintf("past pmark %p p <%c>\n",pmark,*p);
				if ((pmark == NULL) && (*p != 0) && (*p != '\n'))
					pmark = p;
				break;
			}

//	Else set new mark point (unless eol or eos, then bust out)

			else
			{
				if ((*p == 0) || (*p == '\n'))	// hit end of line, wipe marker
				{
					pmark = NULL;
					break;
				}
				pmark = p;									// else advance marker
				currWidth += CHARWIDTH(pfont, ' ');	// and account for space
				p++;
//            mprintf("New mark %p wid %d dis %d\n",pmark,currWidth,p-ps);
			}
		}

//	Now insert soft cr if marked one, unless it's last char that's marked

//#define OLD_WAY
#ifdef OLD_WAY
      if (pmark && (pmark != (p - 1)))
      {
#else
      if (pmark)     // && (pmark != (p - 1)))
		{           // maybe && (*pmark-1!=CHAR_SOFTCR?)
         if ((pmark==(p-1))&&((*p)==0))   
         {           // softspot is last char, and we are at EOS
            ps=p-1;  // so back up to it 
            *(ps)=CHAR_SOFTSP; // and toast it (then fall through and exit while)
         }   
         else        // put a real SOFTCR in
#endif
         {
				*pmark = CHAR_SOFTCR;
				ps = pmark + 1;
//	         mprintf("Try insert soft CR at %p, a <%c>, p @ %p, a <%c> %d\n",ps,*ps,p,*p,*p);
				if (*ps == ' ')			// if wrapped and following space,
					*ps++ = CHAR_SOFTSP;	// turn into (ignored) soft space
         }
		}  //	Otherwise, bump past cr
		else
		{
			if (*p)
				++p;
			ps = p;   // ps points at next line
//         mprintf("Moving a line to %p\n",ps);
		}

//	Bump line counter in any case

		++numLines;
		}

//	When hit end of string, return # lines encountered
//   mprintf("NL is %d\n",numLines);
	return numLines;
}

//	--------------------------------------------------------
//
//	FontUnwrapText() turns soft carriage returns back into
//	spaces.  Usually this is done prior to re-wrapping
//	text with a new width.
//
//		s = ptr to string (soft cr's and spaces turned back to spaces)

/* renamed to gr_font_string_unwrap in font library */

void gr_font_string_unwrap (char *s)
{
	int c;

	while ((c = *s) != 0)
		{
		if ((c == CHAR_SOFTCR) || (c == CHAR_SOFTSP))
			*s = ' ';
		s++;
		}
}



