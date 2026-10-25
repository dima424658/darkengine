/* 
 * $Source: x:/prj/tech/libsrc/input/RCS/joystick.h $
 * $Revision: 1.19 $
 * $Author: DAVET $
 * $Date: 1996/01/25 14:20:09 $
 *
 *	Header for joystick "library"
 *
 * $Log: joystick.h $
 * Revision 1.19  1996/01/25  14:20:09  DAVET
 * Added cplusplus stuff
 * 
 * Revision 1.18  1996/01/19  10:01:17  JACOBSON
 * Doug's Windoze stuff.
 * 
 * Revision 1.15  1995/06/13  16:05:27  JAK
 * removed proto
 * 
 * Revision 1.14  1995/06/06  12:15:27  JAK
 * make thrustmaster_middles[] public by
 * putting it in joystick.h
 * 
 * Revision 1.13  1995/06/05  14:12:47  JAK
 * *** empty log message ***
 * 
 * Revision 1.12  1995/05/05  00:06:30  jak
 * added dead zone setting
 * ,.
 * 
 * Revision 1.11  1995/05/01  20:03:02  jak
 * bypass for bios joystick reading
 * 
 * Revision 1.10  1995/05/01  17:21:56  jak
 * added bios reading
 * 
 * Revision 1.9  1995/04/18  22:01:29  jak
 * renamed vars to joyd_range and joyd_center, and
 * externed them.
 * 
 * Revision 1.8  1995/04/13  16:34:01  jak
 * new old-style calibration system.
 * 
 * Revision 1.7  1995/04/07  12:00:53  jak
 * added autorange option to joy_calibrate()
 * 
 * Revision 1.6  1995/02/23  18:36:11  jak
 * better codes and defines
 * externed "joy_type"
 * 
 * Revision 1.5  1995/02/23  04:53:16  jak
 * I am too tired to remember what I changed.
 * 
 * Revision 1.4  1995/02/22  00:18:23  jak
 * new calibration
 * much cleaning
 * 
 * Revision 1.3  1994/11/07  20:46:00  dc
 * defines for stick types, ngp or not, hat positions
 * 
 * Revision 1.2  1994/01/20  14:33:42  jak
 * New calibration routines.
 * 
 * Revision 1.1  1993/03/29  15:58:13  matt
 * Initial revision
 * 
 * 
 */


#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


short joy_init( int joy_type );  // returns number of active pots
uchar joy_read_buttons( void );	// one bit per button
void  joy_read_pots( char * );	// fills in up to 4 pots
void joy_read_pots_raw( short *raw );
void  joy_center( void );
void joy_guess_ranges(void);
extern int joy_dead_zone_percent;

// Take the current value of the pot as its center, and guess the range.
// If the device is not an auto-center device (e.g. throttle)
// then guess at the just the range
void joy_calibrate_start( void );

// Take a reading of this pot and extend the current range to include this value
// (Call when pot is at extreme to set that value as the extreme value)
void joy_calibrate( int pot );

// Thrustmaster-type hat autocalibration
void hatcal_init( void );
void hatcal_calib( void );
extern short thrustmaster_middles[4];

// Re-guess at default ranges for pots that weren't calibrated
void joy_calibrate_end( void );

// Do we try using the bios if all else fails?
extern bool joy_try_bios;

// Last read raw values
extern short joy_raw[4];
extern bool joy_dead[4];
extern bool joy_mask[4];
extern ulong joy_type;

// Calibration values
extern short joyd_range[4][2];
extern short joyd_center[4];

// How are we reading the pots?
extern uchar joy_readmethod;

// Hardward config codes
#define JOY_MASK      0xF    // historical
#define JOY_TYPE_MASK 0xF
#define JOY_NORM      0
#define JOY_FLPRO     1
#define JOY_THRUST    2
#define JOY_NONE      3
#define JOY_NO_NGP    0x80

// How are we reading the pots?
#define JOY_READMETHOD_BASIC (0)
#define JOY_READMETHOD_NGP   (1)
#define JOY_READMETHOD_BIOS  (2)
#define JOY_READMETHOD_NONE  (100)

// Axis codes
#define JOY_AXIS_X        (0)
#define JOY_AXIS_Y        (1)
#define JOY_AXIS_RUDDER   (2)
#define JOY_AXIS_THROTTLE (3)

// Button value codes
#define JOY_BUTTON_MASK 0xF
#define JOY_HAT_MASK    0x70
#define JOY_HAT_NULL    0x00
#define JOY_HAT_N       0x10
#define JOY_HAT_E       0x20
#define JOY_HAT_S       0x30
#define JOY_HAT_W       0x40

#ifdef __cplusplus
}
#endif  // cplusplus

#endif /* !__JOYSTICK_H */
