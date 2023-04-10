/*
 * $Source: x:/prj/tech/winsrc/input/RCS/kbasctab.cpp $
 * $Revision: 1.10 $
 * $Author: KEVIN $
 * $Date: 1996/09/24 14:59:03 $
 *
 * Scancode to ASCII translation tables.
 *
 * This file is part of the input library.
 */

#include <kbcook.h>

#define ASCII(x) ((ushort)((x)|CNV_CTRL|CNV_ALT))
#define ALPHA(x) (ASCII(x)|CNV_CAPS)
#define SPECIAL(x) ((ushort)((x)|CNV_SPECIAL|CNV_CTRL|CNV_ALT|CNV_SHIFT))

ushort kb_cnv_table[KB_CNV_TBLSIZE][3] =  {
   /* scan code */  /* ascii */ /* shifted ascii */ /* right-alt ascii */
   // ---------------------------------------------------------------------
   /* 00 */    {        0                         ,                  0             , 0          },    // null
   /* 01 */    {       ASCII(0x1B)                ,          ASCII(0x1B)           , 0          },    // escape
   /* 02 */    {       ASCII('1' )                ,          ASCII('!' )           , 0          },   
   /* 03 */    {       ASCII('2' )                ,          ASCII('@' )           , 0          },   
   /* 04 */    {       ASCII('3' )                ,          ASCII('#' )           , 0          },   
   /* 05 */    {       ASCII('4' )                ,          ASCII('$' )           , 0          },   
   /* 06 */    {       ASCII('5' )                ,          ASCII('%' )           , 0          },   
   /* 07 */    {       ASCII('6' )                ,          ASCII('^' )           , 0          },   
   /* 08 */    {       ASCII('7' )                ,          ASCII('&' )           , 0          },   
   /* 09 */    {       ASCII('8' )                ,          ASCII('*' )           , 0          },   
   /* 0A */    {       ASCII('9' )                ,          ASCII('(' )           , 0          },   
   /* 0B */    {       ASCII('0' )                ,          ASCII(')' )           , 0          },   
   /* 0C */    {       ASCII('-' )                ,          ASCII('_' )           , 0          },   
   /* 0D */    {       ASCII('=' )                ,          ASCII('+' )           , 0          },   
   /* 0E */    {       ASCII(0x08)                ,          ASCII(0x08)           , 0          },   // backspace  
   /* 0F */    {       ASCII(0x09)                ,CNV_SHIFT|ASCII(0x09)           , 0          },   // tab
   /* 10 */    {       ALPHA('q' )                ,          ALPHA('Q' )           , 0          },   
   /* 11 */    {       ALPHA('w' )                ,          ALPHA('W' )           , 0          },   
   /* 12 */    {       ALPHA('e' )                ,          ALPHA('E' )           , 0          },   
   /* 13 */    {       ALPHA('r' )                ,          ALPHA('R' )           , 0          },   
   /* 14 */    {       ALPHA('t' )                ,          ALPHA('T' )           , 0          },   
   /* 15 */    {       ALPHA('y' )                ,          ALPHA('Y' )           , 0          },   
   /* 16 */    {       ALPHA('u' )                ,          ALPHA('U' )           , 0          },   
   /* 17 */    {       ALPHA('i' )                ,          ALPHA('I' )           , 0          },   
   /* 18 */    {       ALPHA('o' )                ,          ALPHA('O' )           , 0          },   
   /* 19 */    {       ALPHA('p' )                ,          ALPHA('P' )           , 0          },   
   /* 1A */    {       ASCII('[' )                ,          ASCII('{' )           , 0          },   
   /* 1B */    {       ASCII(']' )                ,          ASCII('}' )           , 0          },   
   /* 1C */    {       ASCII(0x0D)                ,          ASCII(0x0D)           , 0          },   // Enter
   /* 1D */    {     SPECIAL(0x1D)                ,        SPECIAL(0x1D)           , 0          },   // Ctrl
   /* 1E */    {       ALPHA('a' )                ,          ALPHA('A' )           , 0          },   
   /* 1F */    {       ALPHA('s' )                ,          ALPHA('S' )           , 0          },   
   /* 20 */    {       ALPHA('d' )                ,          ALPHA('D' )           , 0          },   
   /* 21 */    {       ALPHA('f' )                ,          ALPHA('F' )           , 0          },   
   /* 22 */    {       ALPHA('g' )                ,          ALPHA('G' )           , 0          },   
   /* 23 */    {       ALPHA('h' )                ,          ALPHA('H' )           , 0          },   
   /* 24 */    {       ALPHA('j' )                ,          ALPHA('J' )           , 0          },   
   /* 25 */    {       ALPHA('k' )                ,          ALPHA('K' )           , 0          },   
   /* 26 */    {       ALPHA('l' )                ,          ALPHA('L' )           , 0          },   
   /* 27 */    {       ASCII(';' )                ,          ASCII(':' )           , 0          },   
   /* 28 */    {       ASCII('\'')                ,          ASCII('"' )           , 0          },   
   /* 29 */    {       ASCII('`' )                ,          ASCII('~' )           , 0          },   
   /* 2A */    {     SPECIAL(0x2A)                ,        SPECIAL(0x2A)           , 0          },   // Lshift
   /* 2B */    {       ASCII('\\')                ,          ASCII('|' )           , 0          },   
   /* 2C */    {       ALPHA('z' )                ,          ALPHA('Z' )           , 0          },   
   /* 2D */    {       ALPHA('x' )                ,          ALPHA('X' )           , 0          },   
   /* 2E */    {       ALPHA('c' )                ,          ALPHA('C' )           , 0          },   
   /* 2F */    {       ALPHA('v' )                ,          ALPHA('V' )           , 0          },   
   /* 30 */    {       ALPHA('b' )                ,          ALPHA('B' )           , 0          },   
   /* 31 */    {       ALPHA('n' )                ,          ALPHA('N' )           , 0          },   
   /* 32 */    {       ALPHA('m' )                ,          ALPHA('M' )           , 0          },   
   /* 33 */    {       ASCII(',' )                ,          ASCII('<' )           , 0          },   
   /* 34 */    {       ASCII('.' )                ,          ASCII('>' )           , 0          },   
   /* 35 */    {       ASCII('/' )                ,          ASCII('?' )           , 0          },   
   /* 36 */    {     SPECIAL(0x36)                ,        SPECIAL(0x36)           , 0          },   // Rshift
   /* 37 */    {       ASCII('*' )|CNV_2ND        ,          ASCII('*' )|CNV_2ND   , 0          },   // Keypad *
   /* 38 */    {     SPECIAL(0x38)                ,        SPECIAL(0x38)           , 0          },   // Lalt
   /* 39 */    {       ASCII(' ' )                ,          ASCII(' ' )           , 0          },   // Space
   /* 3A */    {     SPECIAL(0x3A)                ,        SPECIAL(0x3A)           , 0          },   // Caps lock
   /* 3B */    {     SPECIAL(0x3B)                ,        SPECIAL(0x3B)           , 0          },   // F1
   /* 3C */    {     SPECIAL(0x3C)                ,        SPECIAL(0x3C)           , 0          },   
   /* 3D */    {     SPECIAL(0x3D)                ,        SPECIAL(0x3D)           , 0          },   
   /* 3E */    {     SPECIAL(0x3E)                ,        SPECIAL(0x3E)           , 0          },   
   /* 3F */    {     SPECIAL(0x3F)                ,        SPECIAL(0x3F)           , 0          },   
   /* 40 */    {     SPECIAL(0x40)                ,        SPECIAL(0x40)           , 0          },   
   /* 41 */    {     SPECIAL(0x41)                ,        SPECIAL(0x41)           , 0          },   
   /* 42 */    {     SPECIAL(0x42)                ,        SPECIAL(0x42)           , 0          },   
   /* 43 */    {     SPECIAL(0x43)                ,        SPECIAL(0x43)           , 0          },   
   /* 44 */    {     SPECIAL(0x44)                ,        SPECIAL(0x44)           , 0          },   // F10
   /* 45 */    {     SPECIAL(0x45)                ,        SPECIAL(0x45)           , 0          },   // Num lock
   /* 46 */    {     SPECIAL(0x46)                ,        SPECIAL(0x46)           , 0          },   // scroll lock
   /* 47 */    {       ASCII( '7')|CNV_NUM|CNV_2ND    ,ASCII('7' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   // keypad keys
   /* 48 */    {       ASCII( '8')|CNV_NUM|CNV_2ND    ,ASCII('8' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 49 */    {       ASCII( '9')|CNV_NUM|CNV_2ND    ,ASCII('9' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 4A */    {       ASCII( '-')|CNV_2ND        ,        ASCII('-' )|CNV_2ND    |CNV_SHIFT , 0 },   
   /* 4B */    {       ASCII( '4')|CNV_NUM|CNV_2ND    ,ASCII('4' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 4C */    {       ASCII( '5')|CNV_NUM|CNV_2ND    ,ASCII('5' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 4D */    {       ASCII( '6')|CNV_NUM|CNV_2ND    ,ASCII('6' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 4E */    {       ASCII( '+')|CNV_2ND        ,        ASCII('+' )|CNV_2ND    |CNV_SHIFT , 0 },   
   /* 4F */    {       ASCII( '1')|CNV_NUM|CNV_2ND    ,ASCII('1' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 50 */    {       ASCII( '2')|CNV_NUM|CNV_2ND    ,ASCII('2' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 51 */    {       ASCII( '3')|CNV_NUM|CNV_2ND    ,ASCII('3' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 52 */    {       ASCII( '0')|CNV_NUM|CNV_2ND    ,ASCII('0' )|CNV_NUM|CNV_SHIFT|CNV_2ND , 0 },   
   /* 53 */    {       ASCII( '.')|CNV_NUM|CNV_2ND,    ASCII('.' )|CNV_NUM|CNV_2ND|CNV_SHIFT , 0 },   
   /* 54 */    {     SPECIAL(0x54)                ,        SPECIAL(0x54)           , 0          },   
   /* 55 */    {     SPECIAL(0x55)                ,        SPECIAL(0x55)           , 0          },   
   /* 56 */    {     SPECIAL(0x56)                ,        SPECIAL(0x56)           , 0          },   
   /* 57 */    {     SPECIAL(0x57)                ,        SPECIAL(0x57)           , 0          },   
   /* 58 */    {     SPECIAL(0x58)                ,        SPECIAL(0x58)           , 0          },   
   /* 59 */    {     SPECIAL(0x59)                ,        SPECIAL(0x59)           , 0          },   
   /* 5A */    {     SPECIAL(0x5A)                ,        SPECIAL(0x5A)           , 0          },   
   /* 5B */    {     SPECIAL(0x5B)                ,        SPECIAL(0x5B)           , 0          },   
   /* 5C */    {     SPECIAL(0x5C)                ,        SPECIAL(0x5C)           , 0          },   
   /* 5D */    {     SPECIAL(0x5D)                ,        SPECIAL(0x5D)           , 0          },   
   /* 5E */    {     SPECIAL(0x5E)                ,        SPECIAL(0x5E)           , 0          },   
   /* 5F */    {     SPECIAL(0x5F)                ,        SPECIAL(0x5F)           , 0          },   
   /* 60 */    {     SPECIAL(0x60)                ,        SPECIAL(0x60)           , 0          },   
   /* 61 */    {     SPECIAL(0x61)                ,        SPECIAL(0x61)           , 0          },   
   /* 62 */    {     SPECIAL(0x62)                ,        SPECIAL(0x62)           , 0          },   
   /* 63 */    {     SPECIAL(0x63)                ,        SPECIAL(0x63)           , 0          },   
   /* 64 */    {     SPECIAL(0x64)                ,        SPECIAL(0x64)           , 0          },   
   /* 65 */    {     SPECIAL(0x65)                ,        SPECIAL(0x65)           , 0          },   
   /* 66 */    {     SPECIAL(0x66)                ,        SPECIAL(0x66)           , 0          },   
   /* 67 */    {     SPECIAL(0x67)                ,        SPECIAL(0x67)           , 0          },   
   /* 68 */    {     SPECIAL(0x68)                ,        SPECIAL(0x68)           , 0          },   
   /* 69 */    {     SPECIAL(0x69)                ,        SPECIAL(0x69)           , 0          },   
   /* 6A */    {     SPECIAL(0x6A)                ,        SPECIAL(0x6A)           , 0          },   
   /* 6B */    {     SPECIAL(0x6B)                ,        SPECIAL(0x6B)           , 0          },   
   /* 6C */    {     SPECIAL(0x6C)                ,        SPECIAL(0x6C)           , 0          },   
   /* 6D */    {     SPECIAL(0x6D)                ,        SPECIAL(0x6D)           , 0          },   
   /* 6E */    {     SPECIAL(0x6E)                ,        SPECIAL(0x6E)           , 0          },   
   /* 6F */    {     SPECIAL(0x6F)                ,        SPECIAL(0x6F)           , 0          },   
   /* 70 */    {     SPECIAL(0x70)                ,        SPECIAL(0x70)           , 0          },   
   /* 71 */    {     SPECIAL(0x71)                ,        SPECIAL(0x71)           , 0          },   
   /* 72 */    {     SPECIAL(0x72)                ,        SPECIAL(0x72)           , 0          },   
   /* 73 */    {     SPECIAL(0x73)                ,        SPECIAL(0x73)           , 0          },   
   /* 74 */    {     SPECIAL(0x74)                ,        SPECIAL(0x74)           , 0          },   
   /* 75 */    {     SPECIAL(0x75)                ,        SPECIAL(0x75)           , 0          },   
   /* 76 */    {     SPECIAL(0x76)                ,        SPECIAL(0x76)           , 0          },   
   /* 77 */    {     SPECIAL(0x77)                ,        SPECIAL(0x77)           , 0          },   
   /* 78 */    {     SPECIAL(0x78)                ,        SPECIAL(0x78)           , 0          },   
   /* 79 */    {     SPECIAL(0x79)                ,        SPECIAL(0x79)           , 0          },   
   /* 7A */    {     SPECIAL(0x7A)                ,        SPECIAL(0x7A)           , 0          },   
   /* 7B */    {     SPECIAL(0x7B)                ,        SPECIAL(0x7B)           , 0          },   
   /* 7C */    {     SPECIAL(0x7C)                ,        SPECIAL(0x7C)           , 0          },   
   /* 7D */    {     SPECIAL(0x7D)                ,        SPECIAL(0x7D)           , 0          },   
   /* 7E */    {     SPECIAL(0x7E)                ,        SPECIAL(0x7E)           , 0          },   
   /* 7F */    {     SPECIAL(0x7F)                ,        SPECIAL(0x7F)           , 0          },  // Pause 
   /* 80 */    {     SPECIAL(0x00)|CNV_2ND        ,        SPECIAL(0x00)|CNV_2ND          , 0          },   
   /* 81 */    {     SPECIAL(0x01)|CNV_2ND        ,        SPECIAL(0x01)|CNV_2ND          , 0          },   
   /* 82 */    {     SPECIAL(0x02)|CNV_2ND        ,        SPECIAL(0x02)|CNV_2ND          , 0          },   
   /* 83 */    {     SPECIAL(0x03)|CNV_2ND        ,        SPECIAL(0x03)|CNV_2ND          , 0          },   
   /* 84 */    {     SPECIAL(0x04)|CNV_2ND        ,        SPECIAL(0x04)|CNV_2ND          , 0          },   
   /* 85 */    {     SPECIAL(0x05)|CNV_2ND        ,        SPECIAL(0x05)|CNV_2ND          , 0          },   
   /* 86 */    {     SPECIAL(0x06)|CNV_2ND        ,        SPECIAL(0x06)|CNV_2ND          , 0          },    
   /* 87 */    {     SPECIAL(0x07)|CNV_2ND        ,        SPECIAL(0x07)|CNV_2ND          , 0          },   
   /* 88 */    {     SPECIAL(0x08)|CNV_2ND        ,        SPECIAL(0x08)|CNV_2ND          , 0          },   
   /* 89 */    {     SPECIAL(0x09)|CNV_2ND        ,        SPECIAL(0x09)|CNV_2ND          , 0          },   
   /* 8A */    {     SPECIAL(0x0A)|CNV_2ND        ,        SPECIAL(0x0A)|CNV_2ND          , 0          },   
   /* 8B */    {     SPECIAL(0x0B)|CNV_2ND        ,        SPECIAL(0x0B)|CNV_2ND          , 0          },   
   /* 8C */    {     SPECIAL(0x0C)|CNV_2ND        ,        SPECIAL(0x0C)|CNV_2ND          , 0          },   
   /* 8D */    {     SPECIAL(0x0D)|CNV_2ND        ,        SPECIAL(0x0D)|CNV_2ND          , 0          },   
   /* 8E */    {     SPECIAL(0x0E)|CNV_2ND        ,        SPECIAL(0x0E)|CNV_2ND          , 0          },   
   /* 8F */    {     SPECIAL(0x0F)|CNV_2ND        ,        SPECIAL(0x0F)|CNV_2ND          , 0          },   
   /* 90 */    {     SPECIAL(0x10)|CNV_2ND        ,        SPECIAL(0x10)|CNV_2ND          , 0          },   
   /* 91 */    {     SPECIAL(0x11)|CNV_2ND        ,        SPECIAL(0x11)|CNV_2ND          , 0          },   
   /* 92 */    {     SPECIAL(0x12)|CNV_2ND        ,        SPECIAL(0x12)|CNV_2ND          , 0          },   
   /* 93 */    {     SPECIAL(0x13)|CNV_2ND        ,        SPECIAL(0x13)|CNV_2ND          , 0          },   
   /* 94 */    {     SPECIAL(0x14)|CNV_2ND        ,        SPECIAL(0x14)|CNV_2ND          , 0          },   
   /* 95 */    {     SPECIAL(0x15)|CNV_2ND        ,        SPECIAL(0x15)|CNV_2ND          , 0          },   
   /* 96 */    {     SPECIAL(0x16)|CNV_2ND        ,        SPECIAL(0x16)|CNV_2ND          , 0          },   
   /* 97 */    {     SPECIAL(0x17)|CNV_2ND        ,        SPECIAL(0x17)|CNV_2ND          , 0          },   
   /* 98 */    {     SPECIAL(0x18)|CNV_2ND        ,        SPECIAL(0x18)|CNV_2ND          , 0          },   
   /* 99 */    {     SPECIAL(0x19)|CNV_2ND        ,        SPECIAL(0x19)|CNV_2ND          , 0          },   
   /* 9A */    {     SPECIAL(0x1A)|CNV_2ND        ,        SPECIAL(0x1A)|CNV_2ND          , 0          },   
   /* 9B */    {     SPECIAL(0x1B)|CNV_2ND        ,        SPECIAL(0x1B)|CNV_2ND          , 0          },   
   /* 9C */    {       ASCII(0x0D)|CNV_2ND        ,          ASCII(0x0D)|CNV_2ND   , 0          },  // Keypad enter
   /* 9D */    {     SPECIAL(0x1D)|CNV_2ND        ,        SPECIAL(0x1D)|CNV_2ND          , 0          },   
   /* 9E */    {     SPECIAL(0x1E)|CNV_2ND        ,        SPECIAL(0x1E)|CNV_2ND          , 0          },   
   /* 9F */    {     SPECIAL(0x1F)|CNV_2ND        ,        SPECIAL(0x1F)|CNV_2ND          , 0          },   
   /* A0 */    {     SPECIAL(0x20)|CNV_2ND        ,        SPECIAL(0x20)|CNV_2ND          , 0          },   
   /* A1 */    {     SPECIAL(0x21)|CNV_2ND        ,        SPECIAL(0x21)|CNV_2ND          , 0          },   
   /* A2 */    {     SPECIAL(0x22)|CNV_2ND        ,        SPECIAL(0x22)|CNV_2ND          , 0          },   
   /* A3 */    {     SPECIAL(0x23)|CNV_2ND        ,        SPECIAL(0x23)|CNV_2ND          , 0          },   
   /* A4 */    {     SPECIAL(0x24)|CNV_2ND        ,        SPECIAL(0x24)|CNV_2ND          , 0          },   
   /* A5 */    {     SPECIAL(0x25)|CNV_2ND        ,        SPECIAL(0x25)|CNV_2ND          , 0          },   
   /* A6 */    {     SPECIAL(0x26)|CNV_2ND        ,        SPECIAL(0x26)|CNV_2ND          , 0          },    
   /* A7 */    {     SPECIAL(0x27)|CNV_2ND        ,        SPECIAL(0x27)|CNV_2ND          , 0          },   
   /* A8 */    {     SPECIAL(0x28)|CNV_2ND        ,        SPECIAL(0x28)|CNV_2ND          , 0          },   
   /* A9 */    {     SPECIAL(0x29)|CNV_2ND        ,        SPECIAL(0x29)|CNV_2ND          , 0          },   
   /* AA */    {     SPECIAL(0x2A)|CNV_2ND        ,        SPECIAL(0x2A)|CNV_2ND          , 0          },   
   /* AB */    {     SPECIAL(0x2B)|CNV_2ND        ,        SPECIAL(0x2B)|CNV_2ND          , 0          },   
   /* AC */    {     SPECIAL(0x2C)|CNV_2ND        ,        SPECIAL(0x2C)|CNV_2ND          , 0          },   
   /* AD */    {     SPECIAL(0x2D)|CNV_2ND        ,        SPECIAL(0x2D)|CNV_2ND          , 0          },   
   /* AE */    {     SPECIAL(0x2E)|CNV_2ND        ,        SPECIAL(0x2E)|CNV_2ND          , 0          },   
   /* AF */    {     SPECIAL(0x2F)|CNV_2ND        ,        SPECIAL(0x2F)|CNV_2ND          , 0          },   
   /* B0 */    {     SPECIAL(0x30)|CNV_2ND        ,        SPECIAL(0x30)|CNV_2ND          , 0          },   
   /* B1 */    {     SPECIAL(0x31)|CNV_2ND        ,        SPECIAL(0x31)|CNV_2ND          , 0          },   
   /* B2 */    {     SPECIAL(0x32)|CNV_2ND        ,        SPECIAL(0x32)|CNV_2ND          , 0          },   
   /* B3 */    {     SPECIAL(0x33)|CNV_2ND        ,        SPECIAL(0x33)|CNV_2ND          , 0          },   
   /* B4 */    {     SPECIAL(0x34)|CNV_2ND        ,        SPECIAL(0x34)|CNV_2ND          , 0          },   
   /* B5 */    {     ASCII('/')|CNV_2ND           ,        ASCII('/')|CNV_2ND          , 0          },   
   /* B6 */    {     SPECIAL(0x36)|CNV_2ND        ,        SPECIAL(0x36)|CNV_2ND      , 0          },  // keypad divide  
   /* B7 */    {     SPECIAL(0x37)|CNV_2ND        ,        SPECIAL(0x37)|CNV_2ND          , 0          },   
   /* B8 */    {     SPECIAL(0x38)|CNV_2ND        ,        SPECIAL(0x38)|CNV_2ND          , 0          },   
   /* B9 */    {     SPECIAL(0x39)|CNV_2ND        ,        SPECIAL(0x39)|CNV_2ND          , 0          },   
   /* BA */    {     SPECIAL(0x3A)|CNV_2ND        ,        SPECIAL(0x3A)|CNV_2ND          , 0          },   
   /* BB */    {     SPECIAL(0x3B)|CNV_2ND        ,        SPECIAL(0x3B)|CNV_2ND          , 0          },   
   /* BC */    {     SPECIAL(0x3C)|CNV_2ND        ,        SPECIAL(0x3C)|CNV_2ND          , 0          },   
   /* BD */    {     SPECIAL(0x3D)|CNV_2ND        ,        SPECIAL(0x3D)|CNV_2ND          , 0          },   
   /* BE */    {     SPECIAL(0x3E)|CNV_2ND        ,        SPECIAL(0x3E)|CNV_2ND          , 0          },   
   /* BF */    {     SPECIAL(0x3F)|CNV_2ND        ,        SPECIAL(0x3F)|CNV_2ND          , 0          },   
   /* C0 */    {     SPECIAL(0x40)|CNV_2ND        ,        SPECIAL(0x40)|CNV_2ND          , 0          },   
   /* C1 */    {     SPECIAL(0x41)|CNV_2ND        ,        SPECIAL(0x41)|CNV_2ND          , 0          },   
   /* C2 */    {     SPECIAL(0x42)|CNV_2ND        ,        SPECIAL(0x42)|CNV_2ND          , 0          },   
   /* C3 */    {     SPECIAL(0x43)|CNV_2ND        ,        SPECIAL(0x43)|CNV_2ND          , 0          },   
   /* C4 */    {     SPECIAL(0x44)|CNV_2ND        ,        SPECIAL(0x44)|CNV_2ND          , 0          },   
   /* C5 */    {     SPECIAL(0x45)|CNV_2ND        ,        SPECIAL(0x45)|CNV_2ND          , 0          },   
   /* C6 */    {     SPECIAL(0x46)|CNV_2ND        ,        SPECIAL(0x46)|CNV_2ND          , 0          },    
   /* C7 */    {     SPECIAL(0x47)|CNV_2ND        ,        SPECIAL(0x47)|CNV_2ND          , 0          },   
   /* C8 */    {     SPECIAL(0x48)|CNV_2ND        ,        SPECIAL(0x48)|CNV_2ND          , 0          },   
   /* C9 */    {     SPECIAL(0x49)|CNV_2ND        ,        SPECIAL(0x49)|CNV_2ND          , 0          },   
   /* CA */    {     SPECIAL(0x4A)|CNV_2ND        ,        SPECIAL(0x4A)|CNV_2ND          , 0          },   
   /* CB */    {     SPECIAL(0x4B)|CNV_2ND        ,        SPECIAL(0x4B)|CNV_2ND          , 0          },   
   /* CC */    {     SPECIAL(0x4C)|CNV_2ND        ,        SPECIAL(0x4C)|CNV_2ND          , 0          },   
   /* CD */    {     SPECIAL(0x4D)|CNV_2ND        ,        SPECIAL(0x4D)|CNV_2ND          , 0          },   
   /* CE */    {     SPECIAL(0x4E)|CNV_2ND        ,        SPECIAL(0x4E)|CNV_2ND          , 0          },   
   /* CF */    {     SPECIAL(0x4F)|CNV_2ND        ,        SPECIAL(0x4F)|CNV_2ND          , 0          },   
   /* D0 */    {     SPECIAL(0x50)|CNV_2ND        ,        SPECIAL(0x50)|CNV_2ND          , 0          },   
   /* D1 */    {     SPECIAL(0x51)|CNV_2ND        ,        SPECIAL(0x51)|CNV_2ND          , 0          },   
   /* D2 */    {     SPECIAL(0x52)|CNV_2ND        ,        SPECIAL(0x52)|CNV_2ND          , 0          },   
   /* D3 */    {     SPECIAL(0x53)|CNV_2ND        ,        SPECIAL(0x53)|CNV_2ND          , 0          },   
   /* D4 */    {     SPECIAL(0x54)|CNV_2ND        ,        SPECIAL(0x54)|CNV_2ND          , 0          },   
   /* D5 */    {     SPECIAL(0x55)|CNV_2ND        ,        SPECIAL(0x55)|CNV_2ND          , 0          },   
   /* D6 */    {     SPECIAL(0x56)|CNV_2ND        ,        SPECIAL(0x56)|CNV_2ND          , 0          },    
   /* D7 */    {     SPECIAL(0x57)|CNV_2ND        ,        SPECIAL(0x57)|CNV_2ND          , 0          },   
   /* D8 */    {     SPECIAL(0x58)|CNV_2ND        ,        SPECIAL(0x58)|CNV_2ND          , 0          },   
   /* D9 */    {     SPECIAL(0x59)|CNV_2ND        ,        SPECIAL(0x59)|CNV_2ND          , 0          },   
   /* DA */    {     SPECIAL(0x5A)|CNV_2ND        ,        SPECIAL(0x5A)|CNV_2ND          , 0          },   
   /* DB */    {     SPECIAL(0x5B)|CNV_2ND        ,        SPECIAL(0x5B)|CNV_2ND          , 0          },   
   /* DC */    {     SPECIAL(0x5C)|CNV_2ND        ,        SPECIAL(0x5C)|CNV_2ND          , 0          },   
   /* DD */    {     SPECIAL(0x5D)|CNV_2ND        ,        SPECIAL(0x5D)|CNV_2ND          , 0          },   
   /* DE */    {     SPECIAL(0x5E)|CNV_2ND        ,        SPECIAL(0x5E)|CNV_2ND          , 0          },   
   /* DF */    {     SPECIAL(0x5F)|CNV_2ND        ,        SPECIAL(0x5F)|CNV_2ND          , 0          }   
};
































