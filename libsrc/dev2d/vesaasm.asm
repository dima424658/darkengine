;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/vesaasm.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:03 $
;
; Routines which interface with the VBE supplied by the video
; board vendor or by a third party (like UNIVbe from SciTech).
; Also checks for UNILib from SciTech (not yet).
;
; This file is part of the 2d library.
;

.386

include type.inc
include cseg.inc
include dseg.inc
include dpmi.inc
include mode.inc
include grs.inc
include vesa.inc

_DATA segment
externdef _bankShift:BYTE               ; the amount to shift
externdef _setBankFuncPtr:DWORD         ; protected mode bank swither address
externdef _setDispStartFuncPtr:DWORD    ; protected mode display start function address
externdef _setPaletteFuncPtr:DWORD      ; protected mode palette func
externdef _VBE_version:WORD             ; the version of vbe
externdef _gsSel:WORD                   ; selector to store in gs
externdef _bytesPerScanLine:WORD        ; number of bytes in each scan line
externdef _dpmi_reg_data:PTR            ; point at the dpmi register data
externdef _grd_mode:DWORD               ; the current mode number
externdef _grd_screen:DWORD             ; need the screen info
externdef _useBlankInPalSet:BYTE        ; used to determine if must wait to set palette
_DATA ends

_TEXT segment

;
; SetBankProtA:
;
; Take bank number in eax and set the bank by calling the protected
; mode bank switcher for window A.  First shift by the necessary amount.
; Nothing but eax affected.
;

public SetBankProtA_
SetBankProtA_:

        pushad                          ; regs destroyed by call
        push    gs

        mov     gs,[_gsSel]             ; get the correct video selector
        mov     edx,eax                 ; move requested bank to correct place
        xor     ebx,ebx                 ; bh = 0 for set, bl = 0 for win A
        mov     cl,[_bankShift]         ; get the amount to shift
        shl     dl,cl                   ; shift bank requested by correct amount

        call    [_setBankFuncPtr]       ; make the call

        pop     gs
        popad
        ret

;
; SetBankProtAB:
;
; Take bank number in eax and set the bank by calling the protected
; mode bank switcher for each window.  First shift by the necessary amount.
; Nothing but eax affected.
;

public SetBankProtAB_
SetBankProtAB_:

        pushad                          ; everything destroyed
        push    gs

        mov     gs,[_gsSel]
        mov     edx,eax                 ; get bank in needed register
        xor     ebx,ebx                 ; bh = 0 for set, bl = 0 for win A
        mov     cl,[_bankShift]         ; get the amount to shift
        shl     dl,cl                   ; shift bank requested by correct amount

        pushad                          ; function destroys all regs (potentially)
        call    [_setBankFuncPtr]       ; make the call for window A
        popad                           ; restore all regs

        inc     ebx                     ; bl = 1 for window B
        call    [_setBankFuncPtr]       ; make the call for window B

        pop     gs
        popad                           ; get all regs back
        ret

;
; SetBankRealA:
;
; Take bank number in eax and set the bank by calling the real mode
; function, via interrupt, for modifying the window memory control.
; Change only window A.  Nothing but eax affected.
;

public SetBankRealA_
SetBankRealA_:

        pushad
        push    gs

        ; start by putting the bank in the register we want it
        mov     edx,eax

        mov     gs,[_gsSel]             ; make sure to have the video selector
        pushfd                          ; since we may be in interrupt
        cld                             ; make sure going up

        ; if we are running in an interrupt we need to save the
        ; potentially partially filled dpmi register on the current
        ; stack and then reset it after the current interrupt call

        mov     edi,p _dpmi_reg_data    ; get the pointer to the dpmi registers
        push    esi
        sub     esp,DPMIS_REG_SIZE      ; allocate room for the data
        mov     esi,edi                 ; esi points to source
        mov     edi,esp                 ; edi to destination
        push    es                      ; use es:edi so need ss
        mov     cx,ss                   ; get the stack segment
        mov     es,cx                   ; into es
        mov     ecx,DPMIS_REG_SIZE/4    ; count d words to move
        rep     movsd                   ; move them
        mov     ecx,DPMIS_REG_SIZE AND 3 ; count remainder
        rep     movsb                   ; move them
        mov     edi,p _dpmi_reg_data    ; reset edi
        pop     es                      ; and es

        mov     w [edi+DPMIS_REG_EAX],VBE_BANK_FUNC ; indicate this function
        mov     w [edi+DPMIS_REG_EBX],0 ; bh = 0 means set, bl = 0 means window A

        mov     cl,b _bankShift         ; get the amount to shift
        shl     dx,cl                   ; get true bank value for call
        mov     w [edi+DPMIS_REG_EDX],dx ; store the true bank number to set

        dpmi_real_interrupt 10h,0,es,edi ; actually make the interrupt call

        ; Now restore the old dpmi register data
        mov     esi,esp                 ; now ds:esi = source is stack
        push    ds                      ; so get ds = ss
        mov     cx,ss                   ; using cx as intermediate
        mov     ds,cx
        mov     ecx,DPMIS_REG_SIZE/4    ; number of dwords to move
        rep     movsd
        mov     ecx,DPMIS_REG_SIZE AND 3 ; and number left over
        rep     movsb
        pop     ds                      ; need ds restored
        add     esp,DPMIS_REG_SIZE
        pop     esi                     ; restore the source pointer

        ; clean up the rest and leave
        popfd                           ; restore the flags (direction bit)
        pop     gs
        popad
        ret

;
; SetBankRealAB:
;
; Take bank number in eax and set the bank by calling the real mode
; function, via interrupt, for modifying the window memory control.
; Change windows A and B.  Nothing but edx affected.
;

public SetBankRealAB_
SetBankRealAB_:

        pushad
        push    gs

        ; start by putting the bank in the register we want it
        mov     edx,eax

        mov     gs,[_gsSel]             ; make sure to have the video selector
        pushfd                          ; since we may be in interrupt
        cld                             ; make sure going up

        ; if we are running in an interrupt we need to save the
        ; potentially partially filled dpmi register on the current
        ; stack and then reset it after the current interrupt call

        mov     edi,p _dpmi_reg_data    ; get the pointer to the dpmi registers
        push    esi
        sub     esp,DPMIS_REG_SIZE      ; allocate room for the data
        mov     esi,edi                 ; esi points to source
        mov     edi,esp                 ; edi to destination
        push    es                      ; use es:edi so need ss
        mov     cx,ss                   ; get the stack segment
        mov     es,cx                   ; into es
        mov     ecx,DPMIS_REG_SIZE/4    ; count d words to move
        rep     movsd                   ; move them
        mov     ecx,DPMIS_REG_SIZE AND 3 ; count remainder
        rep     movsb                   ; move them
        mov     edi,p _dpmi_reg_data    ; reset edi
        pop     es                      ; and es

        mov     eax,VBE_BANK_FUNC       ; get ready for the bank switcher
        mov     w [edi+DPMIS_REG_EAX],ax ; store the function number
        mov     w [edi+DPMIS_REG_EBX],0 ; bh = 0 means set, bl = 0 means window A

        mov     cl,b _bankShift         ; get the amount to shift
        shl     dx,cl                   ; get true bank value for call
        mov     w [edi+DPMIS_REG_EDX],dx ; store the true bank number to set

        dpmi_real_interrupt 10h,0,es,edi ; actually make the interrupt call

        mov     eax,VBE_BANK_FUNC       ; do it again
        mov     w [edi+DPMIS_REG_EAX],ax ; and store again
        inc     w [edi+DPMIS_REG_EBX]   ; now bl = 1 means window B

        dpmi_real_interrupt 10h,0,es,edi ; and do it again

        ; Now restore the old dpmi register data
        mov     esi,esp                 ; now ds:esi = source is stack
        push    ds                      ; so get ds = ss
        mov     cx,ss                   ; using cx as intermediate
        mov     ds,cx
        mov     ecx,DPMIS_REG_SIZE/4    ; number of dwords to move
        rep     movsd
        mov     ecx,DPMIS_REG_SIZE AND 3 ; and number left over
        rep     movsb
        pop     ds                      ; need ds restored
        add     esp,DPMIS_REG_SIZE
        pop     esi                     ; restore the source pointer

        ; and clean up the rest of the thrashed vars
        popfd                           ; restore the direction bit
        pop     gs
        popad
        ret

;
; SetFocusIntern:
;
; Change the focus of the display to the requested values
; Takes (x,y) position in eax,edx and wait in ebx, which
; indicates whether we are to wait for the vertical retrace
; or not.
;

public SetFocusIntern_
SetFocusIntern_:

        push    ecx
        push    edi
        push    gs                              ; save old gs
        push    edx                             ; save both y
        push    eax                             ; and x

        mov     gs,[_gsSel]                     ; make sure to have video segment

        ; first determine if can wait for the retrace and if we should
        cmp     [_VBE_version],200h             ; VBE 2.0 and higher can wait
        jge     svf_can_wait
        xor     ebx,ebx                         ; indicate no waiting since not possible

svf_can_wait:
        ; ebx is either 0 for no wait or 1 for wait.  So shifting by
        ; VBE_WAIT_VRT_SHIFT makes ebx 0 or VBE_WAIT_VRT which is
        ; the correct value to send to the routine.
        shl     ebx,VBE_WAIT_VRT_SHIFT          ; shift into correct bit
        mov     ecx,[_setDispStartFuncPtr]      ; check if NULL
        or      ecx,ecx                         ; 
        jz      svf_real                        ; is NULL, use real interrupt

        ; Now the interface to the 2.0 protected mode version of the
        ; routine is different, as it takes the starting address of the
        ; display in DX:CX.  So we need to deal with bytes per pixel and
        ; other wacky things.  Also have strange need to shift by 2 if
        ; we are in something other than a 16 color mode (i.e. we are in
        ; a non-planar mode).

        push    ecx                             ; need to use ecx
        xor     ecx,ecx                         ; want to get short value
        mov     cx,[_bytesPerScanLine]          ; load it
        imul    edx,ecx                         ; start of line offset in edx
        add     edx,eax                         ; edx = total offset to pixel
        pop     ecx                             ; get back ecx
        
        mov     edi,[_grd_mode]                 ; get the mode
        imul    edi,GRS_MODE_INFO_SIZE          ; offset into array
        lea     edi,[_grd_mode_info+edi]        ; point to entry
        mov     al,b [edi+GRS_MODE_INFO_B]      ; get the number of bits

        cmp     al,4                            ; do we have 4 bit color?
        je      @F                              ; yes, skip the shift (why??)
        ; The following divide by 4 is also done by the vga_set_focus routine.
        ; I have no idea why we have to do it, I can find no refernce about it.
        shr     edx,2                           ; need to adjust to plane boundary (???)
@@:
        mov     edi,ecx                         ; store the address to jump to
        mov     cx,dx                           ; get address into DX:CX
        shr     edx,10h                         ; so shift edx down to low word

        ; At this point bx has the correct value, see note above
        pushad                                  ; call may modify all
        call    edi                             ; make call to the routine
        popad                                   ; return all regs
        jmp     svf_leave                       ; all done, finish by storing

svf_real:
        mov     edi,p _dpmi_reg_data            ; get the register data
        mov     w [_dpmi_reg_data+DPMIS_REG_EAX],VBE_DISP_START_FUNC

        ; bx already has the correct value either VBE_SET_START or
        ; VBE_SET_START | VBE_WAIT_VRT, see note above
        mov     w [_dpmi_reg_data+DPMIS_REG_EBX],bx
        mov     w [_dpmi_reg_data+DPMIS_REG_ECX],ax
        mov     w [_dpmi_reg_data+DPMIS_REG_EDX],dx
        dpmi_real_interrupt 10h,0,es,edi        ; make the interrupt

svf_leave:

        ; Now just finish up
        pop     eax
        pop     edx
        pop     gs
        pop     edi
        pop     ecx
        ret

;
; CallPaletteFunc
;
; Short routine to call the protected mode palette function to read
; or write the palette.  Takes start palette register in edx, num
; to read or write in ecx, pointer to data in edi and func in ebx.
;

public CallPaletteFunc_
CallPaletteFunc_:

        pushad                                  ; protected mode routine can change all

        mov     gs,[_gsSel]                     ; make sure to have video segment
        cmp     ebx,VBE_SET_PAL                 ; are we setting palette?
        jne     makeCall                        ; can make the call immediately
        mov     al,[_useBlankInPalSet]          ; get bool indicating if we must wait
        test    al,al                           ; is it non-zero?
        jz      makeCall                        ; go make the call
        or      bx,VBE_WAIT_VRT                 ; indicate we should wait
        jmp     makeCall                        ; now ready to make call

makeCall:
        call    [_setPaletteFuncPtr]            ; actually make call
        popad                                   ; clean up
        ret

_TEXT ends

end

