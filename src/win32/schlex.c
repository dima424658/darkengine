/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\lex -l -o win32\schlex.c c:\prj\cam\src\schlex.lex */
#define YYNEWLINE 10
#define INITIAL 0
#define PREPROC 2
#define COMMENT 4
#define yy_endst 240
#define yy_nxtmax 995
#define YY_LA_SIZE 30

static unsigned short yy_la_act[] = {
 0, 47, 47, 39, 47, 39, 47, 39, 47, 39, 47, 39, 47, 39, 47, 39,
 47, 39, 47, 39, 47, 39, 47, 39, 47, 39, 47, 39, 47, 47, 47, 41,
 42, 47, 47, 43, 47, 44, 47, 45, 47, 46, 47, 47, 42, 41, 42, 42,
 42, 41, 42, 40, 39, 39, 39, 39, 39, 39, 34, 39, 39, 39, 39, 39,
 39, 39, 39, 39, 33, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39,
 39, 39, 38, 39, 39, 39, 39, 39, 39, 39, 32, 39, 39, 39, 39, 39,
 39, 39, 39, 39, 39, 39, 37, 39, 39, 39, 39, 39, 39, 39, 39, 39,
 36, 39, 39, 39, 29, 39, 39, 39, 39, 39, 28, 39, 39, 23, 39, 39,
 39, 39, 24, 39, 39, 39, 39, 39, 39, 22, 39, 39, 39, 39, 39, 39,
 39, 27, 39, 39, 39, 39, 39, 39, 39, 20, 39, 39, 39, 39, 39, 39,
 39, 39, 39, 39, 39, 30, 39, 39, 39, 39, 39, 39, 39, 18, 39, 39,
 39, 39, 39, 39, 39, 39, 39, 39, 39, 35, 39, 39, 39, 39, 39, 39,
 39, 21, 39, 39, 39, 39, 39, 39, 16, 39, 14, 39, 39, 39, 39, 39,
 39, 15, 39, 39, 39, 39, 13, 39, 39, 39, 39, 39, 25, 39, 39, 39,
 12, 39, 39, 39, 39, 39, 19, 39, 39, 17, 39, 39, 39, 11, 39, 39,
 39, 39, 39, 39, 31, 39, 39, 39, 39, 10, 39, 39, 39, 39, 39, 39,
 26, 39, 1, 0, 47, 4, 3, 5, 6, 7, 9, 8, 6, 2,
};

static unsigned char yy_look[] = {
 0
};

static short yy_final[] = {
 0, 0, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27,
 29, 30, 31, 34, 35, 37, 39, 41, 43, 44, 45, 47, 48, 49, 51, 51,
 52, 53, 54, 55, 56, 57, 58, 60, 61, 62, 63, 64, 65, 66, 67, 68,
 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85, 86,
 87, 88, 89, 90, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 104,
 105, 106, 107, 108, 109, 110, 111, 112, 114, 115, 116, 118, 119, 120, 121, 122,
 124, 125, 127, 128, 129, 130, 132, 133, 134, 135, 136, 137, 139, 140, 141, 142,
 143, 144, 145, 147, 148, 149, 150, 151, 152, 153, 155, 156, 157, 158, 159, 160,
 161, 162, 163, 164, 165, 167, 168, 169, 170, 171, 172, 173, 175, 176, 177, 178,
 179, 180, 181, 182, 183, 184, 185, 187, 188, 189, 190, 191, 192, 193, 195, 196,
 197, 198, 199, 200, 202, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 216,
 217, 218, 219, 220, 222, 223, 224, 226, 227, 228, 229, 230, 232, 233, 235, 236,
 237, 239, 240, 241, 242, 243, 244, 246, 247, 248, 249, 251, 252, 253, 254, 255,
 256, 258, 259, 260, 260, 261, 261, 261, 261, 261, 261, 261, 262, 262, 262, 262,
 262, 262, 263, 263, 264, 265, 265, 266, 266, 266, 266, 267, 267, 268, 269, 269,
 270
};
#ifndef yy_state_t
#define yy_state_t unsigned char
#endif

static yy_state_t yy_begin[] = {
 0, 211, 226, 226, 238, 238, 0
};

static yy_state_t yy_next[] = {
 24, 24, 24, 24, 24, 24, 24, 24, 24, 1, 239, 24, 24, 24, 24, 24,
 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
 1, 24, 16, 24, 24, 24, 24, 24, 22, 23, 24, 24, 24, 17, 19, 2,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 21, 24, 24, 20, 24, 24,
 24, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 24, 24, 24, 24, 15,
 24, 8, 15, 10, 6, 14, 4, 15, 15, 15, 15, 15, 13, 9, 12, 15,
 7, 15, 15, 3, 11, 15, 5, 15, 15, 15, 15, 24, 24, 24, 24, 24,
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 27, 33, 26, 26, 26, 26,
 26, 26, 26, 26, 26, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
 29, 29, 29, 29, 29, 29, 29, 29, 29, 29, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 34, 35, 36, 37, 38, 39, 40, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 41, 42, 43, 44, 32, 45, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
 32, 32, 32, 32, 32, 46, 47, 48, 50, 51, 52, 53, 54, 55, 56, 57,
 58, 59, 60, 61, 49, 62, 63, 64, 65, 66, 67, 69, 70, 71, 72, 73,
 74, 75, 76, 77, 78, 68, 79, 81, 84, 82, 85, 86, 87, 88, 89, 90,
 91, 92, 93, 94, 95, 96, 80, 83, 97, 98, 99, 100, 101, 102, 103, 104,
 105, 106, 107, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 108, 119, 120,
 121, 122, 124, 125, 123, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
 137, 138, 139, 140, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 143, 154,
 155, 142, 156, 157, 141, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168,
 169, 170, 171, 172, 173, 174, 175, 177, 178, 179, 176, 180, 181, 182, 184, 186,
 187, 188, 189, 190, 191, 192, 193, 195, 196, 183, 197, 198, 199, 200, 201, 185,
 202, 203, 204, 205, 206, 207, 208, 194, 209, 210, 212, 214, 215, 216, 217, 218,
 213, 219, 220, 221, 222, 223, 224, 225, 227, 240, 240, 240, 240, 240, 240, 240,
 210, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, 240, 240, 240, 240, 240,
 231, 240, 240, 240, 240, 240, 232, 240, 240, 240, 240, 229, 240, 240, 230, 230,
 230, 230, 230, 230, 230, 230, 230, 230, 240, 240, 240, 240, 240, 240, 240, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 240, 240, 240, 240, 228, 240, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 240, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 234, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 240, 233, 240, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 233, 233, 240, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233, 233,
 233, 233, 233, 233, 233, 233, 233, 233, 233, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 236, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235, 235,
 235, 235, 235, 235, 235, 235, 235, 235, 235, 237, 237, 237, 237, 237, 237, 237,
 237, 237, 237, 240, 240, 240, 240, 240, 240, 240, 237, 237, 237, 237, 237, 237,
 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237,
 237, 237, 237, 237, 240, 240, 240, 240, 237, 240, 237, 237, 237, 237, 237, 237,
 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237,
 237, 237, 237, 237,
};

static yy_state_t yy_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 238, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 18, 14, 18, 18, 18, 18,
 18, 18, 18, 18, 18, 18, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 33, 34, 35, 36, 37, 38, 39, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 40, 41, 42, 43, 15, 44, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 45, 46, 13, 48, 50, 51, 52, 53, 54, 55, 56,
 57, 58, 59, 49, 48, 61, 62, 63, 64, 65, 66, 12, 69, 70, 71, 72,
 73, 74, 75, 76, 77, 12, 68, 79, 83, 81, 84, 85, 86, 82, 88, 89,
 80, 91, 92, 93, 94, 11, 79, 81, 96, 97, 98, 99, 100, 10, 102, 103,
 104, 105, 106, 9, 109, 110, 111, 112, 113, 108, 115, 116, 117, 9, 118, 119,
 120, 8, 123, 124, 8, 125, 126, 127, 128, 129, 130, 131, 122, 133, 134, 135,
 136, 137, 138, 7, 143, 144, 145, 146, 147, 148, 149, 142, 151, 152, 7, 153,
 154, 7, 155, 156, 7, 141, 158, 159, 160, 161, 162, 140, 164, 165, 166, 167,
 168, 169, 6, 171, 172, 173, 5, 175, 177, 178, 175, 176, 180, 181, 4, 185,
 186, 184, 188, 183, 190, 191, 3, 194, 195, 4, 196, 197, 193, 199, 200, 4,
 201, 202, 203, 204, 205, 206, 207, 3, 2, 1, 211, 212, 214, 215, 216, 217,
 212, 218, 213, 220, 221, 222, 223, 224, 226, ~0, ~0, ~0, ~0, ~0, ~0, ~0,
 1, 230, 230, 230, 230, 230, 230, 230, 230, 230, 230, ~0, ~0, ~0, ~0, ~0,
 226, ~0, ~0, ~0, ~0, ~0, 226, ~0, ~0, ~0, ~0, 226, ~0, ~0, 226, 226,
 226, 226, 226, 226, 226, 226, 226, 226, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 226,
 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226,
 226, 226, 226, 226, 226, 226, 226, 226, 226, ~0, ~0, ~0, ~0, 226, ~0, 226,
 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226, 226,
 226, 226, 226, 226, 226, 226, 226, 226, 226, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, ~0, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, ~0, 232, ~0, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, ~0, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232, 232,
 232, 232, 232, 232, 232, 232, 232, 232, 232, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231, 231,
 231, 231, 231, 231, 231, 231, 231, 231, 231, 228, 228, 228, 228, 228, 228, 228,
 228, 228, 228, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228, ~0, ~0, ~0, ~0, 228, ~0, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228, 228,
 228, 228, 228, 228,
};

static yy_state_t yy_default[] = {
 240, 240, 240, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 240,
 240, 240, 240, 240, 240, 240, 240, 240, 240, 19, 18, 240, 27, 17, 16, 240,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
 15, 240, 1, 0, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240,
 240, 240, 240, 240, 240, 230, 240, 240, 240, 232, 232, 231, 240, 228, 240, 240,

};

static short yy_base[] = {
 0, 528, 489, 419, 413, 391, 397, 370, 335, 334, 318, 324, 294, 264, 29, 250,
 170, 112, 92, 80, 996, 996, 996, 996, 996, 996, 996, 102, 996, 996, 996, 996,
 996, 190, 214, 194, 214, 209, 218, 200, 240, 229, 226, 239, 232, 272, 274, 996,
 277, 275, 280, 270, 284, 265, 269, 285, 267, 279, 288, 278, 996, 294, 291, 280,
 275, 283, 278, 996, 311, 280, 302, 301, 290, 302, 296, 301, 293, 288, 996, 308,
 315, 312, 314, 299, 312, 314, 296, 996, 310, 314, 996, 305, 317, 322, 304, 996,
 321, 330, 321, 317, 312, 996, 320, 332, 331, 321, 318, 996, 331, 321, 322, 341,
 336, 339, 996, 331, 348, 336, 335, 336, 336, 996, 361, 350, 346, 342, 359, 356,
 348, 360, 343, 344, 996, 357, 361, 347, 343, 353, 365, 996, 381, 380, 367, 371,
 348, 375, 360, 362, 374, 373, 996, 355, 382, 371, 369, 371, 371, 996, 375, 373,
 383, 373, 369, 996, 397, 379, 397, 385, 393, 396, 996, 391, 403, 380, 996, 398,
 390, 405, 404, 996, 399, 408, 996, 418, 413, 410, 399, 996, 413, 996, 413, 402,
 996, 420, 405, 419, 425, 414, 996, 424, 417, 431, 434, 412, 420, 427, 434, 433,
 996, 996, 996, 503, 439, 436, 439, 439, 437, 433, 444, 996, 448, 440, 432, 450,
 450, 996, 542, 996, 873, 996, 513, 793, 665, 996, 996, 996, 996, 996, 0, 996,
 996
};


/*
 * Copyright 1988, 1990 by Mortice Kern Systems Inc.  All rights reserved.
 * All rights reserved.
 */
#include <stdlib.h>
#include <stdio.h>
/*
 * Define gettext() to an appropriate function for internationalized messages
 * or custom processing.
 */
#if	__STDC__
#define YY_ARGS(args)	args
#else
#define YY_ARGS(args)	()
#endif

#ifndef I18N
#define gettext(s)	(s)
#endif
/*
 * Include string.h to get definition of memmove() and size_t.
 * If you do not have string.h or it does not declare memmove
 * or size_t, you will have to declare them here.
 */
#include <string.h>
/* Uncomment next line if memmove() is not declared in string.h */
/*extern char * memmove();*/
/* Uncomment next line if size_t is not available in stdio.h or string.h */
/*typedef unsigned size_t;*/
/* Drop this when LATTICE provides memmove */
#ifdef LATTICE
#define memmove	memcopy
#endif

/*
 * YY_STATIC determines the scope of variables and functions
 * declared by the lex scanner. It must be set with a -DYY_STATIC
 * option to the compiler (it cannot be defined in the lex program).
 */
#ifdef	YY_STATIC
/* define all variables as static to allow more than one lex scanner */
#define	YY_DECL	static
#else
/* define all variables as global to allow other modules to access them */
#define	YY_DECL	
#endif

/*
 * You can redefine yygetc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define yygetc()	yt_getc()
#else
#define	yygetc()	getc(yyin) 	/* yylex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#define	ECHO		fputs(yytext, yyout)
#define	output(c)	putc((c), yyout) /* yylex sink for unmatched chars */
#define	YY_FATAL(msg)	{ fprintf(stderr, "yylex: %s\n", msg); exit(1); }
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */
#define	YYLMAX		100		/* token and pushback buffer size */

/*
 * the following must not be redefined.
 */
#define	yy_tbuf	yytext		/* token string */

#define	BEGIN		yy_start =
#define	REJECT		goto yy_reject
#define	NLSTATE		(yy_lastc = YYNEWLINE)
#define	YY_INIT \
	(yy_start = yyleng = yy_end = 0, yy_lastc = YYNEWLINE)
#define	yymore()	goto yy_more
#define	yyless(n)	if ((n) < 0 || (n) > yy_end) ; \
			else { YY_SCANNER; yyleng = (n); YY_USER; }

YY_DECL	void	yy_reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	yywrap	YY_ARGS((void));
extern	void	yyerror	YY_ARGS((char *fmt, ...));
extern	void	yycomment	YY_ARGS((char *term));
extern	int	yymapch	YY_ARGS((int delim, int escape));


#include <stdlib.h>
#include <schtok.h>
#include <dbg.h>
#include <cfgdbg.h>

#ifdef DEBUG
#include <memall.h>
#include <dbmem.h>

#define local_strdup(s) (strcpy((char *)(malloc(strlen(s) + 1)), (s)))
#else
#define local_strdup(s) strdup(s)
#endif

#ifdef CNTLEX
#define yylval cntlval
#define yytext cnttext
#define yyin cntin
#define yyparse cntparse
#define OpenYyin OpenCntIn
#define YyParseFile CntParseFile
#define yywrap cntwrap
#define yy_reset cnt_reset
#endif




#ifdef	YY_DEBUG
#undef	YY_DEBUG
#define	YY_DEBUG(fmt, a1, a2)	fprintf(stderr, fmt, a1, a2)
#else
#define	YY_DEBUG(fmt, a1, a2)
#endif

/*
 * The declaration for the lex scanner can be changed by
 * redefining YYLEX or YYDECL. This must be done if you have
 * more than one scanner in a program.
 */
#ifndef	YYLEX
#define	YYLEX yylex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/* stdin and stdout may not neccessarily be constants */
YY_DECL	FILE* yyin = NULL;
YY_DECL	FILE* yyout = NULL;
YY_DECL	int	yylineno = 1;		/* line number */

/*
 * yy_tbuf is an alias for yytext.
 * yy_sbuf[0:yyleng-1] contains the states corresponding to yy_tbuf.
 * yy_tbuf[0:yyleng-1] contains the current token.
 * yy_tbuf[yyleng:yy_end-1] contains pushed-back characters.
 * When the user action routine is active,
 * yy_save contains yy_tbuf[yyleng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */

YY_DECL	char yy_tbuf [YYLMAX+1]; /* text buffer (really yytext) */
static	yy_state_t yy_sbuf [YYLMAX+1];	/* state buffer */

static	int	yy_end = 0;		/* end of pushback */
static	int	yy_start = 0;		/* start state */
static	int	yy_lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	yyleng = 0;		/* yytext token length */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char yy_save;	/* saved yytext[yyleng] */

#define	YY_USER	{ /* set up yytext for user */ \
		yy_save = yytext[yyleng]; \
		yytext[yyleng] = 0; \
	}
#define	YY_SCANNER { /* set up yytext for scanner */ \
		yytext[yyleng] = yy_save; \
	}

#else		/* not-so efficient push-back for yytext mungers */

static	char yy_save [YYLMAX];
static	char *yy_push = yy_save+YYLMAX;

#define	YY_USER { \
		size_t n = yy_end - yyleng; \
		yy_push = yy_save+YYLMAX - n; \
		if (n > 0) \
			memmove(yy_push, yytext+yyleng, n); \
		yytext[yyleng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = yy_save+YYLMAX - yy_push; \
		if (n > 0) \
			memmove(yytext+yyleng, yy_push, n); \
		yy_end = yyleng + n; \
	}

#endif

/*
 * The actual lex scanner (usually yylex(void)).
 * NOTE: you should invoke yy_init() if you are calling yylex()
 * with new input; otherwise old lookaside will get in your way
 * and yylex() will die horribly.
 */
YYDECL {
	register int c, i, yyst, yybase;
	int yyfmin, yyfmax;	/* yy_la_act indices of final states */
	int yyoldi, yyoleng;	/* base i, yyleng before look-ahead */
	int yyeof;		/* 1 if eof has already been read */



	yyeof = 0;
	i = yyleng;
	YY_SCANNER;

  yy_again:
	yyleng = i;
	/* determine previous char. */
	if (i > 0)
		yy_lastc = yytext[i-1];
	/* scan previously accepted token adjusting yylineno */
	while (i > 0)
		if (yytext[--i] == YYNEWLINE)
			yylineno++;
	/* adjust pushback */
	yy_end -= yyleng;
	memmove(yytext, yytext+yyleng, (size_t) yy_end);
	i = 0;

  yy_contin:
	yyoldi = i;

	/* run the state machine until it jams */
	yy_sbuf[i] = yyst = yy_begin[yy_start + (yy_lastc == YYNEWLINE)];
	do {
		YY_DEBUG(gettext("<state %d, i = %d>\n"), yyst, i);
		if (i >= YYLMAX)
			YY_FATAL(gettext("Token buffer overflow"));

		/* get input char */
		if (i < yy_end)
			c = yy_tbuf[i];		/* get pushback char */
		else if (!yyeof && (c = yygetc()) != EOF) {
			yy_end = i+1;
			yy_tbuf[i] = c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == yyoldi) {	/* no token */
				yyeof = 0;
				if (yywrap())
					return 0;
				else
					goto yy_again;
			} else {
				yyeof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(gettext("<input %d = 0x%02x>\n"), c, c);

		/* look up next state */
		while ((yybase = yy_base[yyst]+c) > yy_nxtmax || yy_check[yybase] != yyst) {
			if (yyst == yy_endst)
				goto yy_jammed;
			yyst = yy_default[yyst];
		}
		yyst = yy_next[yybase];
	  yy_jammed: ;
	  yy_sbuf[++i] = yyst;
	} while (!(yyst == yy_endst || YY_INTERACTIVE && yy_base[yyst] > yy_nxtmax && yy_default[yyst] == yy_endst));
	YY_DEBUG(gettext("<stopped %d, i = %d>\n"), yyst, i);
	if (yyst != yy_endst)
		++i;

  yy_search:
	/* search backward for a final state */
	while (--i > yyoldi) {
		yyst = yy_sbuf[i];
		if ((yyfmin = yy_final[yyst]) < (yyfmax = yy_final[yyst+1]))
			goto yy_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = yyoldi + 1;
	output(yy_tbuf[yyoldi]);
	goto yy_again;

  yy_found:
	YY_DEBUG(gettext("<final state %d, i = %d>\n"), yyst, i);
	yyoleng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(yy_la_act[yyfmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = yy_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < yyoldi) {	/* no / */
				i = yyoleng;
				break;
			}
			yyst = yy_sbuf[i];
			if (bv[(unsigned)yyst/8] & bits[(unsigned)yyst%8])
				break;
		}
	}

	/* perform action */
	yyleng = i;
	YY_USER;
	switch (yy_la_act[yyfmin] & 0777) {
	case 0:
	;
	break;
	case 1:
	{
   ConfigSpew("lexspew", ("schlex: comment\n"));
   BEGIN COMMENT;
}
	break;
	case 2:
	{
   ConfigSpew("lexspew", ("schlex: end comment\n"));
   BEGIN INITIAL;
}
	break;
	case 3:
	{
   ConfigSpew("lexspew", ("schlex: include\n"));
   BEGIN PREPROC;
   return INCLUDE;
}
	break;
	case 4:
	{
   ConfigSpew("lexspew", ("schlex: define\n"));
   BEGIN PREPROC;
   return DEFINE;
}
	break;
	case 5:
	{
   ConfigSpew("lexspew", ("schlex: return\n"));
   BEGIN INITIAL;
}
	break;
	case 6:
	{
   ConfigSpew("lexspew", ("schlex: ident\n"));
   yylval.strval = local_strdup(yytext);
   return IDENT;
}
	break;
	case 7:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.ival = atoi(yytext);
   return INT;
}
	break;
	case 8:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}
	break;
	case 9:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}
	break;
	case 10:
	{ConfigSpew("lexspew", ("schlex: schema\n")); return SCHEMA;};
	break;
	case 11:
	return FLAGS;
	break;
	case 12:
	return VOLUME;
	break;
	case 13:
	return DELAY;
	break;
	case 14:
	return PAN;
	break;
	case 15:
	return PAN_RANGE;
	break;
	case 16:
	return PRIORITY;
	break;
	case 17:
	return FADE;
	break;
	case 18:
	return ARCHETYPE;
	break;
	case 19:
	return FREQ;
	break;
	case 20:
	return MONO_LOOP;
	break;
	case 21:
	return POLY_LOOP;
	break;
	case 22:
	return CONCEPT;
	break;
	case 23:
	return TAG;
	break;
	case 24:
	return TAG_INT;
	break;
	case 25:
	return VOICE;
	break;
	case 26:
	return SCHEMA_VOICE;
	break;
	case 27:
	return MESSAGE;
	break;
	case 28:
	return NO_REPEAT;
	break;
	case 29:
	return NO_CACHE;
	break;
	case 30:
	return AUDIO_CLASS;
	break;
	case 31:
	return STREAM;
	break;
	case 32:
	return LOOP_COUNT;
	break;
	case 33:
	return ENV_TAG_REQUIRED;
	break;
	case 34:
	return ENV_TAG;
	break;
	case 35:
	return PLAY_ONCE;
	break;
	case 36:
	return NO_COMBAT; 
	break;
	case 37:
	return NET_AMBIENT;
	break;
	case 38:
	return LOCAL_SPATIAL;
	break;
	case 39:
	{
   int val;

   ConfigSpew("lexspew", ("schlex: ident\n"));
   if (!IncTabLookup(yytext, &val))
   {
      yylval.strval = local_strdup(yytext);
      return IDENT;
   }
   else
   {
      yylval.ival = val;
      return INT;
   }
}
	break;
	case 40:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   yylval.strval = local_strdup(yytext);
   return STRING;
}
	break;
	case 41:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.ival = atoi(yytext);
   return INT;
}
	break;
	case 42:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   yylval.fval = atof(yytext);
   return FLOAT;
}
	break;
	case 43:
	return EQUAL;
	break;
	case 44:
	return COLON;
	break;
	case 45:
	return LPAREN;
	break;
	case 46:
	return RPAREN;
	break;
	case 47:
	;
	break;


	}
	YY_SCANNER;
	i = yyleng;
	goto yy_again;			/* action fell though */

  yy_reject:
	YY_SCANNER;
	i = yyoleng;			/* restore original yytext */
	if (++yyfmin < yyfmax)
		goto yy_found;		/* another final state, same length */
	else
		goto yy_search;		/* try shorter yytext */

  yy_more:
	YY_SCANNER;
	i = yyleng;
	if (i > 0)
		yy_lastc = yytext[i-1];
	goto yy_contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct yy_save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	yy_state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
yySaveScan(fp)
FILE * fp;
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = yyin;
	p->oldline = yylineno;
	p->oldend = yy_end;
	p->oldstart = yy_start;
	p->oldlastc = yy_lastc;
	p->oldleng = yyleng;
	(void) memcpy(p->savetext, yytext, sizeof yytext);
	(void) memcpy((char *) p->savestate, (char *) yy_sbuf,
		sizeof yy_sbuf);

	yyin = fp;
	yylineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
yyRestoreScan(p)
YY_SAVED * p;
{
	if (p == NULL)
		return;
	yyin = p->oldfp;
	yylineno = p->oldline;
	yy_end = p->oldend;
	yy_start = p->oldstart;
	yy_lastc = p->oldlastc;
	yyleng = p->oldleng;

	(void) memcpy(yytext, p->savetext, sizeof yytext);
	(void) memcpy((char *) yy_sbuf, (char *) p->savestate,
		sizeof yy_sbuf);
	free(p);
}
/*
 * User-callable re-initialization of yylex()
 */
void
yy_reset()
{
	YY_INIT;
	yylineno = 1;		/* line number */
}
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (yy_end > yyleng) {
		yy_end--;
		memmove(yytext+yyleng, yytext+yyleng+1,
			(size_t) (yy_end-yyleng));
		c = yy_save;
		YY_USER;
#else
	if (yy_push < yy_save+YYLMAX) {
		c = *yy_push++;
#endif
	} else
		c = yygetc();
	yy_lastc = c;
	if (c == YYNEWLINE)
		yylineno++;
	return c;
}

/*f
 * pushback char
 */
YY_DECL int
unput(c)
	int c;
{
#ifndef YY_PRESERVE
	if (yy_end >= YYLMAX)
		YY_FATAL(gettext("Push-back buffer overflow"));
	if (yy_end > yyleng) {
		yytext[yyleng] = yy_save;
		memmove(yytext+yyleng+1, yytext+yyleng,
			(size_t) (yy_end-yyleng));
		yytext[yyleng] = 0;
	}
	yy_end++;
	yy_save = c;
#else
	if (yy_push <= yy_save)
		YY_FATAL(gettext("Push-back buffer overflow"));
	*--yy_push = c;
#endif
	if (c == YYNEWLINE)
		yylineno--;
	return c;
}


BOOL OpenYyin(char *file_name)
{
   FILE *in_file;

   if (yyin != NULL)
      fclose(yyin);
   if (!(in_file = fopen(file_name, "r")))
   {
      Warning(("OpenYYIn: can't open file %s for reading\n",
               file_name));
      return FALSE;
   }
   yyin = in_file;
   return TRUE;
}

extern int yyparse();

void YyParseFile(char *schemaFile)
{
   yyin = NULL;
   yy_reset();
   if (!OpenYyin(schemaFile))
      return;
   while (!feof(yyin))
      yyparse();
   fclose(yyin);
}

int yywrap(void) { return 1;}



