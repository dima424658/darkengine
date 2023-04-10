; $Header: x:/prj/tech/libsrc/fix/RCS/fix.inc 1.19 1996/10/03 13:33:50 KEVIN Exp $
;
; Fixed-point types and macros
;

IFNDEF	fix_inc
fix_inc	EQU	1


fix	TYPEDEF	SDWORD
fix16	TYPEDEF	fix
fixang	TYPEDEF	word

	externdef	fix_sincos:near	;takes bx, returns eax,ebx
	externdef	fix_acos_:near	;takes eax, returns ax
	externdef	fix_sqrt_:near	;takes & returns, returns eax, modifies all but ebp

f_ceil  macro   reg
        add     reg,00000ffffh
        and     reg,0ffff0000h
        endm

f_floor macro   reg
        and     reg,0ffff0000h
        endm

f_cint  macro   reg
        add     reg,00000ffffh
        sar     reg,010h
        endm

f_fint  macro   reg
        sar     reg,010h
        endm

;fixup the result of multiply, putting the result in eax
mfixup	macro
	shr	eax,16
	shl	edx,16
	or	eax,edx
	endm

;fixed-point multiply of eax times arg. trashes edx 
fixmul	macro	arg
	imul	arg
        shrd    eax,edx,16
	endm

;rounding version of fixmul
fixmul_r	macro	arg
	local	no_round
	imul	arg
	add	eax,8000h	;add in get frac bit
	adc	edx,0
	shr	eax,16
	shl	edx,16
	or	eax,edx
	endm

;fixed-point divide of eax by arg. trashes edx. arg cannot be edx
fixdiv_fast     macro   arg
;;	if	("&arg" eq "edx") or ("&arg" eq "EDX")
;;	 echo	Arg to fixdiv cannot be edx
;;	 .err
;;	endif
	mov	edx,eax
	sar	edx,16
	shl	eax,16
	idiv	arg
	endm

fixdiv_safe	macro	arg
        local   ebx_ok,edx_ok,no_overflow,done
IFDIFI <arg>,<edx>
        mov     edx,arg
ENDIF

        push    ebx
        mov     ebx,eax

        sar     ebx,15
        push    edx             ; push denom

        shl     eax,16 
        push    ebx             ; push num high

        test    ebx,ebx
        jns     ebx_ok 

        neg     ebx

ebx_ok:           
        test    edx,edx
        jns     edx_ok 

        neg     edx    

edx_ok:           
        cmp     edx,ebx
        pop     edx             ;now put edx = num high        

        pop     ebx             ;        ebx = divisor
        ja      no_overflow   

        xor     edx,ebx       
        mov     eax,07fffffffh

        sar     edx,31        

        sub     eax,edx

        sub     eax,edx
        jmp     done

no_overflow:
        sar     edx,1         

        idiv    ebx
done:
        pop     ebx
        endm

;multiplies eax by arg1, divides by arg2
;keeps intermediate result in edx
fixmuldiv_fast macro arg1,arg2
        imul    arg1
        idiv    arg2
        endm

fixmuldiv_safe  macro   arg1,arg2
        local   ebx_ok,edx_ok,no_overflow,done

        imul    arg1            ; now in edx:eax

IFDIFI <arg2>, <ebx>
        push    ebx
        mov     ebx,arg2
ENDIF

        push    ebx
        push    edx

        push    eax
        test    ebx,ebx

        jns     ebx_ok
        neg     ebx

ebx_ok:
        test    edx,edx
        jns     edx_ok

        neg     edx

edx_ok:
        shl     edx,1

        shr     eax,31

        or      edx,eax
        pop     eax

        cmp     ebx,edx
        pop     edx

        pop     ebx
        ja      no_overflow

        xor     edx,ebx
        mov     eax,07fffffffh

        sar     edx,31

        sub     eax,edx

        sub     eax,edx
        jmp     done

no_overflow:
        idiv    ebx
done:
        pop     ebx
        endm

ifndef WIN32
fixdiv  macro arg
        fixdiv_fast arg
        endm

fixmuldiv macro arg1,arg2
        fixmuldiv_fast arg1,arg2
        endm
endif

ifdef WIN32
fixdiv  macro arg
        fixdiv_safe arg
        endm

fixmuldiv macro arg1,arg2
        fixmuldiv_safe arg1,arg2
        endm
endif

;makes a fixed point number with integral part a and fractional part b.
;works for immediates
fixmake	macro	a,b
	ifb	<b>
	 exitm	<(a) SHL 16>
	else
	 exitm	<((a) SHL 16) + b>
	endif
	endm


;here's some handy constants

f1_0	= fixmake(1)
f0_5	= fixmake(0,8000h)
f0_25	= fixmake(0,4000h)
	
ENDIF

