; $Header: r:/t2repos/thief2/src/portal/ptmapper.asm,v 1.5 1997/01/31 14:43:38 JAEMZ Exp $
;
; PORTAL Texture Mappers
;
; ptmap.asm
;   support functions


.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc
        include thunks.inc

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	para public USE32 'DATA'

	align	8

tmap_loc  dq 0,0,0
ma  dq 0
mb  dq 0
mc  dq 0
dumb_scale  dd 65536
x_step dd 0
dxdy dd 0
u dd 0
v dd 0
du dd 0,0
dv dd 0,0
span_end dd 0
y_pos dd 0

externdef _fda:qword
externdef _fdb:qword
externdef _fdc:qword
externdef _u_step_data:qword
externdef _v_step_data:qword
externdef _tmap_data:qword
externdef _dest_row:dword
externdef _stat_num_dest_pixels:dword
externdef _clip_alloc:dword

externdef _slist_count:dword
externdef _snext:dword
externdef _start_y:dword
externdef _last_y:dword
 
_DATA	ends


_TEXT	segment para public USE32 'CODE'

;extn pt_scan_convert
;extn pt_asm_outer_loop
extn ptmap_perspective_run_asm

;
; PT_SCAN_CONVERT
;
;  EAX = x
;  EBX = dx
;  ECX = count
;  EDI = destination

  _FUNCDEF pt_scan_convert,4

;  convert to split 16:16

    push   ebp
    push   edi
    
    mov    edi,edx
    mov    edx,ebx

    shl    edx,16
    add    eax,65535    ; let truncation be fix_cint

    sar    ebx,16
    mov    ebp,eax

    shl    ebp,16

    sar    eax,16

scan_loop:
    mov    [edi],eax
    add    ebp,edx

    adc    eax,ebx
    add    edi,8

    dec    ecx
    jnz    scan_loop

    pop    edi
    pop    ebp
    ret

; PT_SCAN_CONVERT_UV
;
;  EAX = uv_scan_info
;  EBX = &uvdata
;  ECX = count
;  EDX = &xdata

  _FUNCDEF pt_scan_convert_uv,4
       push  ebp
       push  esi

       push  edi
       mov   esi,eax

       mov   edi,edx
       push  ecx

; EAX,ECX,EDX temps
;  EDX = this_x around the loop

       mov   eax,[esi+4]
       mov   edx,[esi]

       sar   eax,16
       add   edx,65535

       sar   edx,16
       mov   ecx,[esi+4]
       mov   dxdy,ecx
       mov   ecx,[esi+8]
       mov   u,ecx
       mov   ecx,[esi+12]
       mov   v,ecx
       mov   ecx,[esi+16]
       mov   du,ecx
       mov   ecx,[esi+20]
       mov   dv,ecx
       mov   ecx,[esi+24]
       mov   du[4],ecx
       mov   ecx,[esi+28]
       mov   dv[4],ecx

       mov   ebp,[esi]
       mov   x_step,eax

;  EBX = uvdata
;  EDI = xdata
;  EBP = x

scan_uv_loop:
       mov   [edi],edx    ; xdata[y][0] = this_x
       add   edi,8        ; ++xdata

       mov   eax,dxdy     ; EAX = dx

       add   eax,ebp      ; x += dx
       mov   ecx,x_step

       mov   ebp,eax      ; ebp = new_x
       add   eax,65535
       
       sar   eax,16       ; eax = this_x
       add   ecx,edx      ; edx = prev_x + x_step

       mov   esi,eax
       mov   edx,eax

       sub   esi,ecx      ; edx = (prev_x + x_step != this_x)
       mov   eax,u        ; u

       mov   ecx,v
       mov   [ebx],eax    ; uvdata[y][0]

       mov   [ebx+4],ecx  ; uvdata[y][1]
       add   ebx,8        ; ++uvdata
      
       add   eax,du[esi*4]
       add   ecx,dv[esi*4]

       ; 2-cycle instructions stall

       mov   u,eax
       mov   v,ecx

       pop   eax

       dec   eax          ; decrement & test count

       push  eax
       jnz   scan_uv_loop

       pop   eax
       pop   edi

       pop   esi
       pop   ebp

       ret
       

; EAX = starting (a,b,c)
; EDI = destcore
; ESI = &xdata[i][0]
; ECX = # of lines

  _FUNCDEF pt_asm_outer_loop,4
        push  esi
        push  edi

        mov   edi,edx
        mov   esi,ebx

        fld   qword ptr [eax]
        fld   qword ptr [eax+8]
        fld   qword ptr [eax+16]
        fxch  st(2)
        fstp  ma
        fstp  mb
        fstp  mc
        jmp   outer_scan_loop

;  outer loop:
;    ESI = xdata[i]
;    ECX = # of lines left
;    EDI = destcore
;    ma,mb,mc are globals

no_draw:
        dec    ecx
        jz     all_done

        fld    ma
        fadd   _tmap_data+6*8
        fld    mb
        fadd   _tmap_data+7*8
        fld    mc
        fadd   _tmap_data+8*8
        fxch   st(2)
        fstp   ma
        fstp   mb
        fstp   mc

        mov    eax,_dest_row
        add    esi,8

        add    edi,eax
        nop

outer_scan_loop:
        mov    eax,[esi]
        mov    ebx,[esi+4]

        sub    ebx,eax
        jle    no_draw

; EBX = len
; EAX = x

        push   ecx
        push   esi
        push   edi
        mov    ecx,_stat_num_dest_pixels
        add    ecx,ebx
        mov    _stat_num_dest_pixels,ecx

        fild   dword ptr [esi]        ; 0   x

        ; the above instruction has some latency (7?!!)
        ; so let's make sure some stuff is in cache

        mov    ecx,dword ptr tmap_loc
        mov    edx,dword ptr ma

        mov    ecx,dword ptr _fda
        mov    edx,dword ptr _fdc

        mov    ecx,dword ptr _fdb
        mov    edx,dword ptr mb

        mov    ecx,dword ptr mc
        mov    edx,dword ptr tmap_loc+16

        fld    st(0)                  ; 1   x x

        fmul   _fda                   ; 2   (2)fda*x x
        fxch   st(1)                  ;     x (2)fda*x

        fld    st(0)                  ; 3   x x (2)fda*x

        fmul   _fdb                   ; 4   (4)fdb*x x (2)fda*x
        fxch   st(2)                  ;     (2)fda*x x (4)fdb*x

        fadd   ma                     ; 5   (5)a x (4)fdb*x
        fxch   st(1)                  ;     x (5)a (4)fdb*x

        fmul   _fdc                   ; 6   (6)fdc*x (5)a (4)fdb*x
        fxch   st(2)                  ;     (4)fdb*x (5)a (6)fdc*x

        fadd   mb                     ; 7   (7)b (5)a (6)fdc*x
        fxch   st(1)                  ;     (5)a (7)b (6)fdc*x

        fld    _fdc                   ; 8   fdc (5)a (7)b (6)fdc*x

        fmul   st,st(1)               ; 9   (9)a*fdc (5)a (7)b (6)fdc*x
        fxch   st(3)                  ;     (6)fdc*x (5)a (7)b (9)a*fdc

        fadd   mc                     ; 10  (10)c (5)a (7)b (9)a*fdc
        fxch   st(1)                  ;     (5)a (10)c (7)b (9)a*fdc

        fstp   tmap_loc               ; 11  (10)c (7)b (9)a*fdc

        fxch   st(1)                  ; 13  (7)b (10)c (9)a*fdc

        fstp   tmap_loc+8             ; 15  (10)c (9)a*fdc

        fst    tmap_loc+16            ; 17  c a*fdc

        ; reset counter to 3

        fmul   _fda                   ; 3   (3)c*fda a*fdc
        fxch   st(1)                  ;     a*fdc (3)c*fda

        fld    tmap_loc+16            ; 4   c a*fdc (3)c*fda

        fmul   _fdb                   ; 5   (5)c*fdb a*fdc (3)c*fda
        fxch   st(1)                  ;     a*fdc (5)c*fdb (3)c*fda

        fsubp  st(2),st               ; 6   (5)c*fdb (6)u_step

        fld    tmap_loc+8             ; 7   b (5)c*fdb (6)u_step

        fmul   _fdc                   ; 8   (8)b*fdc (5)c*fdb (6)u_step
        fxch   st(2)                  ;     (6)u_step (5)c*fdb (8)b*fdc

        fld    ma                     ; 9   ma (6)u_step (5)c*fdb (8)b*fdc

        fadd   _tmap_data+6*8         ; 10  ma' (6)u_step (5)c*fdb (8)b*fdc
        fxch   st(2)                  ;     (5)c*fdb (6)u_step ma' (8)b*fdc

        fsubrp  st(3),st              ; 11  (6)u_step ma' (11)v_step

        fstp   _u_step_data           ; 12  ma' (11)v_step

        fld    mb                     ; 14  mb ma' (11)v_step

        fadd   _tmap_data+7*8         ; 15  mb' ma' (11)v_step
        fxch   st(2)                  ;     (11)v_step ma' mb'

        fstp   _v_step_data           ; 16  (10)ma' (15)mb'
        
        fld    mc                     ; 18  mc (10)ma' (15)mb'

        fadd   _tmap_data+8*8         ; 19  (19)mc' (10)ma' (15)mb'
        fxch   st(2)                  ;     (15)mb' (10)ma' (19)mc'

        fstp   mb

        fstp   ma

        fstp   mc

        add    edi,eax 
        mov    esi,ebx

        lea    eax,tmap_loc
        call   ptmap_perspective_run_asm

        pop    edi
        pop    esi
        pop    ecx
        mov    eax,_dest_row

        add    esi,8
        add    edi,eax

        dec    ecx
        jnz    outer_scan_loop

all_done:
        pop    edi
        pop    esi

        ret

;
;  RENDER_RPI_UNLIT_ASM
;
;     EAX: pointer to span info
;     EDX: pointer to ma,mb,mc
;     EBX: dest
;     ECX: span_end
;
;  span: 2 bytes x
;        1 byte  len
;        1 byte  delta y

   _FUNCDEF pt_render_rpi_unlit_asm,4
      push   esi
      push   edi
      push   ebp

      mov    esi,eax
      mov    edi,ebx

      mov    span_end,ecx

      fld   qword ptr [edx]
      fld   qword ptr [edx+8]
      fld   qword ptr [edx+16]
      fxch  st(2)
      fstp  ma
      fstp  mb
      fstp  mc

rrua_loop_top:
      mov    al,byte ptr 2[esi]    ; get length in eax
      xor    ebx,ebx

      cmp    al,0
      je     rrua_big_skip_y       ; length == 0 means skip vertically a bunch
       
      mov    bx,word ptr [esi]
      and    eax,0ffh

      cmp    al,255
      jne    rrua_good_length      ; length == 255 means length encoded in next

      mov    ax,word ptr 4[esi]
      add    esi,4

rrua_good_length:

      ; compute a,b,c,u_step_data,v_step_data

      mov    x_step,ebx

      fild   x_step    ;  x

      fld    st(0)     ;  x x

      fmul   _fdc      ;  fdc*x  x

      fld    st(1)     ;  x  fdc*x  x

      fmul   _fda      ;  fda*x  fdc*x  x
      fxch   st(1)     ;  fdc*x  fda*x  x

      fadd   mc        ;  c  fda*x  x
      fxch   st(2)     ;  x  fda*x  c

      fmul   _fdb      ;  fdb*x  fda*x  c
      fxch   st(1)     ;  fda*x  fdb*x  c

      fadd   ma        ;  a  fdb*x c
      fxch   st(1)     ;  fdb*x  a  c

      fadd   mb        ;  b  a  c
      fxch   st(2)     ;  c  a  b

      fst    qword ptr tmap_loc+16

      fxch   st(1)     ;  a  c  b

      fstp   qword ptr tmap_loc   ; c b

      fxch   st(1)

      fstp   qword ptr tmap_loc+8

      fld    st(0)

      fmul   _fda      ; c*fda c

      fld    _fdc      ; fdc c*fda c

      fmul   qword ptr tmap_loc ; a*fdc c*fda c
      fxch   st(2)     ; c c*fda a*fdc

      fld    _fdc      ; fdc c c*fda a*fdc

      fmul   qword ptr tmap_loc+8 ; b*fdc c c*fda a*fdc
      fxch   st(2)     ; c*fda c b*fdc a*fdc

      fsubrp  st(3),st  ; c b*fdc u_step_data

      fmul   _fdb      ; c*fdb b*fdc u_step_data
      fxch   st(2)     ; u_step_data b*fdc c*fdb

      fstp   _u_step_data    ; b*fdc c*fdb

      fsub

      fstp   _v_step_data
      
      push   edi
      push   esi

      add    edi,ebx
      mov    esi,eax

      lea    eax,tmap_loc
      call   ptmap_perspective_run_asm

      pop    esi
      xor    eax,eax

      pop    edi
      mov    al,byte ptr 3[esi]

      cmp    al,1          ; fast common case: one span per scanline
      jne    rrua_not_one

      fld    ma

      fld    mb

      fld    mc
      fxch   st(2)     ;  a b c

      fadd   _tmap_data+6*8    ; a' b c
      fxch   st(2)

      fadd   _tmap_data+8*8    ; c' b a'
      fxch   st(1)

      fadd   _tmap_data+7*8    ; b' c' a'
      fxch   st(2)             ; a' c' b'

      fstp   ma

      fstp   mc

      fstp   mb

      mov    eax,_dest_row

      add    edi,eax
      jmp    rrua_next      

rrua_big_skip_y:
      mov    ax,word ptr [esi]
      jmp    rrua_skipped

rrua_not_one:
      cmp    al,0        ; other fast case: multiple spans per scanline
      je     rrua_next

      ; slow case: skipped a scanline
      ;   length skipped is in eax
rrua_skipped:

      mov    x_step,eax
      mov    ebx,_dest_row

      imul   eax,ebx

      add    edi,eax

      fild   x_step    ; dy

      fld    st(0)

      fmul   _tmap_data+6*8    ; dy*dady dy
      fxch   st(1)             ; dy dy*dady

      fld    st(0)

      fmul   _tmap_data+7*8    ; dy*dbdy dy dy*dady
      fxch   st(2)

      fadd   ma                ; ma' dy dy*dbdy
      fxch   st(1)

      fmul   _tmap_data+8*8    ; dy*dcdy ma' dy*dbdy
      fxch   st(2)             ; dy*dbdy ma' dy*dcdy

      fadd   mb                ; mb' ma' dy*dcdy
      fxch   st(1)             ; ma' mb' dy*dcdy

      fstp   ma
      fxch   st(1)
      fadd   mc
      fxch   st(1)
      fstp   mb
      fstp   mc

rrua_next:
      mov    eax,span_end
      add    esi,4

      cmp    esi,eax
      jnz    rrua_loop_top

      pop    ebp
      pop    edi
      pop    esi

      ret

;
;  PT_ADD_UNCLIPPED_RUN_ASM
;
;   EAX = x0
;   EDX = x1
;   EBX = y
;
;  span: 2 bytes x
;        1 byte  len
;        1 byte  delta y

   _FUNCDEF pt_add_unclipped_run_asm,3

        push   ebp
        push   esi
        push   edi

        mov    esi,_slist_count
        mov    edi,_snext

        cmp    esi,0
        jz     aura_first      ;  if (slist_count)

        mov    ebp,_last_y
        mov    _last_y,ebx     ;  last_y = y

        sub    ebx,ebp

        mov    [edi-1],bl      ; snext[-1].dy = y - last_y
        add    ebx,ebp

        jmp    aura_cont

aura_first:
        mov    _start_y,ebx
        mov    _last_y,ebx

aura_cont:
        sub    edx,eax

        cmp    edx,254
        jle    aura_short

        mov    [edi],ax
        mov    al,255

        mov    2[edi],al
        mov    4[edi],dx

        mov    eax,_snext
        mov    ebx,_slist_count

        add    eax,8
        add    ebx,2

        mov    _snext,eax
        mov    _slist_count,ebx

        jmp    aura_done

aura_short:
        mov    [edi],ax
        mov    [edi+2],dl

        mov    eax,_snext
        mov    ebx,_slist_count

        add    eax,4
        add    ebx,1

        mov    _snext,eax
        mov    _slist_count,ebx

aura_done:

        pop    edi
        pop    esi
        pop    ebp

        ret


;
;  PT_ADD_CLIPPED_RUN_ASM
;
;    EAX = last    (RunClipData **)
;    EDX = x0
;    EBX = x1
;    ECX = y
;
;      0 start_x
;      2 end_x
;      4 next

  _FUNCDEF  pt_add_clipped_run_asm,4

        push   ebp
        push   esi

        push   edi
        mov    esi,[eax]     ; z = *last
        
        mov    edi,eax       ; last = &clip[y]
        mov    y_pos,ecx

        cmp    esi,0
        jz     acra_done     ; while (z)

acra_looptop:
        xor    eax,eax
        xor    ecx,ecx

        mov    ax,[esi]      ; eax = z->start_x

        cmp    ebx,eax       ; if (x1 <= z->start_x)
        jle    acra_done     ;      return

        mov    cx,2[esi]     ; ecx = z->end_x

        cmp    edx,ecx       ; if (x0 < z->end_x)
        jl     acra_visible

acra_next:
        lea    edi,4[esi]    ; last = &z->next
        mov    esi,4[esi]    ; z = z->next

        cmp    esi,0
        jnz    acra_looptop

acra_done:
        pop    edi
        pop    esi

        pop    ebp

        ret

acra_visible:
         ;  there are four cases:
         ;  we fill over it (and keep going)
         ;  we fit snug against the left
         ;  we fit snug against the right (and keep going)
         ;  we are in the middle

        cmp    edx,eax            ; if (x0 <= z->start_x)
        jg     acra_notleft

         ;  we fill up against the left side

        nop                  ; give the branch prediction a break
        nop

        cmp    ebx,ecx            ; if (x1 >= z->end_x)
        jl     acra_only_left

         ;  we totally fill this gap

        push   edx
        push   ebx

        mov    edx,ecx
        mov    ebx,y_pos
        call   pt_add_unclipped_run_asm_
        pop    ebx
        pop    edx
        ;add_unclipped_run(eax,ecx,y_pos);

         ;  delete the existing run
        mov    esi,4[esi]          ; z = z->next
        mov    [edi],esi           ; *last = z->next;

        cmp    esi,0
        jnz    acra_looptop

        jmp    acra_done

acra_only_left:
        mov    0[esi],bx            ; z->start_x = x1
        push   edx
        push   ebx
        mov    edx,ebx
        mov    ebx,y_pos
        call   pt_add_unclipped_run_asm_
        pop    ebx
        pop    edx
        ;add_unclipped_run(eax,ebx,y_pos);
        jmp    acra_done

acra_notleft:
        cmp    ebx,ecx
        jl     acra_inside

        mov    2[esi],dx            ; z->end_x = x0
        push   edx
        push   ebx
        mov    eax,edx
        mov    edx,ecx
        mov    ebx,y_pos
        call   pt_add_unclipped_run_asm_
        pop    ebx
        pop    edx
        ;add_unclipped_run(edx,ecx,y_pos);
        jmp    acra_next

acra_inside:
        mov    ebp,_clip_alloc
        mov    eax,4[esi]

        mov    0[ebp],bx
        mov    4[ebp],eax

        mov    4[esi],ebp           ; z->next = clip_alloc
        mov    2[ebp],cx

        mov    2[esi],dx           ; z->end_x = x0
        add    ebp,8               ; ++clip_alloc

        mov    _clip_alloc,ebp
        push   edx
        push   ebx
        mov    eax,edx
        mov    edx,ebx
        mov    ebx,y_pos
        call   pt_add_unclipped_run_asm_
        pop    ebx
        pop    edx
        ;add_unclipped_run(edx,ebx,y_pos);
        jmp    acra_done

_TEXT   ends

END
