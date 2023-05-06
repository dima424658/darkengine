#include <kbcook.h>
#include <kbcntry.h>

#define ASCII(x) ((ushort)((x)|CNV_CTRL|CNV_ALT))
#define ALPHA(x) (ASCII(x)|CNV_CAPS)
#define SPECIAL(x) ((ushort)((x)|CNV_SPECIAL|CNV_CTRL|CNV_ALT|CNV_SHIFT))

ushort *kbd_country_tab[];

//global for which country installed
uchar kbd_country = KBC_US;

// turn tables into delta tables
static void kb_make_delta(void)
{
   int i,scan,len;
   uchar cn;
   short *c;
   ushort a;

   // iterate over countries
   for (cn = 1;cn<KBC_CNTRY;++cn) {
      c = kbd_country_tab[cn];
      len = ((int)kbd_country_tab[cn+1] - (int)c)/8;

      for (i=0;i<len;++i) {
         scan = *c;
         a = *(++c);
         *c = a - kb_cnv_table[scan][0];
         a = *(++c);
         *c = a - kb_cnv_table[scan][1];
         a = *(++c);
         *c = a - kb_cnv_table[scan][2];
         ++c;
      }
   }
}

// install a new keyboard
// returns TRUE if successful,
// FALSE if not
bool kb_set_country(uchar country)
{
   int i;
   int scan;
   int len;
   short *c;
   static bool once=FALSE;

   // Init the table if haven't yet
   if (!once) {
      once=TRUE;
      kb_make_delta();
   }

   if (country == kbd_country) return TRUE;
   // last entry
   if (country >= KBC_CNTRY) return FALSE;

   //apply reverse delta if not US
   if (kbd_country != KBC_US) {
      c = kbd_country_tab[kbd_country];
      len = ((int)kbd_country_tab[kbd_country+1] - (int)c)/8;

      for (i=0;i<len;++i) {
         scan = *c;
         kb_cnv_table[scan][0] -= *(++c);
         kb_cnv_table[scan][1] -= *(++c);
         kb_cnv_table[scan][2] -= *(++c);
         ++c;
      }
   }

   kbd_country = country;

   // If US, we're done
   if (country == KBC_US) return TRUE;

   //apply foreign delta table
   c = kbd_country_tab[country];
   len = ((int)kbd_country_tab[country+1] - (int)c)/8;

   for (i=0;i<len;++i) {
      scan = *c;
      kb_cnv_table[scan][0] += *(++c);
      kb_cnv_table[scan][1] += *(++c);
      kb_cnv_table[scan][2] += *(++c);
      ++c;
   }   

   return TRUE;

}


uchar kb_get_country(void)
{
   return kbd_country;
}


// Returns the us ascii equivalent of a given
// key.  For instance, find ASCII('.') or ALPHA('A')
// returns 0 if can't find
char kb_get_us_equivalent(char key)
{
   uchar cntry;
   ushort retval;
   int i,j;

   cntry=kb_get_country();
   if (cntry==KBC_US) return key;

   // Search for the key in the list, remember index
   for (i=0;i<KB_CNV_TBLSIZE;++i) {
      for (j=0;j<3;++j) {
         if (key == (char)kb_cnv_table[i][j])
         {
            // Switch to US and get it again
            kb_set_country(KBC_US);
            retval = (char)kb_cnv_table[i][j];
            // Switch back to original language
            kb_set_country(cntry);
            return retval;
         }
      }
   }
   // Could not find, return 0
   return 0;
}

ushort kbd_fr_tab[][4] = {
   // scan code, ascii, shifted, r-alt
   { 0x1E, ALPHA('q'),  ALPHA('Q'), 0 },
   { 0x32, ASCII(','),  ASCII('?'), 0 },
   { 0x10, ALPHA('a'),  ALPHA('A'), 0 },
   { 0x11, ALPHA('z'),  ALPHA('Z'), 0 },
   { 0x2C, ALPHA('w'),  ALPHA('W'), 0 },
   { 0x0B, ALPHA(0x85), ASCII('0'), ASCII('@') },
   { 0x02, ASCII('&'),  ASCII('1'), 0 },
   { 0x03, ALPHA(0x82), ASCII('2'), ASCII('~') },
   { 0x04, ASCII('"'),  ASCII('3'), ASCII('#') },
   { 0x05, ASCII('\''), ASCII('4'), ASCII('{') },
   { 0x06, ASCII('('),  ASCII('5'), ASCII('[') },
   { 0x07, ASCII('-'),  ASCII('6'), ASCII('|') },
   { 0x08, ALPHA(0x8A), ASCII('7'), ASCII('`') },
   { 0x09, ASCII('_'),  ASCII('8'), ASCII('\\') },
   { 0x0A, ALPHA(0x80), ASCII('9'), ASCII('^') },
   { 0x28, ALPHA(0x97), ASCII('%'), 0 },
   { 0x33, ASCII(';'),  ASCII('.'), 0 },
   { 0x0C, ASCII(')'),  ASCII(0xF8), ASCII(']') },
   { 0x34, ASCII(':'),  ASCII('/'), 0 },
   { 0x35, ASCII('!'),  ASCII(0x15), 0 },
   { 0x27, ALPHA('m'),  ALPHA('M'), 0 },
   { 0x0D, ASCII('='),  ASCII('+'), ASCII('}') },
   { 0x1A, ASCII('^'),  ASCII(0xFA), 0 },
   { 0x1B, ASCII('$'),  ASCII(0x9C), ASCII(0x0F) },
   { 0x29, ASCII(0xFD), ASCII(0xFD), 0 },
   { 0x2B, ASCII('*'), ASCII(0xE6), 0 },
   { 0x56, ASCII('<'), ASCII('>'), 0 }
};

ushort kbd_gr_tab[][4] = {
   { 0x32, ALPHA('m'),  ALPHA('M'), ALPHA(0xE6) },
   { 0x10, ALPHA('q'),  ALPHA('Q'), ASCII('@') },
   { 0x15, ALPHA('z'),  ALPHA('Z'), 0 },
   { 0x2C, ALPHA('y'),  ALPHA('Y'), 0 },
   { 0x0B, ASCII('0'),  ASCII('='), ASCII('}') },
   { 0x03, ASCII('2'),  ASCII('"'), ASCII(0xFD) },
   { 0x04, ASCII('3'),  ASCII(0x15), ASCII(0xFC) },
   { 0x07, ASCII('6'),  ASCII('&'), 0 },
   { 0x08, ASCII('7'),  ASCII('/'), ASCII('{') },
   { 0x09, ASCII('8'),  ASCII('('), ASCII('[') },
   { 0x0A, ASCII('9'),  ASCII(')'), ASCII(']') },
   { 0x28, ALPHA(0x84), ALPHA(0x8E), 0 },
   { 0x33, ASCII(','),  ASCII(';'), 0 },
   { 0x0C, ALPHA(0xE1), ASCII('?'), ASCII('\\') },
   { 0x34, ASCII('.'),  ASCII(':'), 0 },
   { 0x35, ASCII('-'),  ASCII('_'), 0 },
   { 0x27, ALPHA(0x94), ALPHA(0x99), 0 },
   { 0x0D, ASCII('\''), ASCII('`'), 0 },
   { 0x1A, ALPHA(0x81), ALPHA(0x9A), 0 },
   { 0x1B, ASCII('+'),  ASCII('*'), ASCII('~') },
   { 0x29, ASCII('^'),  ASCII(0xF8), 0 },
   { 0x2B, ASCII('#'), ASCII('\''), 0 },
   { 0x56, ASCII('<'), ASCII('>'), ASCII('|') }
};

ushort kbd_uk_tab[][4] = {
   { 0x03, ASCII('2'),  ASCII('"'),  0 },
   { 0x04, ASCII('3'),  ASCII(0x9C), 0 },
   { 0x28, ASCII('\''), ASCII('@'),  0 },
   { 0x2B, ASCII('#'),  ASCII('~'),  0 },
   { 0x56, ASCII('\\'), ASCII('|'),  0 }
};


ushort kbd_cf_tab[][4] = {
   // scan code, ascii, shifted, r-alt
   { 0x32, ALPHA('m'),  ALPHA('M'), ALPHA(0xE6) },
   { 0x18, ALPHA('o'),  ALPHA('O'), ASCII(0x15) },
   { 0x32, ALPHA('p'),  ALPHA('P'), ASCII(0x14) },
   { 0x0B, ASCII('0'),  ASCII(')'), ASCII(0xAC) },
   { 0x03, ASCII('2'),  ASCII('"'), ASCII('@')  },
   { 0x04, ASCII('3'),  ASCII('/'), ASCII(0x9C) },
   { 0x05, ASCII('4'),  ASCII('$'), ASCII(0x9B) },
   { 0x06, ASCII('5'),  ASCII('%'), ASCII(0x0F) },
   { 0x07, ASCII('6'),  ASCII('?'), ASCII(0xAA) },
   { 0x08, ASCII('7'),  ASCII('&'), ASCII('|')  },
   { 0x09, ASCII('8'),  ASCII('*'), ASCII(0xFD) },
   { 0x0A, ASCII('9'),  ASCII(')'), ASCII(0xFC) },
   { 0x28, ASCII('`'),  ASCII('`'), ASCII('{')  },
   { 0x33, ASCII('\''), ASCII('\''),ASCII('_')  },
   { 0x0C, ASCII('-'),  ASCII('_'), ASCII(0xAB) },
   { 0x34, ASCII('.'),  ASCII('.'), ASCII('_')  },
   { 0x35, ASCII('\''), ALPHA(0x90),0           },
   { 0x27, ASCII(';'),  ASCII(':'), ASCII('~')  },
   { 0x0D, ASCII('='),  ASCII('+'), ASCII(0xAD) },
   { 0x1A, ASCII('^'),  ASCII('^'), ASCII('[')  },
   { 0x1B, ASCII(']'),  ASCII('"'), 0           },
   { 0x29, ASCII('#'),  ASCII('|'), ASCII('\\') },
   { 0x2B, ASCII('<'),  ASCII('>'), ASCII('}')  },
   { 0x56, ASCII(0xAE), ASCII(0xAF),ASCII(0xF8) }
};

ushort kbd_it_tab[][4] = {
   { 0x0B, ASCII('0'),  ASCII('='), 0 },
   { 0x03, ASCII('2'),  ASCII('"'), 0 },
   { 0x04, ASCII('3'),  ASCII(0x9C), 0 },
   { 0x07, ASCII('6'),  ASCII('&'), 0 },
   { 0x08, ASCII('7'),  ASCII('/'), 0 },
   { 0x09, ASCII('8'),  ASCII('('), 0 },
   { 0x0A, ASCII('9'),  ASCII(')'), 0 },
   { 0x28, ALPHA(0x86), ALPHA(0xF8), ASCII('#') },
   { 0x33, ASCII(','),  ASCII(';'), 0 },
   { 0x0C, ASCII('\''), ASCII('?'), 0 },
   { 0x34, ASCII('.'),  ASCII(':'), 0 },
   { 0x35, ASCII('-'),  ASCII('_'), 0 },
   { 0x27, ALPHA(0x95), ALPHA(0x80), ASCII('@') },
   { 0x0D, ALPHA(0x8D), ASCII('^'), 0 },
   { 0x1A, ALPHA(0x8A), ALPHA(0x82), ASCII('[') },
   { 0x1B, ASCII('+'),  ASCII('*'), ASCII(']') },
   { 0x29, ASCII('\\'),  ASCII('|'), 0 },
   { 0x2B, ALPHA(0x97), ALPHA(0x15), 0 },
   { 0x56, ASCII('<'), ASCII('>'), 0 }
};

ushort kbd_sp_tab[][4] = {
   { 0x0B, ASCII('0'),  ASCII('='), 0 },
   { 0x02, ASCII('1'),  ASCII('!'), ASCII('!') },
   { 0x03, ASCII('2'),  ASCII('"'), ASCII('@') },
   { 0x04, ASCII('3'),  ASCII(0xF9), ASCII('#') },
   { 0x07, ASCII('6'),  ASCII('&'), ASCII('0xAA') },
   { 0x08, ASCII('7'),  ASCII('/'), 0 },
   { 0x09, ASCII('8'),  ASCII('('), 0 },
   { 0x0A, ASCII('9'),  ASCII(')'), 0 },
   { 0x28, ASCII('\''),  ASCII('"'), ASCII('{') },
   { 0x33, ASCII(','),  ASCII(';'), 0 },
   { 0x0C, ASCII('\''), ASCII('?'), 0 },
   { 0x34, ASCII('.'),  ASCII(':'), 0 },
   { 0x35, ASCII('-'),  ASCII('_'), 0 },
   { 0x27, ALPHA(0xA4), ALPHA(0xA5), 0 },
   { 0x0D, ALPHA(0xAD), ALPHA(0xA8), 0 },
   { 0x1A, ASCII('`'),  ASCII('^'), ASCII('[') },
   { 0x1B, ASCII('+'),  ASCII('*'), ASCII(']') },
   { 0x29, ASCII('<'),  ASCII('>'), 0 },
   { 0x2B, ASCII('}'), ALPHA(0x80), 0 },
   { 0x56, ASCII('<'), ASCII('>'), 0 }
};

ushort kbd_end_tab[][4] = {
   { 0x29, ASCII(0xFD), ASCII(0xFD), 0 }
};


//Table of pointers
ushort *kbd_country_tab[] = {
   NULL,
   kbd_fr_tab[0],
   kbd_gr_tab[0],
   kbd_uk_tab[0],
   kbd_cf_tab[0],
   kbd_it_tab[0],
   kbd_sp_tab[0],
   kbd_end_tab[0]
};

