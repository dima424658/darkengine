; $Header: x:/prj/tech/hsrc/RCS/macros.inc 1.2 1997/09/23 13:28:47 KEVIN Exp $
; 

ifndef MACROS_INC
MACROS_INC equ 1


	include	types.inc

;pops into the debugger
debug_brk	macro	str
	ifdef	dbg_on
	 int	3
	endif
	endm


;does a debug_brk if the condition is true
break_if	macro	cc,str
	local	yes_break,no_break
	ifdef	dbg_on
	 j&cc	yes_break
	 jmp	no_break
yes_break:	debug_brk	str
no_break:
	endif
	endm


;compute absolute value of eax.  trashes edx
abs_eax	macro
	cdq
	xor	eax,edx
	sub	eax,edx
	endm

;computes negative of quadword value
neg64	macro	hireg,loreg
	neg	loreg
	not	hireg
	sbb	hireg,-1
	endm


rcsid	macro	str
	db	str
	align	4
	endm

;copies a constant number of bytes from esi to edi
copy_blk 	macro	size
	if	(size) gt 3
	 if	(size)/4 eq 1
	  movsd
	 else
	  mov	ecx,(size)/4
	  rep	movsd
	 endif
	endif
	if	(size) and 2
	 movsw
	endif
	if	(size) and 1
	 movsb
	endif
	endm

;inserts a profiler mark
_mark_	macro	str
	local	string,skip_str
	ifdef	_MARK_ON
	jmp	skip_str
string:
	%db	"&str"
	db	0
skip_str:	pushf
	pushm	eax,edx
	mov	dx,cs
	lea	eax,p string
	int	3
	popm	eax,edx
	popf
	endif
	endm

;count arguments
@ArgCount	macro	parms:vararg	;returns number of parms
	local	n
	n = 0
	for	p,<parms>
	 n = n+1
	endm
	exitm	<n>
	endm

;for generating enumerated types
enum	macro	n:=<0>,syms:vararg
	enum_cnt = n
	enum2	syms
	endm

;for generating additional values
enum2	macro	syms:vararg
	for	s,<syms>
	 s = enum_cnt
	 enum_cnt = enum_cnt+1
	endm
	endm

;retuns the ascii value of the uppercase of a character
chrval_u	macro	chr
	local	v
	v = '&chr'
	if	(v ge 'a') and (v le 'z')
	 exitm	<v + 'A'-'a'>
	else
	 exitm	<v>
	endif
	endm

;compare strings without case sesitivity. returns true if equal
cmpstr	macro	str0,str1
	local	len,i

	len = @SizeStr(str0)
	if	len ne @SizeStr(str1)
	 exitm	<0>
	endif

	i = 1
	rept	len
	 if	chrval_u(@SubStr(str0,i,1)) ne chrval_u(@SubStr(str1,i,1))
	  exitm	;break out of rept
	 endif
	 i = i+1
	endm

	if	i le len
	 exitm	<0>
	else
	 exitm	<1>
	endif

	endm

;skips the 1-byte instruction immediately following. SETS FLAGS!
skip1	macro		;skips next 1-byte instruction
	db	010101000b	;a8: test al,imm
	endm

;skips the 2-byte instruction immediately following. SETS FLAGS!
skip2	macro		;skips next 2-byte instruction
	db	66h,010101001b	;66,a9: test ax,imm
	endm

;skips the 4-byte instruction immediately following. SETS FLAGS!
skip4	macro		;skips next 2-byte instruction
	db	010101001b	;66,a9: test eax,imm
	endm

;moves memory to memory, trashing eax
m2m	macro	dest,src
	mov	eax,src
	mov	dest,eax
	endm

;swaps memory with memory, trashing eax
mswap	macro	dest,src
	local	reg

	if	SIZEOF(TYPE(dest)) NE SIZEOF(TYPE(src))
	 echo	Error: operands must be same size in mswap
	 .err
	endif

	if	SIZEOF(TYPE(dest)) eq 4
	 reg equ eax
	elseif	SIZEOF(TYPE(dest)) eq 2
	 reg equ ax
	elseif	SIZEOF(TYPE(dest)) eq 1
	 reg equ al
	else
	 echo	Error: unknown operand size in mswap
	 .err
	endif

	mov	reg,dest
	xchg	reg,src
	mov	dest,reg

	endm


;multiple push
pushm	macro	parms:vararg
	for	p,<parms>
	 ifnb	<p>
	  push	p
	 endif
	endm
	endm

;multiple pop
popm	macro	r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13
	ifnb	<r0>
	popm	r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13
	pop	r0
	endif
	endm


;creates a list of externdef statements of a given type
extdef	macro	type,parms:vararg
	for	p,<parms>
	 ifnb	<p>
	  externdef p:type
	 endif
	endm
	endm

;macro to gernerate ext<size> macros
extgen	macro	id,size

 ext&id	macro	parms:vararg
	 extdef	size,parms
	endm

	endm

;generate ext<size> macros for all the sizes
	extgen	n,near
	extgen	b,byte
	extgen	w,word
	extgen	d,dword
        extgen  q,qword
	extgen	fix,fix


;creates a 'public' statement for a symbol with a trailing underscore
cpublic	macro	p0
p0&_	= p0
	public	p0&_
	endm


;returns log2 of a number, or -1 if not power of 2
FINDSHIFT	macro	src
	;local	shiftcnt,t
shiftcnt = 0
t = src
	while	(t and 1) eq 0
	 t = t SHR 1
	 shiftcnt = shiftcnt +1
	endm

	if	t eq 1	;only one bit!
	 exitm	<shiftcnt>
	else
	 exitm	<-1>
	endif

	endm

;returns bit number of lowest bit in src
LOWBIT	macro	src
	;local	shiftcnt,t
shiftcnt = 0
t = src
	while	(t and 1) eq 0
	 t = t SHR 1
	 shiftcnt = shiftcnt + 1
	endm

	exitm	<shiftcnt>

	endm

;returns bit number of lowest bit in src
HIGHBIT	macro	src
	;local	shiftcnt,t
shiftcnt = 0
t = src
	while	t
	 t = t SHR 1
	 shiftcnt = shiftcnt + 1
	endm

	exitm	<shiftcnt-1>

	endm

;returns the number of bits set in srg
NUMBITS	macro	src
	;local	cnt,t
cnt = 0
t = src
	while	t
	 if	t and 1
	  cnt = cnt + 1
	 endif
	 t = t SHR 1
	endm

	exitm	<cnt>

	endm




;returns larger of a or b
MAX	macro	a,b
	if	a gt b
	 exitm	<a>
	else
	 exitm	<b>
	endif
	endm

;returns a bitmask of all bits numbern n..0 set
MAKEMASK	macro	arg
	local	t,n
n = arg
t = 1
	while	n
	 t = (t SHL 1) + 1
	 n = n - 1
	endm

	exitm	<t>

	endm


;generates a series mov,shift,add instructions to do a mul
genmul	macro	reg,n,scr
	local	cnt,t,scr_used
t = n
scr_used = 0
cnt = 0
	while	(t ne 0)

	 if	t eq 1
	  if	cnt
	   shl	reg,cnt
	  endif
	  if	scr_used eq 1
	   add	reg,scr
	  endif
	  exitm
	 elseif	t and 1
	  if	cnt
	   shl	reg,cnt
	  endif
	  cnt = 0
	  if	scr_used eq 1
	   add	scr,reg
	  else
	   mov	scr,reg
	   scr_used = 1
	  endif
	 endif
	 cnt = cnt + 1
	 t = t SHR 1

	endm

	endm

;generates a series mov,shift,add instructions to do a mul
;alternate version for constants like 31, to generate reg = reg << 5 - reg
genmul2	macro	reg,n,scr
	local	cnt,t,scr_used
t = n
scr_used = 0
cnt = 0
	while	(t ne 0)

	 if	t eq 1
	  if	cnt
	   shl	reg,cnt
	  endif
	  if	scr_used eq 1
	   sub	reg,scr
	  endif
	  exitm
	 elseif	t and 1
	  if	cnt
	   shl	reg,cnt
	  endif
	  cnt = 0
	  if	scr_used eq 1
	   add	scr,reg
	  else
	   mov	scr,reg
	   scr_used = 1
	  endif
	 endif
	 cnt = cnt + 1
	 t = t SHR 1

	endm

	endm


;mulitplies by a constant, doing shift if possible
imulc	macro	reg,n,scratch
	;local	lbit,hbit,nbits,mul_cost,use_alt,altn
	if	n eq 1	;check for 1
	 echo	Warning: imulc used with arg==1
	 exitm
	elseif	n eq 0	;check for zero
	 echo	Warning: imulc used with arg==0
	 xor	reg,reg
	 exitm
	endif

	if	n lt 0
	 imulc	reg,-n,scratch
	 neg	reg
	 exitm
	endif

lbit = LOWBIT(n)
hbit = HIGHBIT(n)
nbits = NUMBITS(n)
	if	lbit eq hbit
	 shl	reg,hbit
	elseif	(n eq 3) or (n eq 5) or (n eq 9) ;can we use lea?
	 lea	reg,[reg*(n-1)+reg]	;..ok, use lea
	else

;determine the cost of doing an imul. by the formula in pc mag book
mul_cost = MAX(3,hbit+1) + 6

;determine the cost of doing the shifts
shift_cost = nbits * 3	;each bit is a (shl and (mov or add)) = 3 ticks

	if	nbits GT ((hbit+1)/2)	;use alternte genmul
	 altn = 1 + (n XOR MAKEMASK(hbit+1))
	 shift_cost = NUMBITS(altn) * 3	;each bit is a (shl and (mov or add)) = 3 ticks
	 use_alt = 1
	else
	 use_alt = 0
	endif

ifb <scratch>
 shift_cost = shift_cost + 2	;push/pop pair is 2 ticks
endif

;use the appropriate method

	 if	mul_cost le shift_cost
	  imul	reg,n
	 else
	  ifnb	<scratch>	;use shifts & adds
	   if	use_alt
	    genmul2 reg,altn,scratch
	   else
	    genmul reg,n,scratch
	   endif
	  else
	   if	("reg" eq "ebx") or ("reg" eq "EBX")
	    scr equ eax
	   else
	    scr equ eax
	   endif
	   push	scr
	   if	use_alt
	    genmul2 reg,altn,scr
	   else
	    genmul reg,n,scr
	   endif
	   pop	scr
	  endif
	 endif

	endif

	endm

endif

