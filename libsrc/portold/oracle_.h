/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/oracle_.h,v 1.1 1996/10/21 21:41:11 buzzard Exp $

// INTERFACE

void OracleSmartStart(int answer_flags);
//  Oracle start frame.  The oracle tries to answer questions
//  indicated by answer flags.  Generally you need to do this
//  two frames in a row before he starts doing it.

void OracleStartFrame(int answer_flags, int memorize_flags);
// during this frame, answer queries from answer_flags,
// and memorize queries from memorize_flags
// we can only answer queries if answer_flags == last non-zero
//   memorize_flags)

// The following functions are actually macros so don't have prototypes

// bool OracleAnswer(int flags, void *data, int data_size)
//   returns true if the oracle answered (in which case results are
//   written into data)

// bool OracleMemorize(int flags, void *data, int data_size)
//   returns true if the oracle memorized the results

// bool OracleBool(int flags, function-returning-bool)
//   If the oracle is answering 'flags', then it returns
//   an appropriate bool.  If the oracle is memorizing 'flags',
//   then the function is called and the result memorized
//   and returned.  Otherwise the function is called, and the 
//   result returned out.

// IMPLEMENTATION INNARDS

extern int oracle_flags;
extern int oracle_save;

bool OracleFetch(int requeue, void *data, int datasize);
bool OracleStore(void *data, int datasize);

bool OracleFetchBool(int requeue);
bool OracleStoreBool(bool result);  // returns the passed-in bool

   // boolean function;
   //    pass in data (pointer) and size of data pointed to
   //    returns trye if the Oracle knew the result
#define OracleAnswer(f,data,size) \
     (oracle_flags & (f) ? OracleFetch(oracle_save & (f), data, size) : 0)

   // always returns false:
#define OracleMemorize(f,data,datasize) \
    (oracle_save & (f) ? OracleStore(data, datasize) : 0)

   // special convenient boolean oracle:

#define OracleBool(flags, func)                   \
     (oracle_flags & (flags)                      \
         ? OracleFetchBool(oracle_save & (flags)) \
         : (oracle_save & (flags))                \
             ? OracleStoreBool(func)              \
             : (func))
