// $Header: x:/prj/tech/libsrc/dev2d/RCS/astring.h 1.4 1997/02/07 11:22:04 KEVIN Exp $

#ifndef __ASTRING_H
#define __ASTRING_H

#ifdef __cplusplus
extern "C" {
#endif

/* This routine is in dev2d.lib                        */
extern void memcpya (void *d, void *s, int n, void *a);
extern void memcpy_align_src (void *d, void *s, int n);
extern void memcpy_align_dst (void *d, void *s, int n);
extern void memcpy_cache_dst (void *d, void *s, int n);
extern void memcpy_by_byte (void *d, void *s, int n);

/* aligned memcpy (a is the address to which to align) */
#ifdef __WATCOMC__

/* macro upon which above is based */
void memcpya_ (void *d, void *s, int n, void *a);
#pragma aux memcpya_ =              \
   "cmp   ecx, 4"                   \
   "jle   L7"                       \
   "neg   eax"                      \
   "and   eax, 3"                   \
   "jz    L2"                       \
                                    \
/* do one at a time for alignment */\
   "sub   ecx, eax"                 \
"L1:"                               \
   "mov   ah, [esi]"                \
   "inc   esi"                      \
   "mov   [edi], ah"                \
   "inc   edi"                      \
   "dec   al"                       \
   "jnz   L1"                       \
"L2:"                               \
   "mov   eax, ecx"                 \
   "and   ecx, 3"                   \
   "shr   eax, 2"                   \
   "sub   eax, 4"                   \
   "jb    L4"                       \
                                    \
/* do aligned blocks of 16" */      \
"L3:"                               \
   "mov   ebx, [esi]"               \
   "mov   edx, [esi+4]"             \
   "mov   [edi], ebx"               \
   "mov   [edi+4],edx"              \
   "mov   ebx, [esi+8]"             \
   "mov   edx, [esi+12]"            \
   "mov   [edi+8], ebx"             \
   "mov   [edi+12],edx"             \
   "add   esi, 16"                  \
   "add   edi, 16"                  \
   "sub   eax, 4"                   \
   "jae   L3"                       \
"L4:"                               \
   "and   eax, 3"                   \
   "jz    L6"                       \
                                    \
/* do aligned blocks of 4 */        \
"L5:"                               \
   "mov   ebx, [esi]"               \
   "add   esi, 4"                   \
   "mov   [edi], ebx"               \
   "add   edi, 4"                   \
   "dec   eax"                      \
   "jnz   L5"                       \
"L6:"                               \
   "test  ecx,ecx"                  \
   "jz    L8"                       \
/* do one at a time cleanup */      \
"L7:"                               \
   "mov   ch, [esi]"                \
   "inc   esi"                      \
   "mov   [edi], ch"                \
   "inc   edi"                      \
   "dec   cl"                       \
   "jnz   L7"                       \
"L8:"                               \
   parm [edi] [esi] [ecx] [eax]     \
   modify [eax ebx ecx edx esi edi]
#else
__inline void memcpya_ (void *d, void *s, int n, void *a)
{
	__asm
	{
		mov		edi, d
		mov		esi, s
		mov		ecx, n
		mov		eax, a
		cmp		ecx, 4
		jle		L7
		neg		eax
		and		eax, 3
		jz		L2

// do one at a time for alignment
		sub		ecx, eax
	L1:
		mov		ah, [esi]
		inc		esi
		mov		[edi], ah
		inc		edi
		dec		al
		jnz		L1
	L2:
		mov		eax, ecx
		and		ecx, 3
		shr		eax, 2
		sub		eax, 4
		jb		L4

// do aligned blocks of 16
	L3:
		mov		ebx, [esi]
		mov		edx, [esi+4]
		mov		[edi], ebx
		mov		[edi+4], edx
		mov		ebx, [esi+8]
		mov		edx, [esi+12]
		mov		[edi+8], ebx
		mov		[edi+12], edx
		add		esi, 16
		add		edi, 16
		sub		eax, 4
		jae		L3
	L4:
		and		eax, 3
		jz		L6

// do aligned blocks of 4
	L5:
		mov		ebx, [esi]
		add		esi, 4
		mov		[edi], ebx
		add		edi, 4
		dec		eax
		jnz		L5
	L6:
		test	ecx, ecx
		jz		L8
// do one at a time cleanup
	L7:
		mov		ch, [esi]
		inc		esi
		mov		[edi], ch
		inc		edi
		dec		cl
		jnz		L7
	L8:
	}
}
#endif

#ifdef __cplusplus
}
#endif

#endif
