;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; $Source: x:/prj/tech/hsrc/RCS/thunks.inc $
; $Author: KEVIN $
; $Date: 1997/02/07 15:31:21 $
; $Revision: 1.14 $
;
; These are macros to help make functions originally coded for
; Watcom callable from both Watcom & Microsoft C compiled
; functions. (toml 10-17-96)
;
; If you're compiling under Watcom C, everything is "normal"
; Under Microsoft, a thunk function with a Microsoft compatible
; name is generated.
;
; The macro generates needed thunk code when calling from
; Microsoft C modules into assembly language routines originally
; coded for Watcom.  In short, it converts __cdecl to Watcom:
;
;              ret     foo     arg1    arg2    arg3    arg4        cleanup
;
;   __cdecl:   eax      *      esp+4   esp+8   esp+12  esp+16      caller
;   Watcom:    eax      *      eax     edx     ebx     ecx         n/a
;
; Right now, these thunks automatically save certain registers
; that often do not need to be saved.  This is to cover functions
; that rely on "pragma aux modify" which instructs the compiler
; to save the registers, if needed.  We could probably add the
; registers to save as arguments to the _FUNCDEF macro
;
; Be aware the _FUNCDEF macro is limited to functions taking 0 - 4
; parameters, where the parameters are integer types or pointers.
; Floats, doubles and complex types are not supported.
;
; @Note (toml 10-15-96): The overkill register saving REALLY has got to go!
;
; I've added declarations to make the "real" watcom-convention routine
; name always accesible as funcname_asm from either Watcom or Microsoft
; so that c files can manipulate tables of functions called only from
; asm.
;
; Note that funcname_ should _always_ work for direct asm to asm, watcom
; passing convention.

ifndef THUNKS_INC
THUNKS_INC equ 1

; Not sure if this particular macro will ever be useful, but here it is.
;
 _FUNCNAME   MACRO   funcname
            _&funcname&
            ENDM

 _FUNCNAME_ASM   MACRO   funcname
            _&funcname&_asm
            ENDM


; Creates a list of externdef statements of a given type
;
 _EXTFUNC MACRO   parms:vararg
    for p,<parms>
      ifnb <p>
        externdef _&p&:near
      endif
    endm
    endm

; Put this at the top of every function that can be called by C or C++,
; where number indicates number of parameters 0-4
; @Note (toml 11-04-96): We should/could probably pass as arguments to
; this macro the registers to save and restore, rather than unconditionally
; saving all that we do.
;
 _FUNCDEF    MACRO   funcname:REQ, nargs:REQ
            if nargs gt 4
                echo Error: _FUNCDEF #args nargs out of allowed range 0-4
                .err
            endif
                public _&funcname&
                _&funcname&:
                push ebp
                mov  ebp, esp
                push ebx
                push edi
                push esi
                ifdif <nargs>,<0>
                    mov eax, 8[ebp]
                    ifdif <nargs>,<1>
                        mov  edx, 12[ebp]
                        ifdif <nargs>,<2>
                            mov  ebx, 16[ebp]
                            ifdif <nargs>,<3>
                                mov  ecx, 20[ebp]
                            endif
                        endif
                    endif
                endif
                call &funcname&_
                pop esi
                pop edi
                pop ebx
                pop ebp
                ret
                public _&funcname&_asm
                _&funcname&_asm:
            public &funcname&_
            &funcname&_:
            ENDM

;doesn't generate thunk, just appropriate naming convention
 _FUNCDEF_ASM   MACRO   funcname
            public _&funcname&_asm
            _&funcname&_asm:
            public &funcname&_
            &funcname&_:
            ENDM

; Use this when calling assembly functions defined with the _FUNCDEF macro,
; if you're mixing C and ASM.  It's more efficient, though, to directly
; call the WATCOM version.
;
_CALLFUNC   MACRO   funcname, nargs:=<0>
            ifidn <nargs>,<4>
                push ecx
                push ebx
                push edx
                push eax
            endif
            ifidn <nargs>,<3>
                push ecx
                
                push ebx
                push edx
                push eax
            endif
            ifidn <nargs>,<2>
                push ecx
                
                push edx
                push eax
            endif
            ifidn <nargs>,<1>
                push ecx
                
                push eax
            endif

            call _&funcname&

            ifidn <nargs>,<4>
                add ESP,010h
            endif
            ifidn <nargs>,<3>
                add ESP, 0Ch
                pop ecx
            endif
            ifidn <nargs>,<2>
                add ESP, 08h
                pop ecx
            endif
            ifidn <nargs>,<1>
                add ESP, 04h
                pop ecx
            endif
            ENDM

_CALLNAME   MACRO   funcname, nargs:=<0>
            _CALL _&funcname&, nargs
            ENDM

; Use this when calling throught a function pointer if you're mixing C and ASM.  
;
_CALLFUNCPTR   MACRO   funcname, nargs:=<0>
                ifidn <nargs>,<4>
                    push ecx
                    push ebx
                    push edx
                    push eax
                endif
                ifidn <nargs>,<3>
                    push ecx
                    
                    push ebx
                    push edx
                    push eax
                endif
                ifidn <nargs>,<2>
                    push ecx
                    
                    push edx
                    push eax
                endif
                ifidn <nargs>,<1>
                    push ecx
                    
                    push eax
                endif

                call &funcname&

                ifidn <nargs>,<4>
                    add ESP,010h
                endif
                ifidn <nargs>,<3>
                    add ESP, 0Ch
                    pop ecx
                endif
                ifidn <nargs>,<2>
                    add ESP, 08h
                    pop ecx
                endif
                ifidn <nargs>,<1>
                    add ESP, 04h
                    pop ecx
                endif
            ENDM

; Use this when jumping to assembly functions defined with the _FUNCDEF macro.
;
_JUMPFUNC   MACRO   jumpcondition, funcname
            jumpcondition &funcname&_
            ENDM

endif
