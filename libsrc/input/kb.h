#ifndef __KB_H 
#define __KB_H 
#ifdef __cplusplus
extern "C"  {
#endif  
typedef struct kbs_event {
   uchar code;
   uchar state;
} kbs_event;
EXTERN kbs_event kb_null_event;
#ifdef __cplusplus
}
#endif  
#ifdef __cplusplus
extern "C"  {
#endif  
extern uchar kb_state(uchar code);
extern int kb_startup(void *init_buf);
extern int kb_shutdown(void);
extern kbs_event kb_next(void);
extern kbs_event kb_look_next(void);
extern void kb_flush(void);
extern uchar kb_get_state(uchar kb_code);
extern void kb_clear_state(uchar kb_code, uchar bits);
extern void kb_set_state(uchar kb_code, uchar bits);
extern void kb_set_signal(uchar code, uchar int_no);
extern int kb_get_flags();
extern void kb_set_flags(int flags);
extern void kb_generate(kbs_event e);
extern void kb_flush_bios(void);
extern bool kb_get_cooked(ushort* key);
EXTERN HANDLE kb_get_queue_available_signal(void);
#ifdef __cplusplus
}
#endif  
#ifdef __cplusplus
extern "C"  {
#endif  
#define KBC_SHIFT_PREFIX 0xe0
#define KBC_PAUSE_PREFIX 0xe1
#define KBC_PAUSE_DOWN   0xe11d
#define KBC_PAUSE_UP     0xe19d
#define KBC_PRSCR_DOWN   0x2a
#define KBC_PRSCR_UP     0xaa
#define KBC_PAUSE        0x7f
#define KBC_NONE         0xff
#define KBC_LSHIFT       0x2A
#define KBC_RSHIFT       0x36
#define KBC_LCTRL        0x1D
#define KBC_RCTRL        0x9D
#define KBC_LALT         0x38
#define KBC_RALT         0xB8
#define KBC_CAPS         0x3A
#define KBC_NUM          0x45
#define KBC_SCROLL       0x46
#define KBC_ACK          0xfa
#define KBC_RESEND       0xfe
#ifdef __cplusplus
}
#endif  
#ifdef __cplusplus
extern "C"  {
#endif  
#define KBM_SCROLL   0x0001
#define KBM_NUM      0x0002
#define KBM_CAPS     0x0004
#define KBM_LSHIFT   0x0008
#define KBM_RSHIFT   0x0010
#define KBM_LCTRL    0x0020
#define KBM_RCTRL    0x0040
#define KBM_LALT     0x0080
#define KBM_RALT     0x0100
#define KBM_LED_MASK 7
#define KBM_CAPS_SHF  2
#define KBM_SHIFT_SHF 3
#define KBM_CTRL_SHF  5
#define KBM_ALT_SHF   7
#ifdef __cplusplus
}
#endif  
#ifdef __cplusplus
extern "C"  {
#endif  
extern char *kbd_array;
#define KBA_STATE       1
#define KBA_REPEAT      2
#define KBA_SIGNAL      4
#ifdef __cplusplus
}
#endif  
#ifdef __cplusplus
extern "C"  {
#endif  
#define KBS_UP   0
#define KBS_DOWN 1
#ifdef __cplusplus
}
#endif  
#endif /* !__KB_H */
