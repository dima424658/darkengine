///////////////////////////////////////////////////////////////////////////////
// Portable Windows stack capture.

#include <types.h>
#include <stktrace.h>

#if defined(_WIN32)

#include <win32_platform.h>

int LGAPI FillStackArray(int Skip, int MaxFrames, void **p)
{
   USHORT captured;
   int i;

   if (p == NULL || MaxFrames <= 0)
      return 0;
   for (i = 0; i < MaxFrames; ++i)
      p[i] = NULL;

   /* Skip this wrapper in addition to the frames requested by the caller. */
   captured = CaptureStackBackTrace((ULONG)(Skip + 1), (ULONG)MaxFrames,
                                    p, NULL);
   return (int)captured;
}

int LGAPI FillThreadStackArray(HANDLE hThread, int Skip, int MaxFrames, void **p)
{
   CONTEXT context;
   DWORD threadId;
   int i;

   if (p == NULL || MaxFrames <= 0 || hThread == NULL)
      return 0;
   for (i = 0; i < MaxFrames; ++i)
      p[i] = NULL;

   threadId = GetThreadId(hThread);
   if (threadId == GetCurrentThreadId())
      return FillStackArray(Skip + 1, MaxFrames, p);

   ZeroMemory(&context, sizeof(context));
   context.ContextFlags = CONTEXT_CONTROL;
   if (!GetThreadContext(hThread, &context) || Skip > 0)
      return 0;

#if defined(_M_X64)
   p[0] = (void *)(ULONG_PTR)context.Rip;
#elif defined(_M_IX86)
   p[0] = (void *)(ULONG_PTR)context.Eip;
#elif defined(_M_ARM64)
   p[0] = (void *)(ULONG_PTR)context.Pc;
#else
   return 0;
#endif
   return 1;
}

#else

int FillStackArray(int Skip, int MaxFrames, void **p)
{
   return 0;
}

#endif
