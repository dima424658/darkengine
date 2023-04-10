/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamespec.h,v 1.7 2000/01/29 13:20:59 adurant Exp $
// game specific rendering code header
#pragma once

// how do we do game specific stuff

// give game chance to intercept obj rendering/so forth
void gamespec_init_object_rend(void);

// set the player that subsequent gamespec calls can refer to
void gamespec_set_player(int playerobj);
// parse a key, returns if we ate it
bool gamespec_key_parse(int keycode);
// called every frame
void gamespec_update_frame(int ms);
// grs_canvas will be the current screen
void gamespec_hud(void);

// these return whether the object is still with us
// damage obj p by data
bool gamespec_damage(int damagee, int damager, int data);
// kill off an object
bool gamespec_kill(int killee, int data);

// tmap is really a rhnd, type is the model type
int gamespec_model_texture_swap(int tmap, int type);

// system/object messages
void gamespec_sysmsg(int priority, char *msg);
void gamespec_objmsg(int obj1, int obj2, int data);
void gamespec_talkmsg(int obj1, char *txt);
