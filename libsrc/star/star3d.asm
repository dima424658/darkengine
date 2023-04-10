;
; $Source: r:/prj/lib/src/star/RCS/star3d.asm $
; $Revision: 1.1 $
; $Author: jaemz $
; $Date: 1994/10/24 23:27:46 $
;
; Star 3d routines
; 

.386
	option	oldstructs

	.nolist

        stereo_on       equ       1
	include	macros.inc
	include	lg.inc
	include	3d.inc
        include fault.inc
	.list
	include	2d.inc
	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'
	rcsid	"$Header: r:/prj/lib/src/star/RCS/star3d.asm 1.1 1994/10/24 23:27:46 jaemz Exp $"

ifdef star_spew
extd    _star_num_behind
extd    _star_num_projected
_star_num_behind        dd      0
_star_num_projected     dd      0
endif

extfix  _std_min_z     ; minimum z there
extfix  _std_max_rad   ; maximum radius of view ports
_std_min_z     fix     7fffffffh
_std_max_rad   fix     0

	align	4
_DATA	ends

_TEXT	segment dword public USE32 'CODE'

extn    star_transform_point


;matrix rotate and code a star point.  Project if clip codes
;are not set, rotate fully if in front of viewer
;(or smaller than a boundary
;takes esi=pointer to vector
;returns edi, point
star_transform_point:
        getpnt  edi     ;get free point
        mov     [edi].p3_flags,0

;third column (z)
	mov     eax,[esi]
	imul    vm3
	mov     ebx,eax
	mov     ecx,edx

	mov     eax,4[esi]
	imul    vm6
	add     ebx,eax
	adc     ecx,edx

	mov     eax,8[esi]
	imul    vm9
	add     eax,ebx
	adc     edx,ecx
	mfixup

        ; check out z, see if behind
        cmp     eax,_std_min_z ;ostensibly min plane
        js      star_behind

	mov     [edi].z,eax        ;save z

;first column (x)
	mov     eax,[esi]
	imul    vm1
	mov     ebx,eax
	mov     ecx,edx

	mov     eax,4[esi]
        imul    vm4
	add     ebx,eax
	adc     ecx,edx

	mov     eax,8[esi]
	imul    vm7
	add     eax,ebx
	adc     edx,ecx
	mfixup                           
	mov     [edi].x,eax        ;save x

;second column (y)
        mov     eax,[esi]
	imul    vm2
	mov     ebx,eax
	mov     ecx,edx

	mov     eax,4[esi]
	imul    vm5
	add     ebx,eax
	adc     ecx,edx

	mov     eax,8[esi]
	imul    vm8
	add     eax,ebx
	adc     edx,ecx
	mfixup
	mov     [edi].y,eax        ;save y

;call clip codes

	call     code_point

;transform if not clipped
        test    bl,bl
        jnz     star_done

        ifdef  star_spew
        inc     _star_num_projected
        endif

        jmp     g3_project_point        ; project and return

star_behind:
        mov     [edi].codes,CC_BEHIND

        ifdef  star_spew
        inc     _star_num_behind
        endif

star_done:
        ret     

_TEXT   ends

end





