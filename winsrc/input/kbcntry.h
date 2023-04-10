
#ifndef __KBCNTRY_H 
#define __KBCNTRY_H 
#ifdef __cplusplus
extern "C"  {
#endif  

enum {
   KBC_US,
   KBC_FR,
   KBC_GR,
   KBC_UK,
   KBC_CF,
   KBC_IT,
   KBC_SP,
   KBC_CNTRY
};

bool kb_set_country(uchar country);
uchar kb_get_country(void);

// Returns the us ascii equivalent of a given
// key.  
char kb_get_us_equivalent(char key);

#ifdef __cplusplus
}
#endif  
#endif
