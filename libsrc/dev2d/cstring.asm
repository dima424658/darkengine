; $Header: x:/prj/tech/libsrc/dev2d/RCS/cstring.asm 1.3 1997/03/03 15:43:37 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc

assume ds:_DATA
_DATA segment
_DATA ends

_TEXT segment
punt:
        ret

; eax = dest, edx = src, ebx = count
_FUNCDEF memcpy_cache_dst, 3
        test    ebx,ebx
        jz      punt

        push    ebp
        push    esi

        push    edi
        push    ecx

        mov     edi, eax
        mov     esi, edx

        mov     ecx, ebx
        mov     bl, [edi]                        

        neg     eax                      
        cmp     ecx, 4                   

        mov     ebp, eax                 
        jb      L10                      

        and     eax, 3                   
        jz      L2                       

        sub     ecx, eax
                
L1:                               
        mov     ah, [esi]                
        inc     esi                      

        mov     [edi], ah                
        inc     edi                      

        dec     al                       
        jnz     L1                       

L2:                               
        and     ebp, 12                  
                
        cmp     ebp, ecx                 
                
        jae     L7                       
        sub     ecx, ebp                 

        shr     ebp, 2                   
        jz      L3                       
L20:                              
        mov     ebx, [esi]               
        add     esi, 4                   
        mov     [edi], ebx               
        add     edi, 4                   
        dec     ebp                      
        jnz     L20                      
L3:                               
        mov     eax, ecx                 
        and     ecx, 15                  
        shr     eax, 4                   
        jz      L7                       
        dec     eax                      
        jnz     L5                       
L4:                               
        mov     bl, [edi+ecx+15]         
        jmp     L6                       
                                    
;do aligned blocks of 16
L5:                               
        mov     bl, [edi+16]             
L6:                                 
        mov     ebx, [esi]               
        mov     edx, [esi+4]             
        mov     [edi], ebx               
        mov     [edi+4], edx              
        mov     ebx, [esi+8]             
        mov     edx, [esi+12]            
        mov     [edi+8], ebx             
        mov     [edi+12], edx             
        add     esi, 16                  
        add     edi, 16                  
        sub     eax, 1
        jg      L5                       
        jns     L4                       
L7:                                      
        mov     eax, ecx                 
        and     ecx, 3                   
        shr     eax, 2                   
        jz      L9                       
L8:                               
        mov     ebx, [esi]               
        add     esi, 4                   
        mov     [edi], ebx               
        add     edi, 4                   
        dec     eax                      
        jnz     L8                       
L9:                               
        test    ecx,ecx                  
        jz      L11                      
L10:                              
        mov     ch, [esi]                
        inc     esi                      
        mov     [edi], ch                
        inc     edi                      
        dec     cl                       
        jnz     L10                      
L11:
        pop     ecx
        pop     edi

        pop     esi
        pop     ebp
        ret

_TEXT ends
end

