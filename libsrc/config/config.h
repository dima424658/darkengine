
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * $Source: x:/prj/tech/libsrc/config/RCS/config.h $
 * $Revision: 1.18 $
 * $Author: dc $
 * $Date: 1997/09/01 21:52:52 $
 *
 */

// Includes
#include <lg.h>  // every file should have this
#include <error.h>

// -------------
// CONFIG SYSTEM
// -------------

/*
The config system parses configuration files and command-line
arguments into a set of variables and values.   Each variable has a
string name of up to 50 characters, containing exclusively non-space
characters.  A value is a string token, an integer, or a list of
strings or integers separated by whitespace or commas.  Config files
have the following format:

<var1> <value1>
<var2> <value2>
<var3> <value3>
.
.
.
<varN> <valueN>

Pretty simple, eh?

In order to be parsed by the config system, command line arguments must either
be switches or assignments.  A switch is a single character preceded by '/'
(e.g. "/a" or "/s").  Multiple switches can be concatenated to a string of
characters following a single '/' (e.g. "/as").  The precise effect of a switch
depends on the application, but developers are encouraged to adhere to the convention
a switch simply defines or undefines a configuration variable.   Some switches behave
like variables and take an argument (e.g. "-n Harold" or "-nHarold", either works)
See config_parse_commandline() for how to define the effect of switches.
An assignment has the following syntax <varname>=<value>.  It sets the variable
named to the value specified.  An assignment MUST BE A SINGLE ARGUMENT on the
commandline.  If it contains spaces, it should be enclosed in quotes.  Assigning a variable
to the empty string undefines it.


*/

// MAHK 6/27  The config system now understands comments in config files. ';' is the comment character.
//            Also, the system is now case-insensitive


// MAHK 10/24/94  The command-line parse now understands the syntax +<var>
//                Which simply defines <var>

// THIS LIBRARY DEPENDS ON THE FOLLOWING LIBRARIES: DSTRUCT LG

// Defines


#ifdef __cplusplus
extern "C"  {
#endif


#define VARNAME_LEN 50
#define CONFIG_DFT_LO_PRI 3
#define CONFIG_DFT_HI_PRI 6
#define CONFIG_DFT_LIB_PRI CONFIG_DFT_LO_PRI
#define CONFIG_DFT_APP_PRI CONFIG_DFT_HI_PRI

#define CONFIG_STRING_TYPE 0
#define CONFIG_INT_TYPE    1
#define CONFIG_FLOAT_TYPE  2

#define isSwitch(x) (((x) == '/') || ((x) == '-'))


// Note that a config value is either an int, float, or a char*, here it is defined as a void[]
// so it can be a scalar
typedef void* config_valtype;
typedef void* config_valptr; // a pointer to a valtype.


// Prototypes

errtype config_init(void);
// Initializes the config system. returns ERR_NOEFFECT if the config system
// has already been initialized.

errtype config_shutdown(void);
// shuts down the config system, purging all data.


//---------------------------------------------------
// File operations

errtype config_read_file(const char* file,uint (*readfunc)(char* var));
// reads in the configuration variables stored in a specified file.
// readfunc is a function mapping variable names onto integer priorities.
// A given variable is loaded from the file only if readfunc specifies
// a greater priority than has already been set for that variable.
// If the readfunc specifies priority zero for a variable, then that
// variable is never read in from the file.

typedef bool (*writefunc)(char* filename, char* var);

errtype config_write_file(const char* file,writefunc write);
// Writes out all writable configuration variables to the specified filename.
// If the file already exists, then non-writable variables and comments in the file will be preserved
// <write> is a predicate that determines what set of variables
// is "writeable."  A variable will be written out to the config file
// iff <write> returns true.

bool config_write_to_same_file( char* filename, char* var );
// Intended for use with config_write_file.
// returns TRUE iff filename matches the origin of var.

//--------------------------------------------------
// Stream operations

void config_add_to_or_extract_from_recording( void );
// save or reload the entire state of the config system to/from the
// recording stream, depending on the recording mode.  no-op if the
// recorder is inactive

//--------------------------------------------------
// Set and get methods for variables

bool config_get_value(const char* varname, int type, config_valptr fillvec, int* cnt);
// if varname has been defined, fill in the array pointed to by fillvec with
// string, float or integer tokens.  Tokens are separated by commas or spaces.
// Sets the cnt to the number of items placed in fillvec.
// Returns true iff the variable is defined.  If a positive value if *cnt is passed in, then
// only the first *cnt elements will be retrieved.
// When the token type is CONFIG_STRING_TYPE, the vector is filled with
// pointers to memory allocated using Malloc();

errtype config_set_value(const char* varname, int type, config_valptr fillvec, int cnt);
// sets the value of varname to a value of the specified type

errtype config_set_single_value(const char* varname, int type, config_valtype value);
// sets the value of varname to a single element of the specified type

bool config_get_single_value(const char* varname, int type, config_valptr value);
// Like config_get_value, except only gets the first available value.
// returns true iff the variable was defined and had at least one value.
// When the type is CONFIG_STRING_TYPE, space for the string value is allocated
// using Malloc();

bool config_get_raw(const char* varname, char* buf, int bufsize);
// Copies up to bufsize characters of the raw, uninterpreted value of the
// named variable into buf, or returns false iff the variable is
// undefined.

typedef bool (*ConfigIterFunc)(char *var);
void config_get_raw_all(ConfigIterFunc ifunc, char* buf, int bufsize);
// For every config variable <var>, copies the first bufsize
// bytes of the raw value into buf, and then calls ifunc(var);
// stops when it's called with all vars or as soon as ifunc
// returns TRUE.  The order is arbitrary (not sorted).

// Useful macros

#define config_set_int(varname,val)      config_set_single_value(varname,CONFIG_INT_TYPE,(config_valtype)(val))
#define config_set_string(varname,val)   config_set_single_value(varname,CONFIG_STRING_TYPE,(config_valtype)(val))
#define config_set_float(varname,val)    config_set_single_value(varname,CONFIG_FLOAT_TYPE,(config_valtype)(val))
#define config_get_int(varname,valptr)   config_get_single_value(varname,CONFIG_INT_TYPE,(config_valtype)(valptr))
#define config_get_word(varname,valptr)  config_get_single_value(varname,CONFIG_STRING_TYPE,(config_valtype)(valptr))
#define config_get_float(varname,valptr) config_get_single_value(varname,CONFIG_FLOAT_TYPE,(config_valtype)(valptr))

// since config_set_float, im pretty sure, doesnt work
// this requires you pass in the var in a fltvar, since it has to do type idiocy to get it
#define config_set_float_from_var(varname,fltvar) \
        config_set_single_value(varname,CONFIG_FLOAT_TYPE,(config_valtype)(*(void **)(&fltvar)))

#define config_is_defined(var) config_get_raw(var,NULL,0)

errtype config_unset(const char* varname);
// undefines the specified variable.

#define PRIORITY_TRANSIENT 0

errtype config_set_priority(const char* varname, uint priority);
// Sets the "priority" of a variable to the value specified.
// The priority of a variable affects what config files will
// change it upon being loaded.  see config_read_file
// setting a variable's priority to PRIORITY_TRANSIENT will
// cause it to never be written out by config_write_file.
// If the specified variable does not exist, returns
// ERR_NOEFFECT

uint config_get_priority(const char* varname);
// returns the priority of a variable.
// If the variable is not defined, returns PRIORITY_TRANSIENT

errtype config_get_origin(const char* varname, char* buf, int buflen);
// fills <buf> with the name of the config file from which <varname> was loaded.
// returns ERR_NOEFFECT if <varname> is not defined.
// <buflen> is the size of <buf>.


extern uint config_default_priority;
// The default priority used for a variable set
// using config_set_value.  Defaults to CONFIG_DFT_LO_PRI.


//----------------------------------------
// miscellany

void config_set_assign_characters (char* str);
// Before calling config_parse_command_line, call this function with a string
// containing all the characters you want to use to denote assignment.  If you
// don't call it, it is as if you called it with the string "=".

errtype config_parse_commandline(int argc, const char* argv[], bool (*do_switch)(char c));
// parses the switches and assignments on the commandline.  Assignments are stored in
// the current configuration.  Switches are handed to the switch handler.  If the switch handler returns
// true, the switch will be parsed as a variable, and will be set to the next token following the
// switch.  (regardless of spaces)

int config_compare(const char* var1, const char* var2);
// compares to config variables.  returns <0 if var1 < var2
// 0 if var1 == var2  > 0 if var1 > var2;



// ====================================================================
//                STANDARD CONFIG CLIENT IMPLEMENTATION
// ====================================================================


// ------------------------------------------------------
// DEFAULT WRITABLE FUNCTION

typedef struct _config_write_spec
{
   char* var;
   bool writable;
} config_write_spec;


// specify a null-terminated list of config_write_specs that is used by
// config_default_writable.
// If this is never called, then the table of write_specs will be emtpy.

extern void config_set_writable_table(config_write_spec* );

// This function is a valid "writable" argument to
// config_write_file.  It searches through
// the write_spec table for an entry that matches var, and
// returns the corresponding "writable" value.

extern bool config_default_writable(char* filaname, char* var);

// returned by config_default_writable for vars which have no table entry
// initially FALSE;
extern bool config_default_writability;

// --------------------------------------------------------
// DEFAULT SWITCH HANDLER

// The default switch handler operates on an array of switchdef structs.
// a switchdef struct names a switch character, an optional config variable, and
// a behavior for that switch.  The config_switchtype type describes the set of possible
// switch behaviors:


typedef enum
{
   CFGS_DEFINE,    // define the specified variable
   CFGS_UNDEFINE,  // undefine the specified variable.
   CFGS_VARIABLE,  // ignore the variable, and return "TRUE" from the default switch handler
                   // (i.e. parse as if the switch itself were a variable)
}  config_switchtype;


typedef struct _config_switchdef
{
   char sw;    // switch
   char* var;      // variable to set or unset, if applicable
   config_switchtype type;     // As specified above.
} config_switchdef;


// Specify a null-terminated list of switchdefs, which is used by config_default_switchfunc
// if this function is never called, then the switch table is empty.

extern void config_set_switch_table(config_switchdef* table);

// config_default_switchfunc iterates through the switch_table for a switchdef whose
// switch field matches "c."  If the type field of the switchdef is CFGS_DEFINE, then
// the config variable specified in the entry will be defined.  If it is CFGS_UNDEFINE,
// then the variable will be unset.
// returns TRUE iff type is CFGS_VARIABLE.

extern bool config_default_switchfunc(char c);

// Note that the default switch handler and the default writable func work together to assure that
// state changes due to switches are not written out.


// Globals

#ifdef __cplusplus
}
#endif

#endif // __CONFIG_H

