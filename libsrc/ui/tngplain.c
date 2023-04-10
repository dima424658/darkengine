#include <tngplain.h>
#include <_ui.h>

bool tng_plain_keycooked(TNG *ptng, ushort key)
{
   bool retval = FALSE;
   IF_SET_RV(tng_cb_keycooked(ptng, key));
   return(retval);
}

bool tng_plain_mousebutt(TNG *ptng, uchar type, Point loc)
{
   tng_cb_mousebutt(ptng,type,loc);
   return(TRUE);
}

bool tng_plain_signal(TNG *ptng, ushort signal)
{
   tng_cb_signal(ptng,signal);
   return(TRUE);
}

// Initializes the TNG
errtype tng_plain_init(void *ui_data, TNG *ptng, Point size)
{
   TNG_plain *ppbtng;

   ppbtng = (TNG_plain *)GUI_MALLOC(ptng->ui_data, sizeof(TNG_plain));

   TNGInit(ptng,NULL,ui_data);
   ptng->type_data = ppbtng;
   ptng->keycooked = &tng_plain_keycooked;
   ptng->mousebutt = &tng_plain_mousebutt;
   ptng->signal = &tng_plain_signal;

   ppbtng->tng_data = ptng;
   ppbtng->size = size;
   return(OK);
}

// Deallocate all memory used by the TNG 
errtype tng_plain_destroy(TNG *ptng)
{
   GUI_DEALLOC(ptng->ui_data, ptng->type_data);
   return(OK);
}

// Fill in ppt with the size...
errtype tng_plain_size(TNG *ptng, Point *ppt)
{
   *ppt = TNG_PL(ptng)->size;
   return(OK);
}

// Returns the current "value" of the TNG
int tng_plain_getvalue(TNG *ptng)
{
   void *dummy;   dummy = ptng;
   return(0);
}


