// $Header: x:/prj/tech/libsrc/ui/RCS/hotkey.c 1.17 1997/06/15 21:08:57 TOML Exp $

#define __HOTKEY_SRC
#include <hotkey.h> 
#include <hash.h>

#ifdef HOTKEY_HELP
#include <string.h>
#endif

#define CHAIN_LENGTH 2
#define CHAIN_END -1

ulong HotkeyContext = 0xFFFFFFFF;
static hotkey_callback hotkey_shutdown_callback;


int hotkey_hash_func(void* v)
{
   hotkey_entry* e = (hotkey_entry*)v;
   return e->key;
}

int hotkey_equ_func(void* v1, void* v2)
{
   return ((hotkey_entry*)v1)->key - ((hotkey_entry*)v2)->key;
}


errtype hotkey_init(int tblsize)
{
   return hash_init(&hotkey_table,sizeof(hotkey_entry),tblsize,hotkey_hash_func,hotkey_equ_func);
}

errtype hotkey_add(short keycode, ulong contexts, hotkey_callback func, void* state)
{
#ifdef HOTKEY_HELP
   return(hotkey_add_help(keycode,contexts,func,state,NULL));
}

errtype hotkey_add_help(short keycode, ulong contexts, hotkey_callback func, void* state, char *help_text)
{
#endif
   hotkey_entry e,*ch;
   errtype err;
   int i;
   hotkey_link *chain;
   e.key = keycode;
   err = hash_lookup(&hotkey_table,&e,&ch);
   if (err != OK) return err;
   if (ch == NULL)
   {
      Spew(DSRC_UI_Hotkey,("Creating new hotkey chain\n"));
      err = hash_insert(&hotkey_table,&e);
      if (err != OK) return err;
      hash_lookup(&hotkey_table,&e,&ch);
      array_init(&ch->keychain,sizeof(hotkey_link),CHAIN_LENGTH);
      ch->first = CHAIN_END;
   }
   err = array_newelem(&ch->keychain,&i);
   if (err != OK) return err;
   chain = (hotkey_link*)ch->keychain.vec;
   chain[i].context = contexts;
   chain[i].func = func;
   chain[i].state = state;
#ifdef HOTKEY_HELP
   if (help_text)
   {
      chain[i].help_text = Malloc(strlen(help_text)+1);
      strcpy(chain[i].help_text,help_text);
   }
   else
   {
      chain[i].help_text = NULL;
   }
#endif
   chain[i].next = ch->first;
   ch->first = i;
   return OK;
}

#ifdef HOTKEY_HELP
char *hotkey_help_text(short keycode, ulong contexts, hotkey_callback func)
{
   hotkey_entry *ch;
   errtype err;
   int i;
   hotkey_link *chain;
   err = hash_lookup(&hotkey_table,(hotkey_entry*)&keycode,&ch);
   if (err != OK) return(NULL) ;
   if (ch == NULL) return(NULL);
   chain = (hotkey_link*)ch->keychain.vec;
   for (i = ch->first; chain[i].func == func;)
   {
      chain[i].context &= ~contexts;
      if (chain[i].context == 0)
      {
            return(chain[i].help_text);
      }
   }
   for(i = ch->first; chain[i].next != CHAIN_END; i = chain[i].next)
   {
      int n = chain[i].next;
      if (chain[n].func == func)
      {
         chain[n].context &= ~contexts;
         if (chain[n].context == 0)
         {
            return(chain[n].help_text);
         }
      }
   }
   return(NULL);
}
#endif

errtype hotkey_remove(short keycode, ulong contexts, hotkey_callback func)
{
   hotkey_entry *ch;
   errtype err;
   int i;
   hotkey_link *chain;
   err = hash_lookup(&hotkey_table,(hotkey_entry*)&keycode,&ch);
   if (err != OK) return err;
   err = ERR_NOEFFECT;
   if (ch == NULL) return err;
   chain = (hotkey_link*)ch->keychain.vec;

   for (i = ch->first; i != CHAIN_END && chain[i].func == func;)
   {
      chain[i].context &= ~contexts;
      if (chain[i].context == 0)
      {
         ch->first = chain[i].next;
#ifdef HOTKEY_HELP
         if (chain[i].help_text)
            Free(chain[i].help_text);
#endif // HOTKEY_HELP
         array_dropelem(&ch->keychain,i);
         i = ch->first;
         err = OK;
      } else
         break; // if we don't update loop var, then break out!
   }
   if (i == CHAIN_END) return err;

   for(i = ch->first; chain[i].next != CHAIN_END; i = chain[i].next)
   {
      int n = chain[i].next;
      if (chain[n].func == func)
      {
         chain[n].context &= ~contexts;
         if (chain[n].context == 0)
         {
            chain[i].next = chain[n].next;
#ifdef HOTKEY_HELP
            if (chain[i].help_text)
               Free(chain[i].help_text);
#endif // HOTKEY_HELP
            array_dropelem(&ch->keychain,n);
            err = OK;
         }
      }
   }
   return err;
}

errtype hotkey_replace(short keycode,
    ulong old_contexts, hotkey_callback old_func, void *old_state,
    ulong new_contexts, hotkey_callback new_func, void *new_state)
{
   hotkey_entry *ch;
   errtype err;
   int i;
   hotkey_link *chain;
   err = hash_lookup(&hotkey_table,(hotkey_entry*)&keycode,&ch);
   if (err != OK) return err;
   if (ch == NULL) return ERR_NOEFFECT;
   chain = (hotkey_link*)ch->keychain.vec;
   for (i = ch->first; i != CHAIN_END; i = chain[i].next)
   {
      if (chain[i].func == old_func &&
          chain[i].context == old_contexts &&
          chain[i].state == old_state)
      {
          chain[i].func = new_func;
          chain[i].context = new_contexts;
          chain[i].state = new_state;
          return OK;
      }
   }
   return ERR_NOEFFECT;
}

errtype hotkey_dispatch(short keycode)
{
   hotkey_entry *ch;
   errtype err;
   int i;
   hotkey_link *chain;
   err = hash_lookup(&hotkey_table,(hotkey_entry*)&keycode,&ch);
   if (err != OK) return err;
   if (ch == NULL) return ERR_NOEFFECT;
   chain = (hotkey_link*)ch->keychain.vec;
   for (i = ch->first; i != CHAIN_END; i = chain[i].next)
   {
      Spew(DSRC_UI_Hotkey,("checking link %d \n",i));
      if (chain[i].context & HotkeyContext)
      {
         Spew(DSRC_UI_Hotkey,("Succeeded context test %d\n",chain[i].context));
         if (chain[i].func(keycode,HotkeyContext,chain[i].state))
            return OK;
      }
   }
   return ERR_NOEFFECT;
}

void hotkey_set_shutdown_callback(hotkey_callback func)
{
    hotkey_shutdown_callback = func;
}

static bool shutdown_iter_func(void* elem, void* data)
{
   int i;
   hotkey_entry* ch = (hotkey_entry*)elem;
   hotkey_link *chain = (hotkey_link*)(ch->keychain.vec);
#ifndef NO_DUMMIES
   void *dummy = data;
#endif // NO_DUMMIES

   if (ch == NULL) return FALSE;
   for (i = ch->first; i != CHAIN_END; i = chain[i].next)
   {
      if (hotkey_shutdown_callback)
         hotkey_shutdown_callback(ch->key, chain[i].context, chain[i].state);
#ifdef HOTKEY_HELP
      if (chain[i].help_text)
         Free(chain[i].help_text);
#endif // HOTKEY_HELP
      chain[i].help_text = NULL;
      chain[i].state = NULL;
   }

#ifndef NO_DUMMIES
   data = dummy;
#endif // NO_DUMMIES
   array_destroy(&ch->keychain);
   return FALSE;
}

errtype hotkey_shutdown(void)
{
   hash_iter(&hotkey_table,shutdown_iter_func,NULL);
   hash_destroy(&hotkey_table);
   return OK;
}

int list_index = 0;

#ifdef GODDAMN_THIS_MESS_IS_IMPOSSIBLE
bool hotkey_list(char **item, int sort_type)
{
   void *res;
   hotkey_entry* ch;
   hotkey_link *chain;
   int i;

   hash_step(&hotkey_table, &res, &list_index);
   ch = (hotkey_entry *)res;
   if (ch == NULL) return ERR_NOEFFECT;
   chain = (hotkey_link*)ch->keychain.vec;
   strcpy(*item, "");
   for (i = ch->first; i != CHAIN_END; i = chain[i].next)
   {
      strcat(*item, 
      if (chain[i].context & HotkeyContext)
      {
         Spew(DSRC_UI_Hotkey,("Succeeded context test %d\n",chain[i].context));
         if (chain[i].func(keycode,HotkeyContext,chain[i].state))
            return OK;
      }
   }
   strcpy(*item, 
}

errtype hotkey_list_clear()
{
   list_index = 0;
}

#endif

   

