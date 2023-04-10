; luke heapwalker, no doubt

MAX_HEAP_ELEM_SIZE EQU 003FFFFFFH                ; i wish this were smaller 
MAX_HEAP_VAL       EQU 003FFFFFFH

; can we learn about the main heap part?

.386p
                NAME    nheapwal
                EXTRN   ___nheapbeg :BYTE
DGROUP          GROUP   _DATA

_DATA           SEGMENT DWORD PUBLIC USE32 'DATA'
                public _oheap_max_elem_size
                public _oheap_max_val
_oheap_max_elem_size    dd      MAX_HEAP_ELEM_SIZE
_oheap_max_val          dd      MAX_HEAP_VAL
_DATA           ENDS

_TEXT           SEGMENT BYTE PUBLIC USE32 'CODE'
                ASSUME  CS:_TEXT ,DS:DGROUP,SS:DGROUP
                PUBLIC  _oheapwalk_ 
                PUBLIC  __OHeapWalk_ 

_oheapwalk_:    push    edx
                mov     edx,dword ptr ___nheapbeg
                call    near ptr __OHeapWalk_
                pop     edx
                ret     
__OHeapWalk_:   push    ebx
                mov     ebx,eax                         ; this is the heapwalk_info *
                test    edx,edx                         ;  is heapbeg zero?
                jne     short L1
                mov     eax,00000001H                   ; ok, no heap
                jmp     near ptr L8           
                                        ; eax+ebx:passed parameter  edx:heap beginning
L1:             mov     eax,dword ptr [eax]             ; load the passed in info *
                test    eax,eax                         ;  is null?
                jne     short L2
                lea     eax,0cH[edx]                    ; yep, start at beginning of heap
                jmp     short L4                        ;  eax now loaded correctly
L2:             mov     edx,dword ptr [eax]             ; get size of block
                and     dl,0feH                         ;  mask out used bit

                cmp     edx,_oheap_max_elem_size        ; do we care about sizes per se
                ja      L2_B_3                          ; that element was too big

                add     edx,eax                         ;  add back to block base
                cmp     edx,eax                         ; make sure it is above the first block
                jb      short L2_B_1

                sub     edx,dword ptr ___nheapbeg       ; check relative to start of heap
                cmp     edx,_oheap_max_val              ; are we past what we think is valid heap space...?
                ja      short L2_B_2
                add     edx,dword ptr ___nheapbeg
                jmp     L3
                
L2_B_1:         mov     eax,00000003H              
                jmp     short L2_B_END         
L2_B_2:         mov     eax,00000006H              
                jmp     short L2_B_END         
L2_B_3:         mov     eax,00000007H              
                jmp     short L2_B_END         
L2_B_4:         mov     eax,00000008H              
                jmp     short L2_B_END         

L2_B_END:       jmp     short L8

L3:             mov     eax,edx         ; eax:pointer to next block
L4:             cmp     dword ptr [eax],0ffffffffH      ; make sure size ! -1
                jne     short L6                        ;  go parse the block
                add     eax,00000004H                   ; following dword is either heap continue addr or zero
                cmp     dword ptr [eax],00000000H       ; if zero, we at end of the heap
                jne     short L5                        ; non-zero, skip to next block
                xor     eax,eax                         ; EOHeap, head home
                mov     dword ptr 0aH[ebx],eax
                mov     dword ptr 6H[ebx],eax
                mov     word ptr 4H[ebx],ax
                mov     dword ptr [ebx],eax
                mov     eax,00000004H                   ; we be done
                jmp     short L8
L5:             mov     eax,dword ptr [eax]             ; load next block in, continue

                sub     eax,dword ptr ___nheapbeg
                cmp     eax,MAX_HEAP_VAL                ; are we past what we think is valid heap space...?
                ja      L2_B_4
                add     eax,dword ptr ___nheapbeg

                jmp     short L4
L6:             mov     word ptr 4H[ebx],ds            ; save off our block
                mov     dword ptr [ebx],eax
                mov     dword ptr 0aH[ebx],00000001H   ; unused
                mov     edx,dword ptr [eax]
                and     dl,0feH
                mov     dword ptr 6H[ebx],edx          ; mask for size
                test    byte ptr [eax],01H             ;  test for 1 bit
                je      short L7
                mov     dword ptr 0aH[ebx],00000000H   ; wait, 1 bit set, we are used...
L7:             xor     eax,eax
L8:             pop     ebx
                ret     
_TEXT           ENDS
                END
