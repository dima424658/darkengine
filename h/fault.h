#ifndef __FAULT_H 
#define  __FAULT_H 
#ifdef __cplusplus
extern "C"  {
#endif
extern int ex_install_handler(ubyte exc);
extern int ex_startup(ulong exc_mask);
extern int ex_uninstall_handler(uchar exc);
extern int ex_shutdown(void);
extern int ex_push_div_call(void *div_adr,void *dst_adr);
#ifdef __cplusplus
}
#endif
#define EXC_DIVIDE_ERR   0
#define EXC_INV_OPCODE   6
#define EXC_DOUBLE_FAULT 8
#define EXC_STACK_FAULT  0xc
#define EXC_GEN_FAULT    0xd
#define EXC_PAGE_FAULT   0xe
#define EXC_END          (EXC_PAGE_FAULT+1)
#define EXC_MAX          0x20
#define EXM_DIVIDE_ERR (1<<EXC_DIVIDE_ERR)
#define EXM_INV_OPCODE (1<<EXC_INV_OPCODE)
#define EXM_DOUBLE_FAULT (1<<EXC_DOUBLE_FAULT)
#define EXM_STACK_FAULT (1<<EXC_STACK_FAULT)
#define EXM_GEN_FAULT (1<<EXC_GEN_FAULT)
#define EXM_PAGE_FAULT (1<<EXC_PAGE_FAULT)
#define EXM_ALL (EXM_DIVIDE_ERR | EXM_INV_OPCODE | EXM_DOUBLE_FAULT | \
                 EXM_STACK_FAULT | EXM_GEN_FAULT | EXM_PAGE_FAULT)
#ifdef __cplusplus
extern "C"  {
#endif
extern void ex_divide_handler(void);
extern void ex_opcode_handler(void);
extern void ex_double_handler(void);
extern void ex_stack_handler(void);
extern void ex_general_handler(void);
extern void ex_page_handler(void);
extern void ex_divide_error(void);
extern void ex_divide_saturate(void);
extern void ex_reg_dump(void);
extern void exd_handlers_start(void);
extern void exd_handlers_end(void);
#define EXD_HANDLERS_SIZE ((ulong)&exd_handlers_end-(ulong)&exd_handlers_start)
#ifdef __cplusplus
}
#endif
#define EXA_DIV_ERROR ex_divide_error
#define EXA_DIV_HANDLE ex_divide_saturate
#define ex_set_div_action(m) (exd_div_action=(m))
#define ex_get_div_action() (exd_div_action)
extern void (*exd_div_action)(void);
#ifdef __cplusplus
extern "C"  {
#endif  
typedef struct {
   long eax,ebx,ecx,edx;
   long esi,edi,ebp;
   long cs,eip;
   long ss,esp;
   long ds,es,fs,gs;
   long eflags;
   long err,exc;
   long text,data;
} exs_state;
#ifdef __cplusplus
}
#endif 
#ifdef __cplusplus
extern "C"  {
#endif
extern exs_state exd_proc_state;
#ifdef DBG_ON
extern void ExdResetTable();
extern ulong* ExdGetTableInfo(int* numEntries);
#endif 
#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
extern "C"  {
#endif  
extern void ex_print_state(void);
#ifdef __cplusplus
}
#endif 
#endif /* __FAULT_H */ 