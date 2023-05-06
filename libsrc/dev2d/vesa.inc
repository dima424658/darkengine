;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/vesa.inc $
; $Revision: 1.1 $
; $Author: KEVIN $
; $Date: 1996/04/10 16:26:52 $
;
; Constants and declarations for VESA.
;
; This file is part of the 2d library.
;

ifndef vesa_inc
vesa_inc equ 1

;
; We begin with the the equates for the VBE functions themselves
; together with the version number with which each is associated
;

VBE_DETECT                      equ     4F00h           ; 1.0
VBE_GET_MODE_INFO               equ     4F01h           ; 1.0
VBE_SET_MODE                    equ     4F02h           ; 1.0
VBE_GET_MODE                    equ     4F03h           ; 1.0
VBE_STATE_FUNC                  equ     4F04h           ; 1.0
        VBE_STATE_SIZE          equ        0h           ; 1.0
        VBE_SAVE_STATE          equ        1h           ; 1.0
        VBE_RESTORE_STATE       equ        2h           ; 1.0
VBE_BANK_FUNC                   equ     4F05h           ; 1.0
        VBE_SET_BANK            equ        0h           ; 1.0
        VBE_GET_BANK            equ        1h           ; 1.0
VBE_SCAN_LINE_FUNC              equ     4F06h           ; 1.1
        VBE_SET_PIXEL_W         equ        0h           ; 1.1
        VBE_GET_SCAN_LEN        equ        1h           ; 1.1
        VBE_SET_BYTE_W          equ        2h           ; 2.0
        VBE_GET_MAX_LEN         equ        3h           ; 2.0
VBE_DISP_START_FUNC             equ     4F07h           ; 1.1
        VBE_SET_START           equ        0h           ; 1.1
        VBE_GET_START           equ        1h           ; 1.1
VBE_DAC_WIDTH_FUNC              equ     4F08h           ; 1.2
        VBE_SET_DAC_W           equ        0h           ; 1.2
        VBE_GET_DAC_W           equ        1h           ; 1.2
VBE_PAL_FUNC                    equ     4F09h           ; 2.0
        VBE_SET_PAL             equ        0h           ; 2.0
        VBE_GET_PAL             equ        1h           ; 2.0
        VBE_SET_2ND_PAL         equ        2h           ; 2.0
        VBE_GET_2ND_PAL         equ        3h           ; 2.0
VBE_GET_FUNCS                   equ     4F0Ah           ; 2.0
        VBE_GET_SETBANK         equ        1h           ; 2.0
        VBE_GET_SETDISP         equ        2h           ; 2.0
        VBE_GET_SETPAL          equ        3h           ; 2.0

VBE_RET_OK                      equ     004fh

; Flags for the save/restore state function

VBE_STATE_HARDWARE      equ     0001h
VBE_STATE_BIOS          equ     0002h
VBE_STATE_DAC           equ     0004h
VBE_STATE_SVGA          equ     0008h
VBE_STATE_ALL           equ     000Fh

; Flags for the mode setting function

VBE_DONT_CLEAR          equ     8000h           ; don't clear display memory
VBE_DONT_CLEAR_SHIFT    equ       0Fh           ; shift 15 for don't clear bit
VBE_LINEAR_BUFFER       equ     4000h           ; enable linear frame buffer
VBE_LINEAR_BUFF_SHIFT   equ       0Eh           ; shift 14 for linear buff bit

; The following can be OR'd with either VBE_SET_START or
; VBE_SET_PAL to make sure these two are set only on retrace.

VBE_WAIT_VRT            equ     0080h           ; VBE 2.0
VBE_WAIT_VRT_SHIFT      equ     7h              ; bit position of wait bit

;
; Now for the structure definitions
;

VBE_VESA_SIG            equ     000000000h      ; 'VESA' signature
VBE_VESA_VERSION        equ     000000004h      ; VBE version number
VBE_OEM_STRING          equ     000000006h      ; pointer to OEM string
VBE_CAPABILITIES        equ     00000000Ah      ; capabilities of video card
VBE_VIDEO_MODE_LIST     equ     00000000Eh      ; pointer to mode list
; 1.1 extension
VBE_TOTAL_MEMORY        equ     000000012h      ; number of 64Kb blocks
; 2.0 extensions
VBE_OEM_SOFTWARE_REV    equ     000000014h      ; OEM software revision number
VBE_OEM_VENDOR_NAME     equ     000000016h      ; pointer to vendor name str
VBE_OEM_PRODUCT_NAME    equ     00000001Ah      ; pointer to product name str
VBE_OEM_PRODUCT_REV     equ     00000001Eh      ; pointer to product revision str
VBE_RESERVED            equ     000000022h      ; start of 222 byte reserved area
VBE_OEM_DATA            equ     000000100h      ; start of scratch area for OEM data
VBE_INFO_SIZE           equ     000000200h      ; total size of info (if 2.0)

; Capabilities field flags

VBE_8_BIT_DAC           equ     000000001h      ; DAC width can be made 8 bit
; 2.0 extensions 
VBE_NON_VGA             equ     000000002h      ; Controller is non-VGA
VBE_BLANK_RAM_DAC       equ     000000004h      ; Programmed DAC with blank bit

;
; Next the offsets of the data in the 256 byte structure returned
; by the VBE_MODE_INFO function
;

VBE_MODE_ATTRIBUTES     equ     000000000h      ; mode attributes
VBE_WIN_A_ATTRIBUTES    equ     000000002h      ; window A attributes
VBE_WIN_B_ATTRIBUTES    equ     000000003h      ; window B attributes
VBE_WIN_GRANULARITY     equ     000000004h      ; window granularity in Kb
VBE_WIN_SIZE            equ     000000006h      ; window size in Kb
VBE_WIN_A_SEGMENT       equ     000000008h      ; window A segment
VBE_WIN_B_SEGMENT       equ     00000000Ah      ; window B segment
VBE_WIN_FUNC_PTR        equ     00000000Ch      ; pointer to window function
VBE_BYTES_PER_SCAN_LINE equ     000000010h      ; bytes per scanline
VBE_X_RESOLUTION        equ     000000012h      ; horizontal resolution
VBE_Y_RESOLUTION        equ     000000014h      ; vertical resolution
VBE_X_CHAR_SIZE         equ     000000016h      ; character cell width
VBE_Y_CHAR_SIZE         equ     000000017h      ; character cell height
VBE_NUMBER_OF_PLANES    equ     000000018h      ; number of memory planes
VBE_BITS_PER_PIXEL      equ     000000019h      ; bits per pixel
VBE_NUMBER_OF_BANKS     equ     00000001Ah      ; number of CGA style banks
VBE_MEMORY_MODEL        equ     00000001Bh      ; memory model type
VBE_BANK_SIZE           equ     00000001Ch      ; size of CGA style banks

; VBE 1.1 extensions

VBE_NUM_IMAGE_PAGES     equ     00000001Dh      ; number of image pages
VBE_BYTE_RESERVED       equ     00000001Eh      ; reserved byte (for page function)

; VBE 1.2 extensions

VBE_RED_MASK_SIZE       equ     00000001Fh      ; size of direct color red mask
VBE_RED_FIELD_POS       equ     000000020h      ; bit posn of lsb of red mask
VBE_GREEN_MASK_SIZE     equ     000000021h      ; size of direct color green mask
VBE_GREEN_FIELD_POS     equ     000000022h      ; bit posn of lsb of green mask
VBE_BLUE_MASK_SIZE      equ     000000023h      ; size of direct color blue mask
VBE_BLUE_FIELD_POS      equ     000000024h      ; bit posn of lsb of blue mask
VBE_RSVD_MASK_SIZE      equ     000000025h      ; size of direct color res mask
VBE_RSVD_FIELD_POS      equ     000000026h      ; bit posn of lsb of res mask
VBE_DIRET_COLOR_INFO    equ     000000027h      ; direct color mode attributes

; VBE 2.0 extensions

VBE_PHYSICAL_BASE_PTR   equ     000000028h      ; physical address for linear buf
VBE_OFF_SCREEN_MEM_OFF  equ     00000002Ch      ; pointer to start of offscreen mem
VBE_OFF_SCREEN_MEM_SIZE equ     000000030h      ; amount of offscreen mem in Kb
VBE_INFO_PAD_BLOCK      equ     000000032h      ; pad block to 256 bytes
VBE_MODE_INFO_SIZE      equ     000000100h      ; size of mode info block

;
; Next come defines for some of the information in the above block
;

; First the mode attributes (bit 1 reserved)

VBE_MODE_SUPPORTED      equ     000000001h      ; video mode is supported
VBE_BIOS_TTY_OUTPUT     equ     000000004h      ; tty output supported by BIOS
VBE_COLOR_MODE          equ     000000008h      ; mode is a color mode
VBE_GRAPHICS_MODE       equ     000000010h      ; mode is a graphics mode
; 2.0 extensions
VBE_NON_VGA_MODE        equ     000000020h      ; mode is not VGA compatible
VBE_NON_BANK_MODE       equ     000000040h      ; mode does not support banking
VBE_MODE_LINEAR         equ     000000080h      ; linear mode is supported

; WinA and WinB attributes

VBE_WIN_SUPPORTED       equ     000000001h
VBE_WIN_READABLE        equ     000000002h
VBE_WIN_WRITABLE        equ     000000003h
VBE_WIN_ALL_ATTR        equ     000000007h      ; can read/write window

; Direct color mode info

VBE_RAMP_CHANGES        equ     000000001h      ; indicates color ramp is programmable
VBE_COLOR_RSVD_AVAIL    equ     000000002h      ; indicates that rsvd bits avail

; Memory model identification

VBE_TEXT_MEMORY         equ     000000000h      ; text mode memory model
VBE_CGA_MEMORY          equ     000000001h      ; CGA style mode
VBE_HERCULES_MEMORY     equ     000000002h      ; hercules graphics mode
VBE_PLANAR_MEMORY       equ     000000003h      ; 16 color VGA planar mode
VBE_PACKED_MEMORY       equ     000000004h      ; packed pixel mode (256 color)
VBE_MODEX_MEMORY        equ     000000005h      ; non-chain 4, 256 color (mode x)
VBE_RGB_MEMORY          equ     000000006h      ; direct color RGB mode
VBE_YUV_MEMORY          equ     000000007h      ; direct color YUV mode

;
; The VBE 2.0 palette format (the same as used by Windows but different
; from the one used by normal VGA BIOS)
;

VBE_PALETTE_BLUE        equ     000000000h      ; one byte (or 6 bits) blue
VBE_PALETTE_GREEN       equ     000000001h      ; one byte (or 6 bits) green
VBE_PALETTE_RED         equ     000000002h      ; one byte (or 6 bits) red
VBE_PALETTE_ALPHA       equ     000000003h      ; alpha/alignment

endif   ;ndef vesa_inc

