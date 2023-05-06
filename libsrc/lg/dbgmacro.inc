;
; $Source: x:/prj/windex/libsrc/lg/rcs/dbgmacro.inc $
; $Revision: 1.17 $
; $Author: JACOBSON $
; $Date: 1996/02/06 13:38:16 $
;
; Macros for debug system
;
; $Log: dbgmacro.inc $
;Revision 1.17  1996/02/06  13:38:16  JACOBSON
;Removed the damned one/letter macros, removed a name collision,
;and re/enabled the optimized dbg macros.
;
;Revision 1.16  1996/02/05  17:14:55  JACOBSON
;Removed doug's windoze hacks since it seems to work better without
;them (why were they put in?? Will things blow up in the future?)
;
;Revision 1.13  1993/08/10  22:45:14  dc
;externdef dbgBank since h2inc cant convert that stuff correctly
;
; Revision 1.12  1993/04/16  14:17:22  matt
; Added enddbg as alternate to dbgend
; 
; Revision 1.11  1993/04/14  17:47:43  matt
; Made pushlong work with segment registers
; 
; Revision 1.10  1993/04/14  12:14:33  matt
; Fixed dbstr macro to handle \n at start of string (or two \n's together)
; 
; Revision 1.9  1993/04/13  14:02:51  matt
; Added check for pushlong argument with no size
; 
; Revision 1.8  1993/04/13  13:43:33  matt
; Cleaned up spew & pushlong macros
; 
; Revision 1.7  1993/04/09  14:34:03  matt
; Changed spew_if to use vararg parms
; 
; Revision 1.6  1993/04/02  10:38:02  matt
; Fixed dbgs to conditional assemble based on spew_on flag.
; 
; Revision 1.5  1993/03/31  12:02:28  matt
; Added dbgs macro (like the C one), and made <flag> optional on spew.
; 
; Revision 1.4  1993/03/28  17:16:21  matt
; Took out debugging 'echo' statements in macro
; 
; Revision 1.3  1993/03/26  20:03:06  matt
; spew macro now sign/zero extends parms if preceded by @
; 
; Revision 1.2  1993/03/26  11:51:30  matt
; Changed spew to generate just LF for \n, not CR/LF
; 
; Revision 1.1  1993/03/24  12:13:17  matt
; Initial revision
; 
;


;macro to db a string, replacing \n with an LF
dbstr	macro	str
	t	= 2
	pos	= @InStr(,str,<\n>)
	while	pos
	 if	(pos-t)
	  tmp	textequ @SubStr(&str,t,pos-t)
	  %db	"&tmp"
	 endif
	 db	10
	 t	= pos + 2
	 pos	= @InStr(t,str,<\n>)
	endm

	if	t lt @SizeStr(str)
	 tmp	textequ @SubStr(&str,t)
	 %db	"&tmp
	endif

	endm

	extn	DbgDoSpew_,DbgSpewTest_

;generate a mov mem,reg when mem is in the code segment
movc_mr	macro	addr,reg
	mov	ds:4,reg
	org	$-4
	dd	addr
	endm

;generate a mov reg,mem when mem is in the code segment
movc_rm	macro	reg,addr
	mov	reg,ds:4
	org	$-4
	dd	addr
	endm

;pushes a longword, zero extending if necessary
pushlong	macro	parm
	local	t
	if	(OPATTR (parm)) AND 4	;immediate?
	 push	parm
	elseif	(SIZEOF(TYPE(parm))) EQ 0
	 if	(OPATTR (parm)) AND 10h	;if a reg with zero len, must be a seg reg
	  push	eax
	  mov	ax,parm
	  movzx	eax,ax
	  xchg	eax,[esp]
	 else
	  echo	pushlong error: unknown size for argument <parm>
	  .err
	 endif
	elseif	(SIZEOF(TYPE(parm))) LT 4
	 push	eax
	 movzx	eax,parm
	 xchg	eax,[esp]
	else
	 push	parm
	endif

	endm

;spew like the C function - takes flag,ctrl string, and variable number of args
spew	macro	flag,str,p0,p1,p2,p3,p4,p5,p6,p7,p8,p9
	local	string,skip_string,done,esp_fix
	ifdef	spew_on
	jmp	skip_string
string:	dbstr	str
	db	0
skip_string:	pushf
	pushm	eax,ebx
	ifnb	<flag>
	 push	eax
	 mov	eax,flag
	 call	DbgSpewTest_
	 or	al,al
	 pop	eax
	 je	done
	endif
	movc_mr	esp_fix+1,esp
	for	parm,<p9,p8,p7,p6,p5,p4,p3,p2,p1,p0>
	 ifnb	<parm>
	  pushlong parm
	 endif
	endm
	push	offset string

	call	DbgDoSpew_
esp_fix:	mov	esp,12345678h	;restore sp
done:
	popm	eax,ebx
	popf
	endif
	endm

spew_if	macro	cc,flag,str,parms:vararg
	local	yes_spew,no_spew
	j&cc	yes_spew
	jmp	no_spew
yes_spew:	spew	flag,str,parms
no_spew:
	endm


DBGBANK	macro	src
	exitm	<src SHR NUM_DBG_SLOTS>
	endm

label_ctr = 0

;Conditional debugging macros

ifdef dbg_on	;if on, generate debugging macros

;generates code if dgb_on and that bit on. must be paired with dbgend
dbg	macro	src
	local	do_debug
	pushf
	push	eax
	mov	eax,DBGBANK(src)
;;	imul	eax,size DbgBank
	imulc	eax,%(size DbgBank)
	mov	eax,dbgBank[eax].(gate+DG_DBG)
	and	eax,src
	cmp	eax,src AND DBG_SLOT_MASK
	pop	eax
	je	do_debug
	popf	
	jmp	@CatStr(__dbg__,%label_ctr)
do_debug:	popf
	endm

;end of a block started with dbg - either enddbg or dbgend
dbgend	macro
@CatStr(__dbg__,%label_ctr):
label_ctr = label_ctr+1
	endm

enddbg	macro
@CatStr(__dbg__,%label_ctr):
label_ctr = label_ctr+1
	endm

else	;if no debugging, generate if 0, endif to kill code

dbg equ <if 0 ;>
dbgend equ <endif>
enddbg equ <endif>

endif



ifdef spew_on	;if on, generate debugging macros

;like dbg, but based on spew flag. paired with dbgend
dbgs	macro	src
	local	do_debug
	pushf
	push	eax
	mov	eax,src
	call	DbgSpewTest_
	or	al,al
	pop	eax
	jnz	do_debug
	popf	
	jmp	@CatStr(__dbg__,%label_ctr)
do_debug:	popf
	endm

;end of a block started with dbg
dbgsend	macro
@CatStr(__dbg__,%label_ctr):
label_ctr = label_ctr+1
	endm

;end of a block started with dbg
enddbgs	macro
@CatStr(__dbg__,%label_ctr):
label_ctr = label_ctr+1
	endm

; and, since h2inc is, although short named, a total piece of shit
EXTERNDEF		C dbgBank:DbgBank

else	;if no debugging, generate if 0, endif to kill code

dbgs equ <if 0 ;>
dbgsend equ <endif>
enddbgs equ <endif>

endif

