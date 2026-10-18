//
// 

#ifndef __NEC98_H
#define __NEC98_H

//
// The ScanKey codes on the NEC PC98 are different than a regular
// PC keyboard.  So, when a PC98 keycode is fed to this table, the
// corresponding regular code is obtained. For instance, on the NEC
// keyboard a ESC generates a 0. And at 0 in our table here is 
// a 0x01 which is the ESC code on a PC-101 board. 



int nec2pc[]={
0x01,					// ESC          0
0x02,					// 1            1
0x03,					// 2            2
0x04,					// 3            3
0x05,					// 4            4
0x06,					// 5            5
0x07,					// 6            6
0x08,					// 7            7
0x09,					// 8            8
0x0a,                                   // 9            9
0x0b,					// 0            10
0x0c,					// -=           11
0,			     	        // ^    <<<<    12
0x2B,					// BackSlash    13
0x0e,					// BS           14
0x0f,					// TAB          15
0x10,					// Q            16
0x11,					// W            17
0x12,					// E            18
0x13,					// R            19
0x14,					// T            20
0x15,					// Y            21
0x16,					// U            22
0x17,					// I            23
0x18,					// O            24
0x19,					// P            25
0x29,					// @~   <<<     26
0x1a,					// [            27
0x1c,					// Enter        28
0x1e,					// A            29
0x1f,					// S            30
0x20,					// D            31
0x21,					// F            32 
0x22,					// G            33
0x23,					// H            34
0x24,					// J            35
0x25,					// K            36
0x26,					// L            37
0x27,					// ;+           38
0x28,					// :*   <<<     39 
0x1b,					// ]            40
0x2c,					// Z            41
0x2d,					// X            42
0x2e,					// C            43
0x2f,					// V            44
0x30,					// B            45
0x31,					// N            46
0x32,					// M            47
0x33,					// ,<           48
0x34,					// .>           49
0x35,					// ?            50
0,					// _    <<<     51
0x39,					// Space        52
0,					// XFER         53
0xd1,					// ROLL_UP      54
0xc9,					// ROLL_DOWN    55
0x52,					// INS
0x53,					// DEL
0xc8,					// Up_Arrow
0xcb,					// Left_Arrow
0xcd,					// Right_Arrow
0xd0,					// Down_Arrow
0xc7,					// HOME_CLR
0x4f,					// HELP
0x4a,					// -
0x46,					// /
0x47,					// 7
0x48,					// 8
0x49,					// 9
0x37,					// *
0x4b,					// 4
0x4c,					// 5
0x4d,					// 6
0x4e,					// +
0x4f,					// 1
0x50,					// 2
0x51,					// 3
0,					// =
0x52,					// 0
0,					// ,
0x53,					// .
0,					// NFER
0x57,					// f11
0x58,					// f12
0,					// f13
0,					// f14
0,					// f15
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// HOME
0,					// 
0,                                      // STOP
0,                                      // COPY
0x3b,					// f1
0x3c,					// f2
0x3d,					// f3
0x3e,					// f4
0x3f,					// f5
0x40,					// f6
0x41,					// f7
0x42,					// f8
0x43,					// f9
0x44,					// f10
0,					// 
0,					// 
0,					// 
0,					// 
0x2a,					// SHIFT
0x3a,					// CAPS
0,					// KANA
0x38,					// GRPH
0x1d,					// CTRL
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
0,					// 
};



#endif // __NEC98_H

/*  
There is no documented way to detect the manufacturer of the computer that
is currently running an application. However, a Windows-based application
can detect the type of OEM Windows by using the return value of the
GetKeyboardType() function.
 
If an application uses the GetKeyboardType API, it can get OEM ID by
specifying "1" (keyboard subtype) as argument of the function. Each OEM ID
is listed here:
 
   OEM Windows       OEM ID
   ------------------------------
   Microsoft         00H (DOS/V)
   all AX            01H
   EPSON             04H
   Fujitsu           05H
   IBMJ              07H
   Matsushita        0AH
   NEC               0DH
   Toshiba           12H
 


* NEC's PC98 have No Right-Shift,Right-Control,Right-Alt,
    It's same as Left-Shift,Left-Control,Left-Alt.
    also there is no PrintScrn,ScrollLock,Pause,and NumLock.

    */
