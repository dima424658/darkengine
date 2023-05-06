
option expr32
option casemap:none

IFNDEF	stdio_inc
stdio_inc	EQU		1

; Begin of file p:\w95\h\stdio.h
; 
;  *  stdio.h	Standard I/O functions
;  *
;  *  Copyright by WATCOM International Corp. 1988-1993.  All rights reserved.
;  
size_t		TYPEDEF		WORD

NULL		EQU		000000000h
__va_list		TYPEDEF		PTR SBYTE

BUFSIZ		EQU		512t
;  number of files that can be handled 
_NFILES		EQU		20t
FILENAME_MAX		EQU		80t
@t_stdio0		TYPEDEF		PTR BYTE
;  next character position 
;  number of characters left 
@t_stdio1		TYPEDEF		PTR BYTE
;  location of buffer 
;  mode of file access 
;  file handle 
;  size of buffer 
;  character placed here by ungetc 
;  tmpfile number 
__iobuf		STRUCT 1t
_ptr		@t_stdio0		?
_cnt		SWORD		?
_base		@t_stdio1		?
_flag		WORD		?
_handle		SWORD		?
_bufsize		WORD		?
_ungotten		BYTE		?
_tmpfchar		BYTE		?
__iobuf		ENDS

FILE		TYPEDEF		__iobuf

fpos_t		TYPEDEF		SDWORD

FOPEN_MAX		EQU		000000014h
OPEN_MAX		EQU		000000014h
;  maximum length of full pathname excl. '0' 
PATH_MAX		EQU		143t
;  extensions not enabled 
EXTERNDEF		C __iob:FILE

; 
;  *  Define macros to access the three default file pointer (and descriptors)
;  *  provided to each process by default.  They will always occupy the
;  *  first three file pointers in each processes' table.
;  
;  standard input file	
;  standard output file 
;  standard error file	
;  extensions enabled 
;  standard auxiliary file  
;  standard printer file  
;  values for _flag field in FILE struct and _iomode array 
;  file opened for reading 
_READ		EQU		000000001h
;  file opened for writing 
_WRITE		EQU		000000002h
;  ungetc has been done 
_UNGET		EQU		000000004h
;  big buffer allocated 
_BIGBUF		EQU		000000008h
;  EOF has occurred 
_EOF		EQU		000000010h
;  error has occurred on this file 
_SFERR		EQU		000000020h
;  file opened for append 
_APPEND		EQU		000000080h
;  file is binary, skip CRLF processing 
_BINARY		EQU		000000040h
;  full buffering 
_IOFBF		EQU		000000100h
;  line buffering 
_IOLBF		EQU		000000200h
;  no buffering 
_IONBF		EQU		000000400h
;  this is a temporary file 
_TMPFIL		EQU		000000800h
;  buffer has been modified 
_DIRTY		EQU		000001000h
;  is console device 
_ISTTY		EQU		000002000h
;   FILE is dynamically allocated   
_DYNAMIC		EQU		000004000h
;   End of File/Error return code   
EOF		EQU		0ffffffffh
;   Seek relative to start of file  
SEEK_SET		EQU		0t
;   Seek relative to current positn 
SEEK_CUR		EQU		1t
;   Seek relative to end of file    
SEEK_END		EQU		2t
L_tmpnam		EQU		13t
;   Max times tmpnam can be called  
TMP_MAX		EQU		0000044a8h
@proto_stdio0		TYPEDEF		PROTO C :PTR FILE
clearerr		PROTO		@proto_stdio0

@proto_stdio1		TYPEDEF		PROTO C :PTR FILE
fclose		PROTO		@proto_stdio1

@proto_stdio2		TYPEDEF		PROTO C :PTR FILE
feof		PROTO		@proto_stdio2

@proto_stdio3		TYPEDEF		PROTO C :PTR FILE
ferror		PROTO		@proto_stdio3

@proto_stdio4		TYPEDEF		PROTO C :PTR FILE
fflush		PROTO		@proto_stdio4

@proto_stdio5		TYPEDEF		PROTO C :PTR FILE
fgetc		PROTO		@proto_stdio5

@proto_stdio6		TYPEDEF		PROTO C :PTR FILE, :PTR fpos_t
fgetpos		PROTO		@proto_stdio6

@proto_stdio7		TYPEDEF		PROTO C :PTR SBYTE, :SWORD, :PTR FILE
fgets		PROTO		@proto_stdio7

@proto_stdio8		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE
fopen		PROTO		@proto_stdio8

@proto_stdio9		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE, :VARARG
fprintf		PROTO		@proto_stdio9

@proto_stdio10		TYPEDEF		PROTO C :SWORD, :PTR FILE
fputc		PROTO		@proto_stdio10

@proto_stdio11		TYPEDEF		PROTO C :PTR SBYTE, :PTR FILE
fputs		PROTO		@proto_stdio11

@proto_stdio12		TYPEDEF		PROTO C :PTR , :size_t, :size_t, :PTR FILE
fread		PROTO		@proto_stdio12

@proto_stdio13		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :PTR FILE
freopen		PROTO		@proto_stdio13

@proto_stdio14		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE, :VARARG
fscanf		PROTO		@proto_stdio14

@proto_stdio15		TYPEDEF		PROTO C :PTR FILE, :SDWORD, :SWORD
fseek		PROTO		@proto_stdio15

@proto_stdio16		TYPEDEF		PROTO C :PTR FILE, :PTR fpos_t
fsetpos		PROTO		@proto_stdio16

@proto_stdio17		TYPEDEF		PROTO C :PTR FILE
ftell		PROTO		@proto_stdio17

@proto_stdio18		TYPEDEF		PROTO C :PTR , :size_t, :size_t, :PTR FILE
fwrite		PROTO		@proto_stdio18

@proto_stdio19		TYPEDEF		PROTO C :PTR FILE
getc		PROTO		@proto_stdio19

@proto_stdio20		TYPEDEF		PROTO C 
getchar		PROTO		@proto_stdio20

@proto_stdio21		TYPEDEF		PROTO C :PTR SBYTE
gets		PROTO		@proto_stdio21

@proto_stdio22		TYPEDEF		PROTO C :PTR SBYTE
perror		PROTO		@proto_stdio22

@proto_stdio23		TYPEDEF		PROTO C :PTR SBYTE, :VARARG
printf		PROTO		@proto_stdio23

@proto_stdio24		TYPEDEF		PROTO C :SWORD, :PTR FILE
putc		PROTO		@proto_stdio24

@proto_stdio25		TYPEDEF		PROTO C :SWORD
putchar		PROTO		@proto_stdio25

@proto_stdio26		TYPEDEF		PROTO C :PTR SBYTE
puts		PROTO		@proto_stdio26

@proto_stdio27		TYPEDEF		PROTO C :PTR SBYTE
remove		PROTO		@proto_stdio27

@proto_stdio28		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE
rename		PROTO		@proto_stdio28

@proto_stdio29		TYPEDEF		PROTO C :PTR FILE
rewind		PROTO		@proto_stdio29

@proto_stdio30		TYPEDEF		PROTO C :PTR SBYTE, :VARARG
scanf		PROTO		@proto_stdio30

@proto_stdio31		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE
setbuf		PROTO		@proto_stdio31

@proto_stdio32		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE, :SWORD, :size_t
setvbuf		PROTO		@proto_stdio32

@proto_stdio33		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :VARARG
sprintf		PROTO		@proto_stdio33

@proto_stdio34		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :VARARG
sscanf		PROTO		@proto_stdio34

@proto_stdio35		TYPEDEF		PROTO C 
tmpfile		PROTO		@proto_stdio35

@proto_stdio36		TYPEDEF		PROTO C :PTR SBYTE
tmpnam		PROTO		@proto_stdio36

@proto_stdio37		TYPEDEF		PROTO C :SWORD, :PTR FILE
ungetc		PROTO		@proto_stdio37

@proto_stdio38		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE, :PTR PTR SBYTE
vfprintf		PROTO		@proto_stdio38

@proto_stdio39		TYPEDEF		PROTO C :PTR SBYTE, :PTR PTR SBYTE
vprintf		PROTO		@proto_stdio39

@proto_stdio40		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :PTR PTR SBYTE
vsprintf		PROTO		@proto_stdio40

@proto_stdio41		TYPEDEF		PROTO C 
fcloseall		PROTO		@proto_stdio41

@proto_stdio42		TYPEDEF		PROTO C :SWORD, :PTR SBYTE
fdopen		PROTO		@proto_stdio42

@proto_stdio43		TYPEDEF		PROTO C :SWORD
_grow_handles		PROTO		@proto_stdio43

@proto_stdio44		TYPEDEF		PROTO C 
fgetchar		PROTO		@proto_stdio44

@proto_stdio45		TYPEDEF		PROTO C :SWORD
fputchar		PROTO		@proto_stdio45

@proto_stdio46		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :SWORD
_fsopen		PROTO		@proto_stdio46

@proto_stdio47		TYPEDEF		PROTO C 
flushall		PROTO		@proto_stdio47

@proto_stdio48		TYPEDEF		PROTO C :PTR FILE, :PTR SBYTE, :PTR PTR SBYTE
vfscanf		PROTO		@proto_stdio48

@proto_stdio49		TYPEDEF		PROTO C :PTR SBYTE, :PTR PTR SBYTE
vscanf		PROTO		@proto_stdio49

@proto_stdio50		TYPEDEF		PROTO C :PTR SBYTE, :PTR SBYTE, :PTR PTR SBYTE
vsscanf		PROTO		@proto_stdio50

@proto_stdio51		TYPEDEF		PROTO C :PTR SBYTE, :WORD, :PTR SBYTE, :VARARG
_bprintf		PROTO		@proto_stdio51

@proto_stdio52		TYPEDEF		PROTO C :PTR SBYTE, :WORD, :PTR SBYTE, :PTR PTR SBYTE
_vbprintf		PROTO		@proto_stdio52

; End of file p:\w95\h\stdio.h

ENDIF

