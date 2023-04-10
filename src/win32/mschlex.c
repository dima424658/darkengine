/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* n:\bin\lex -p msch -l -o win32\mschlex.c c:\prj\cam\src\mschlex.lex */
#define YYNEWLINE 10
#define INITIAL 0
#define PREPROC 2
#define COMMENT 4
#define msch_endst 521
#define msch_nxtmax 1275
#define YY_LA_SIZE 66

static unsigned short msch_la_act[] = {
 0, 99, 99, 91, 99, 91, 99, 91, 99, 91, 99, 91, 99, 91, 99, 91,
 99, 91, 99, 91, 99, 91, 99, 91, 99, 91, 99, 91, 99, 91, 99, 99,
 99, 93, 94, 99, 99, 95, 99, 96, 99, 97, 99, 98, 99, 99, 94, 93,
 94, 94, 94, 93, 94, 92, 91, 91, 91, 91, 42, 91, 91, 91, 91, 91,
 91, 91, 91, 91, 91, 39, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 91, 91, 91, 36, 91, 91, 91, 35, 91, 91, 91, 91, 91, 91, 91, 34,
 91, 91, 91, 91, 91, 91, 91, 91, 32, 91, 91, 91, 91, 91, 91, 31,
 91, 91, 91, 30, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 91, 91, 29, 91, 91, 91, 91, 91, 91, 91, 28, 91, 91, 91, 91, 91,
 91, 91, 37, 91, 91, 91, 27, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 26, 91, 91, 91, 25, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 91, 50, 91, 49, 91, 91, 91, 44, 91, 91, 91, 91, 91, 91, 24, 91,
 91, 91, 91, 91, 91, 22, 91, 91, 91, 91, 91, 14, 91, 20, 91, 18,
 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 41, 91, 91, 91, 91,
 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 43, 91, 91, 91, 91,
 91, 91, 91, 91, 91, 91, 91, 91, 47, 91, 91, 91, 91, 91, 91, 91,
 91, 91, 33, 91, 91, 91, 15, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 16, 91, 91, 91, 91, 91, 91, 91, 46, 91, 91, 91, 91, 91, 91, 13,
 91, 91, 91, 91, 91, 91, 91, 48, 91, 91, 91, 91, 91, 91, 45, 91,
 12, 91, 91, 91, 91, 91, 91, 91, 91, 23, 91, 11, 91, 19, 91, 91,
 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 91, 91, 91, 91, 91, 81, 91, 91, 91, 91, 75, 91, 91, 91, 91, 91,
 91, 85, 91, 86, 91, 87, 91, 91, 91, 91, 91, 91, 91, 91, 91, 91,
 91, 91, 91, 91, 91, 91, 91, 80, 91, 91, 91, 91, 79, 91, 91, 91,
 78, 91, 91, 91, 91, 91, 91, 91, 91, 72, 91, 91, 91, 71, 91, 91,
 91, 68, 91, 91, 91, 88, 91, 91, 91, 91, 77, 91, 91, 91, 91, 91,
 91, 91, 64, 91, 91, 91, 91, 91, 91, 91, 63, 91, 91, 91, 91, 82,
 91, 91, 91, 91, 91, 91, 91, 90, 91, 91, 91, 91, 89, 91, 91, 57,
 91, 91, 91, 56, 91, 91, 91, 91, 61, 91, 62, 91, 91, 91, 91, 91,
 69, 91, 91, 91, 91, 91, 59, 91, 60, 91, 91, 91, 55, 91, 91, 91,
 91, 54, 91, 91, 91, 58, 91, 91, 53, 91, 91, 91, 91, 91, 52, 91,
 91, 91, 91, 91, 91, 91, 91, 91, 91, 91, 84, 91, 91, 91, 91, 91,
 91, 83, 91, 91, 91, 91, 91, 91, 91, 70, 91, 91, 91, 91, 91, 91,
 91, 91, 91, 74, 91, 91, 76, 91, 91, 91, 91, 91, 91, 91, 67, 91,
 91, 91, 91, 91, 66, 91, 91, 91, 91, 65, 91, 51, 91, 91, 91, 91,
 91, 91, 91, 73, 91, 17, 91, 91, 91, 91, 91, 91, 40, 91, 91, 91,
 91, 38, 91, 91, 91, 91, 91, 21, 91, 91, 91, 91, 10, 91, 1, 0,
 99, 4, 3, 5, 6, 7, 9, 8, 6, 2,
};

static unsigned char msch_look[] = {
 0
};

static short msch_final[] = {
 0, 0, 2, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27,
 29, 31, 32, 33, 36, 37, 39, 41, 43, 45, 46, 47, 49, 50, 51, 53,
 53, 54, 55, 56, 57, 58, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 85, 86, 87,
 89, 90, 91, 92, 93, 94, 95, 97, 98, 99, 100, 101, 102, 103, 104, 106,
 107, 108, 109, 110, 111, 113, 114, 115, 117, 118, 119, 120, 121, 122, 123, 124,
 125, 126, 127, 128, 129, 130, 132, 133, 134, 135, 136, 137, 138, 140, 141, 142,
 143, 144, 145, 146, 148, 149, 150, 152, 153, 154, 155, 156, 157, 158, 159, 160,
 162, 163, 164, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 179,
 181, 182, 183, 185, 186, 187, 188, 189, 190, 192, 193, 194, 195, 196, 197, 199,
 200, 201, 202, 203, 205, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
 235, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 250, 251, 252,
 253, 254, 255, 256, 257, 258, 260, 261, 262, 264, 265, 266, 267, 268, 269, 270,
 271, 272, 274, 275, 276, 277, 278, 279, 280, 282, 283, 284, 285, 286, 287, 289,
 290, 291, 292, 293, 294, 295, 297, 298, 299, 300, 301, 302, 304, 306, 307, 308,
 309, 310, 311, 312, 313, 315, 317, 319, 320, 321, 322, 323, 324, 325, 326, 327,
 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 343, 344,
 345, 346, 348, 349, 350, 351, 352, 353, 355, 357, 359, 360, 361, 362, 363, 364,
 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 377, 378, 379, 380, 382,
 383, 384, 386, 387, 388, 389, 390, 391, 392, 393, 395, 396, 397, 399, 400, 401,
 403, 404, 405, 407, 408, 409, 410, 412, 413, 414, 415, 416, 417, 418, 420, 421,
 422, 423, 424, 425, 426, 428, 429, 430, 431, 433, 434, 435, 436, 437, 438, 439,
 441, 442, 443, 444, 446, 447, 449, 450, 451, 453, 454, 455, 456, 458, 460, 461,
 462, 463, 464, 466, 467, 468, 469, 470, 472, 474, 475, 476, 478, 479, 480, 481,
 483, 484, 485, 487, 488, 490, 491, 492, 493, 494, 496, 497, 498, 499, 500, 501,
 502, 503, 504, 505, 506, 508, 509, 510, 511, 512, 513, 515, 516, 517, 518, 519,
 520, 521, 523, 524, 525, 526, 527, 528, 529, 530, 531, 533, 534, 536, 537, 538,
 539, 540, 541, 542, 544, 545, 546, 547, 548, 550, 551, 552, 553, 555, 557, 558,
 559, 560, 561, 562, 563, 565, 566, 567, 568, 569, 570, 571, 572, 574, 575, 576,
 577, 579, 580, 581, 582, 583, 585, 586, 587, 588, 590, 591, 592, 592, 593, 593,
 593, 593, 593, 593, 593, 594, 594, 594, 594, 594, 594, 595, 595, 596, 597, 597,
 598, 598, 598, 598, 599, 599, 600, 601, 601, 602
};
#ifndef msch_state_t
#define msch_state_t unsigned short
#endif

static msch_state_t msch_begin[] = {
 0, 492, 507, 507, 519, 519, 0
};

static msch_state_t msch_next[] = {
 25, 25, 25, 25, 25, 25, 25, 25, 25, 1, 520, 25, 25, 25, 25, 25,
 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
 1, 25, 17, 25, 25, 25, 25, 25, 23, 24, 25, 25, 25, 18, 20, 2,
 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 22, 25, 25, 21, 25, 25,
 25, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 25, 25, 25, 25, 16,
 25, 6, 9, 14, 8, 16, 16, 16, 12, 11, 16, 16, 16, 7, 10, 5,
 13, 16, 15, 3, 4, 16, 16, 16, 16, 16, 16, 25, 25, 25, 25, 25,
 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 28, 34, 27, 27, 27, 27,
 27, 27, 27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 29, 29, 29,
 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 32, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 35, 36, 37, 38, 39, 40, 41, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 42, 43, 44, 45, 33, 46, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33, 33,
 33, 33, 33, 33, 33, 47, 48, 49, 50, 51, 52, 54, 55, 56, 57, 58,
 59, 60, 61, 62, 63, 53, 64, 65, 66, 67, 68, 69, 70, 72, 73, 74,
 75, 76, 71, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 80, 89,
 90, 91, 92, 94, 95, 96, 97, 93, 98, 99, 100, 101, 102, 103, 104, 105,
 106, 107, 108, 109, 111, 112, 113, 114, 115, 116, 117, 118, 110, 119, 121, 122,
 123, 124, 125, 126, 127, 128, 129, 130, 120, 135, 136, 137, 138, 131, 139, 140,
 141, 142, 143, 134, 144, 145, 133, 146, 147, 132, 148, 149, 150, 151, 152, 153,
 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 167, 170, 171, 172,
 173, 174, 175, 176, 177, 168, 178, 179, 180, 169, 182, 166, 183, 184, 185, 186,
 187, 188, 189, 190, 191, 192, 193, 181, 195, 196, 194, 197, 198, 199, 200, 201,
 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
 218, 219, 220, 221, 222, 223, 224, 225, 227, 228, 226, 229, 230, 231, 232, 233,
 234, 235, 236, 237, 238, 241, 242, 243, 244, 239, 245, 246, 247, 248, 249, 240,
 250, 251, 252, 253, 255, 256, 257, 258, 259, 260, 261, 254, 262, 264, 265, 263,
 266, 278, 272, 280, 275, 281, 271, 279, 273, 282, 283, 284, 269, 268, 285, 286,
 277, 287, 270, 267, 276, 274, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297,
 299, 301, 302, 303, 298, 304, 306, 307, 308, 309, 310, 311, 312, 313, 300, 314,
 315, 316, 317, 318, 319, 305, 320, 321, 322, 323, 324, 326, 327, 328, 329, 330,
 325, 331, 332, 333, 334, 335, 337, 338, 340, 341, 342, 343, 336, 344, 345, 346,
 347, 348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 339, 359, 360, 361, 363,
 364, 365, 366, 362, 367, 368, 358, 369, 370, 371, 372, 373, 374, 375, 376, 377,
 378, 379, 380, 381, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395,
 396, 397, 383, 398, 399, 382, 400, 402, 403, 404, 405, 406, 407, 408, 409, 410,
 401, 414, 415, 416, 417, 418, 419, 420, 421, 423, 424, 425, 426, 427, 412, 413,
 428, 411, 429, 430, 431, 432, 433, 434, 436, 437, 438, 439, 422, 435, 440, 441,
 442, 443, 444, 445, 448, 447, 449, 450, 451, 452, 453, 446, 454, 455, 456, 457,
 458, 459, 460, 462, 463, 464, 465, 466, 467, 468, 469, 470, 474, 475, 476, 477,
 478, 479, 480, 481, 482, 483, 461, 484, 472, 485, 486, 487, 471, 488, 489, 473,
 490, 491, 493, 495, 496, 497, 498, 499, 494, 500, 501, 502, 503, 504, 505, 506,
 508, 521, 521, 521, 521, 521, 521, 521, 491, 511, 511, 511, 511, 511, 511, 511,
 511, 511, 511, 521, 521, 521, 521, 521, 512, 521, 521, 521, 521, 521, 513, 521,
 521, 521, 521, 510, 521, 521, 511, 511, 511, 511, 511, 511, 511, 511, 511, 511,
 521, 521, 521, 521, 521, 521, 521, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 521, 521, 521, 521, 509, 521, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 521, 514, 514, 514, 514,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 515, 514, 514, 514, 514, 514,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 521, 514, 521,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 521,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514,
 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514, 514,
 514, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 517, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516, 516,
 516, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 521, 521, 521, 521, 521,
 521, 521, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518,
 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 521, 521, 521, 521,
 518, 521, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518,
 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518, 518,
};

static msch_state_t msch_check[] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 519, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 19, 15, 19, 19, 19, 19,
 19, 19, 19, 19, 19, 19, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28,
 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 34, 35, 36, 14, 38, 39, 40, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 41, 42, 43, 44, 16, 45, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 46, 13, 48, 49, 50, 51, 52, 54, 55, 56, 57,
 58, 59, 53, 61, 62, 52, 12, 64, 65, 66, 67, 68, 69, 11, 72, 73,
 74, 75, 11, 76, 77, 71, 79, 81, 82, 83, 80, 85, 86, 10, 79, 88,
 89, 90, 91, 93, 94, 95, 96, 91, 97, 98, 99, 100, 92, 102, 103, 104,
 105, 106, 107, 9, 110, 111, 112, 113, 114, 109, 116, 117, 9, 118, 119, 121,
 122, 123, 124, 125, 126, 120, 128, 129, 119, 8, 135, 136, 137, 8, 138, 139,
 140, 141, 134, 8, 133, 144, 8, 145, 132, 8, 147, 148, 149, 150, 151, 131,
 153, 154, 155, 156, 157, 7, 159, 160, 161, 162, 163, 164, 6, 169, 170, 171,
 172, 173, 174, 175, 176, 6, 168, 178, 179, 6, 181, 6, 182, 183, 184, 185,
 186, 187, 188, 189, 190, 191, 180, 179, 194, 195, 180, 196, 197, 198, 199, 200,
 201, 202, 203, 193, 205, 206, 207, 208, 209, 210, 211, 212, 167, 214, 215, 216,
 217, 218, 219, 220, 221, 222, 223, 224, 166, 227, 166, 228, 229, 230, 231, 226,
 233, 234, 235, 236, 237, 5, 241, 242, 243, 5, 244, 240, 246, 247, 248, 5,
 249, 250, 239, 4, 254, 255, 256, 257, 258, 259, 253, 4, 261, 263, 264, 261,
 265, 266, 266, 279, 266, 280, 266, 266, 266, 281, 282, 283, 266, 266, 284, 278,
 266, 286, 266, 266, 266, 266, 287, 288, 289, 290, 291, 292, 293, 294, 294, 294,
 277, 300, 301, 302, 277, 303, 305, 306, 307, 308, 309, 310, 311, 312, 277, 313,
 304, 315, 316, 317, 299, 303, 319, 320, 298, 322, 323, 325, 326, 327, 328, 324,
 323, 330, 331, 276, 333, 334, 275, 337, 336, 340, 341, 339, 275, 343, 344, 345,
 346, 347, 348, 274, 350, 351, 352, 353, 354, 355, 273, 336, 358, 359, 360, 361,
 363, 364, 365, 361, 366, 362, 273, 368, 369, 370, 357, 372, 272, 374, 375, 376,
 377, 378, 379, 380, 271, 384, 385, 383, 387, 388, 389, 390, 391, 382, 393, 394,
 270, 396, 271, 397, 398, 271, 269, 401, 400, 403, 268, 405, 406, 407, 408, 267,
 269, 413, 414, 415, 416, 417, 418, 419, 412, 422, 423, 424, 425, 421, 267, 267,
 427, 267, 428, 429, 430, 431, 432, 411, 435, 436, 437, 438, 412, 411, 439, 440,
 441, 434, 443, 444, 445, 445, 448, 449, 450, 447, 452, 445, 453, 454, 455, 446,
 457, 458, 459, 410, 462, 463, 464, 465, 466, 467, 262, 3, 473, 474, 475, 472,
 477, 478, 479, 471, 481, 482, 410, 483, 3, 484, 470, 486, 3, 487, 488, 3,
 2, 1, 492, 493, 495, 496, 497, 498, 493, 499, 494, 501, 502, 503, 504, 505,
 507, ~0, ~0, ~0, ~0, ~0, ~0, ~0, 1, 511, 511, 511, 511, 511, 511, 511,
 511, 511, 511, ~0, ~0, ~0, ~0, ~0, 507, ~0, ~0, ~0, ~0, ~0, 507, ~0,
 ~0, ~0, ~0, 507, ~0, ~0, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507,
 ~0, ~0, ~0, ~0, ~0, ~0, ~0, 507, 507, 507, 507, 507, 507, 507, 507, 507,
 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507,
 507, ~0, ~0, ~0, ~0, 507, ~0, 507, 507, 507, 507, 507, 507, 507, 507, 507,
 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507, 507,
 507, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, ~0, 513, 513, 513, 513,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, ~0, 513, ~0,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, ~0,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513, 513,
 513, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512, 512,
 512, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, ~0, ~0, ~0, ~0, ~0,
 ~0, ~0, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, ~0, ~0, ~0, ~0,
 509, ~0, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509,
 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509, 509,
};

static msch_state_t msch_default[] = {
 521, 521, 521, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 20, 19, 521, 28, 18, 17,
 521, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 521, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 521, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 521, 1, 0, 521, 521, 521,
 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 521, 511, 521,
 521, 521, 513, 513, 512, 521, 509, 521, 521,
};

static short msch_base[] = {
 0, 808, 769, 696, 482, 457, 393, 374, 356, 327, 312, 287, 273, 266, 200, 28,
 250, 170, 112, 92, 80, 1276, 1276, 1276, 1276, 1276, 1276, 1276, 102, 1276, 1276, 1276,
 1276, 1276, 210, 198, 194, 1276, 202, 198, 198, 244, 232, 227, 239, 236, 272, 1276,
 278, 255, 276, 264, 313, 281, 269, 262, 306, 278, 275, 287, 1276, 278, 290, 1276,
 282, 295, 283, 283, 290, 296, 1276, 310, 303, 287, 292, 304, 304, 303, 1276, 298,
 293, 296, 309, 298, 1276, 297, 302, 1276, 316, 309, 322, 313, 313, 308, 304, 326,
 320, 319, 305, 325, 327, 1276, 334, 328, 326, 312, 332, 334, 1276, 340, 322, 323,
 333, 340, 333, 1276, 332, 343, 350, 346, 342, 346, 346, 352, 333, 343, 336, 1276,
 344, 354, 1276, 364, 358, 357, 363, 357, 347, 356, 345, 354, 367, 355, 1276, 1276,
 364, 371, 1276, 377, 359, 371, 366, 368, 1276, 364, 384, 372, 384, 383, 1276, 370,
 382, 377, 379, 375, 433, 1276, 455, 424, 394, 381, 397, 381, 391, 381, 393, 388,
 390, 1276, 408, 404, 421, 402, 397, 410, 403, 416, 415, 414, 398, 404, 402, 402,
 1276, 417, 419, 409, 424, 429, 428, 427, 411, 417, 415, 415, 1276, 425, 438, 437,
 436, 420, 426, 424, 424, 1276, 430, 428, 428, 449, 445, 445, 448, 440, 452, 436,
 450, 1276, 455, 454, 451, 446, 452, 458, 1276, 459, 445, 441, 451, 463, 1276, 462,
 470, 466, 467, 457, 467, 1276, 458, 465, 463, 462, 477, 1276, 1276, 483, 471, 480,
 463, 486, 470, 473, 1276, 473, 736, 492, 482, 497, 496, 634, 625, 625, 615, 611,
 603, 577, 563, 557, 545, 527, 499, 494, 482, 485, 485, 489, 505, 1276, 508, 500,
 499, 500, 503, 521, 509, 505, 572, 1276, 1276, 1276, 534, 530, 510, 525, 532, 530,
 543, 511, 520, 518, 533, 520, 534, 539, 537, 518, 1276, 533, 537, 545, 1276, 532,
 526, 1276, 537, 542, 554, 546, 549, 549, 538, 1276, 555, 542, 1276, 543, 560, 1276,
 564, 562, 1276, 557, 562, 565, 1276, 554, 554, 574, 567, 559, 559, 1276, 561, 561,
 581, 574, 566, 566, 1276, 595, 570, 569, 591, 583, 588, 587, 592, 572, 571, 1276,
 592, 592, 581, 1276, 595, 1276, 602, 595, 584, 607, 591, 606, 592, 1276, 606, 597,
 594, 594, 1276, 593, 616, 600, 615, 601, 1276, 608, 603, 1276, 618, 619, 608, 1276,
 626, 608, 1276, 613, 1276, 631, 632, 625, 633, 1276, 690, 654, 647, 623, 626, 625,
 635, 626, 641, 643, 1276, 633, 635, 636, 646, 648, 1276, 647, 643, 645, 659, 643,
 637, 1276, 659, 646, 661, 653, 662, 658, 666, 667, 1276, 667, 676, 664, 679, 676,
 663, 665, 673, 1276, 678, 675, 664, 673, 1276, 673, 671, 670, 1276, 1276, 674, 690,
 686, 686, 682, 690, 1276, 1276, 706, 689, 694, 695, 696, 690, 1276, 700, 700, 688,
 1276, 703, 689, 708, 705, 1276, 710, 704, 717, 1276, 1276, 1276, 783, 719, 716, 719,
 719, 717, 713, 724, 1276, 728, 720, 712, 730, 730, 1276, 822, 1276, 1153, 1276, 793,
 1073, 945, 1276, 1276, 1276, 1276, 1276, 0, 1276, 1276
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
 * You can redefine mschgetc. For YACC Tracing, compile this code
 * with -DYYTRACE to get input from yt_getc
 */
#ifdef YYTRACE
extern int	yt_getc YY_ARGS((void));
#define mschgetc()	yt_getc()
#else
#define	mschgetc()	getc(mschin) 	/* mschlex input source */
#endif

/*
 * the following can be redefined by the user.
 */
#define	ECHO		fputs(mschtext, mschout)
#define	output(c)	putc((c), mschout) /* mschlex sink for unmatched chars */
#define	YY_FATAL(msg)	{ fprintf(stderr, "mschlex: %s\n", msg); exit(1); }
#define	YY_INTERACTIVE	1		/* save micro-seconds if 0 */
#define	YYLMAX		100		/* token and pushback buffer size */

/*
 * the following must not be redefined.
 */
#define	msch_tbuf	mschtext		/* token string */

#define	BEGIN		msch_start =
#define	REJECT		goto msch_reject
#define	NLSTATE		(msch_lastc = YYNEWLINE)
#define	YY_INIT \
	(msch_start = mschleng = msch_end = 0, msch_lastc = YYNEWLINE)
#define	mschmore()	goto msch_more
#define	mschless(n)	if ((n) < 0 || (n) > msch_end) ; \
			else { YY_SCANNER; mschleng = (n); YY_USER; }

YY_DECL	void	msch_reset YY_ARGS((void));
YY_DECL	int	input	YY_ARGS((void));
YY_DECL	int	unput	YY_ARGS((int c));

/* functions defined in libl.lib */
extern	int	mschwrap	YY_ARGS((void));
extern	void	mscherror	YY_ARGS((char *fmt, ...));
extern	void	mschcomment	YY_ARGS((char *term));
extern	int	mschmapch	YY_ARGS((int delim, int escape));


#include <stdlib.h>
#include <mschtok.h>
#include <dbg.h>
#include <cfgdbg.h>
#include <mtagvals.h>
#include <drkcret.h> /* for actor types */
#include <shkcret.h> /* for actor types */
#include <dpccret.h> /* for Deep Cover actor types */

#ifdef DEBUG
#include <memall.h>
#include <dbmem.h>

#define local_strdup(s) (strcpy((char *)(malloc(strlen(s) + 1)), (s)))
#else
#define local_strdup(s) strdup(s)
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
#define	YYLEX mschlex			/* name of lex scanner */
#endif

#ifndef YYDECL
#define	YYDECL	int YYLEX YY_ARGS((void))	/* declaration for lex scanner */
#endif

/* stdin and stdout may not neccessarily be constants */
YY_DECL	FILE   *mschin = stdin;
YY_DECL	FILE   *mschout = stdout;
YY_DECL	int	mschlineno = 1;		/* line number */

/*
 * msch_tbuf is an alias for mschtext.
 * msch_sbuf[0:mschleng-1] contains the states corresponding to msch_tbuf.
 * msch_tbuf[0:mschleng-1] contains the current token.
 * msch_tbuf[mschleng:msch_end-1] contains pushed-back characters.
 * When the user action routine is active,
 * msch_save contains msch_tbuf[mschleng], which is set to '\0'.
 * Things are different when YY_PRESERVE is defined. 
 */

YY_DECL	char msch_tbuf [YYLMAX+1]; /* text buffer (really mschtext) */
static	msch_state_t msch_sbuf [YYLMAX+1];	/* state buffer */

static	int	msch_end = 0;		/* end of pushback */
static	int	msch_start = 0;		/* start state */
static	int	msch_lastc = YYNEWLINE;	/* previous char */
YY_DECL	int	mschleng = 0;		/* mschtext token length */

#ifndef YY_PRESERVE	/* the efficient default push-back scheme */

static	char msch_save;	/* saved mschtext[mschleng] */

#define	YY_USER	{ /* set up mschtext for user */ \
		msch_save = mschtext[mschleng]; \
		mschtext[mschleng] = 0; \
	}
#define	YY_SCANNER { /* set up mschtext for scanner */ \
		mschtext[mschleng] = msch_save; \
	}

#else		/* not-so efficient push-back for mschtext mungers */

static	char msch_save [YYLMAX];
static	char *msch_push = msch_save+YYLMAX;

#define	YY_USER { \
		size_t n = msch_end - mschleng; \
		msch_push = msch_save+YYLMAX - n; \
		if (n > 0) \
			memmove(msch_push, mschtext+mschleng, n); \
		mschtext[mschleng] = 0; \
	}
#define	YY_SCANNER { \
		size_t n = msch_save+YYLMAX - msch_push; \
		if (n > 0) \
			memmove(mschtext+mschleng, msch_push, n); \
		msch_end = mschleng + n; \
	}

#endif

/*
 * The actual lex scanner (usually mschlex(void)).
 * NOTE: you should invoke msch_init() if you are calling mschlex()
 * with new input; otherwise old lookaside will get in your way
 * and mschlex() will die horribly.
 */
YYDECL {
	register int c, i, mschst, mschbase;
	int mschfmin, mschfmax;	/* msch_la_act indices of final states */
	int mscholdi, mscholeng;	/* base i, mschleng before look-ahead */
	int mscheof;		/* 1 if eof has already been read */



	mscheof = 0;
	i = mschleng;
	YY_SCANNER;

  msch_again:
	mschleng = i;
	/* determine previous char. */
	if (i > 0)
		msch_lastc = mschtext[i-1];
	/* scan previously accepted token adjusting mschlineno */
	while (i > 0)
		if (mschtext[--i] == YYNEWLINE)
			mschlineno++;
	/* adjust pushback */
	msch_end -= mschleng;
	memmove(mschtext, mschtext+mschleng, (size_t) msch_end);
	i = 0;

  msch_contin:
	mscholdi = i;

	/* run the state machine until it jams */
	msch_sbuf[i] = mschst = msch_begin[msch_start + (msch_lastc == YYNEWLINE)];
	do {
		YY_DEBUG(gettext("<state %d, i = %d>\n"), mschst, i);
		if (i >= YYLMAX)
			YY_FATAL(gettext("Token buffer overflow"));

		/* get input char */
		if (i < msch_end)
			c = msch_tbuf[i];		/* get pushback char */
		else if (!mscheof && (c = mschgetc()) != EOF) {
			msch_end = i+1;
			msch_tbuf[i] = c;
		} else /* c == EOF */ {
			c = EOF;		/* just to make sure... */
			if (i == mscholdi) {	/* no token */
				mscheof = 0;
				if (mschwrap())
					return 0;
				else
					goto msch_again;
			} else {
				mscheof = 1;	/* don't re-read EOF */
				break;
			}
		}
		YY_DEBUG(gettext("<input %d = 0x%02x>\n"), c, c);

		/* look up next state */
		while ((mschbase = msch_base[mschst]+c) > msch_nxtmax || msch_check[mschbase] != mschst) {
			if (mschst == msch_endst)
				goto msch_jammed;
			mschst = msch_default[mschst];
		}
		mschst = msch_next[mschbase];
	  msch_jammed: ;
	  msch_sbuf[++i] = mschst;
	} while (!(mschst == msch_endst || YY_INTERACTIVE && msch_base[mschst] > msch_nxtmax && msch_default[mschst] == msch_endst));
	YY_DEBUG(gettext("<stopped %d, i = %d>\n"), mschst, i);
	if (mschst != msch_endst)
		++i;

  msch_search:
	/* search backward for a final state */
	while (--i > mscholdi) {
		mschst = msch_sbuf[i];
		if ((mschfmin = msch_final[mschst]) < (mschfmax = msch_final[mschst+1]))
			goto msch_found;	/* found final state(s) */
	}
	/* no match, default action */
	i = mscholdi + 1;
	output(msch_tbuf[mscholdi]);
	goto msch_again;

  msch_found:
	YY_DEBUG(gettext("<final state %d, i = %d>\n"), mschst, i);
	mscholeng = i;		/* save length for REJECT */
	
	/* pushback look-ahead RHS */
	if ((c = (int)(msch_la_act[mschfmin]>>9) - 1) >= 0) { /* trailing context? */
		unsigned char *bv = msch_look + c*YY_LA_SIZE;
		static unsigned char bits [8] = {
			1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7
		};
		while (1) {
			if (--i < mscholdi) {	/* no / */
				i = mscholeng;
				break;
			}
			mschst = msch_sbuf[i];
			if (bv[(unsigned)mschst/8] & bits[(unsigned)mschst%8])
				break;
		}
	}

	/* perform action */
	mschleng = i;
	YY_USER;
	switch (msch_la_act[mschfmin] & 0777) {
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
   mschlval.strval = local_strdup(mschtext);
   return IDENT;
}
	break;
	case 7:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.ival = atoi(mschtext);
   return INT;
}
	break;
	case 8:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}
	break;
	case 9:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}
	break;
	case 10:
	{ConfigSpew("lexspew", ("schlex: schema\n")); return SCHEMA;};
	break;
	case 11:
	return TAG;
	break;
	case 12:
	return TAG_OPT;
	break;
	case 13:
	return ARCHETYPE;
	break;
	case 14:
	return MOTION;
	break;
	case 15:
	return ACTOR;
	break;
	case 16:
	return ACTORS_DECLARE;
	break;
	case 17:
	return TAGLIST_HEADER;
	break;
	case 18:
	return MOTLIST_HEADER;
	break;
	case 19:
	return TAGLIST_HEADER;
	break;
	case 20:
	return MOTLIST_HEADER;
	break;
	case 21:
	return STRETCH;
	break;
	case 22:
	return DISTANCE;
	break;
	case 23:
	return TIMEWARP;
	break;
	case 24:
	return DURATION;
	break;
	case 25:
	return MOT_BLEND_NONE;
	break;
	case 26:
	return MOT_BLEND_DEFAULT;
	break;
	case 27:
	return MOT_BLEND_LENGTH;
	break;
	case 28:
	return MOT_NECK_FIXED;
	break;
	case 29:
	return MOT_NECK_NOT_FIXED;
	break;
	case 30:
	return MOT_IS_TURN;
	break;
	case 31:
	return MOT_IS_LOCO;
	break;
	case 32:
	return MOT_IN_PLACE;                 
	break;
	case 33:
	{ mschlval.ival=kNumDarkActorTypes; return INT; }
	break;
	case 34:
	{ mschlval.ival=kDATYPE_Humanoid; return INT; }
	break;
	case 35:
	{ mschlval.ival=kDATYPE_PlayerLimb; return INT; }
	break;
	case 36:
	{ mschlval.ival=kDATYPE_PlayerBowLimb; return INT; }
	break;
	case 37:
	{ mschlval.ival=kDATYPE_Burrick; return INT; }
	break;
	case 38:
	{ mschlval.ival=kDATYPE_Spider; return INT; }
	break;
	case 39:
	{ mschlval.ival=kDATYPE_Constantine; return INT; }
	break;
	case 40:
	{ mschlval.ival=kDATYPE_Sweel; return INT; }
	break;
	case 41:
	{ mschlval.ival=kDATYPE_Apparition; return INT; } 
	break;
	case 42:
	{ mschlval.ival=kDATYPE_Robot; return INT; }
	break;
	case 43:
	{ mschlval.ival=kNumShockActorTypes; return INT; }
	break;
	case 44:
	{ mschlval.ival=kShATYPE_Droid; return INT; }
	break;
	case 45:
	{ mschlval.ival=kShATYPE_Overlord; return INT; }
	break;
	case 46:
	{ mschlval.ival=kShATYPE_Arachnid; return INT; }
	break;
	case 47:
	{ mschlval.ival=kNumDPCActorTypes; return INT; }
	break;
	case 48:
	{ mschlval.ival=kDPCATYPE_OldDog; return INT; }
	break;
	case 49:
	{ mschlval.ival=kDPCATYPE_Dog; return INT; }
	break;
	case 50:
	{ mschlval.ival=kDPCATYPE_DeepHuman; return INT; }
	break;
	case 51:
	{ mschlval.ival=kMTV_set; return INT; }
	break;
	case 52:
	{ mschlval.ival=kMTV_middle; return INT; }
	break;
	case 53:
	{ mschlval.ival=kMTV_left; return INT; }
	break;
	case 54:
	{ mschlval.ival=kMTV_right; return INT; }
	break;
	case 55:
	{ mschlval.ival=kMTV_front; return INT; }
	break;
	case 56:
	{ mschlval.ival=kMTV_back; return INT; }
	break;
	case 57:
	{ mschlval.ival=kMTV_high; return INT; }
	break;
	case 58:
	{ mschlval.ival=kMTV_low; return INT; }
	break;
	case 59:
	{ mschlval.ival=kMTV_forward; return INT; }
	break;
	case 60:
	{ mschlval.ival=kMTV_forward; return INT; }
	break;
	case 61:
	{ mschlval.ival=kMTV_backward; return INT; }
	break;
	case 62:
	{ mschlval.ival=kMTV_backward; return INT; }
	break;
	case 63:
	{ mschlval.ival=kMTV_upstairs; return INT; }
	break;
	case 64:
	{ mschlval.ival=kMTV_downstairs; return INT; }
	break;
	case 65:
	{ mschlval.ival=kMTV_swing_short; return INT; }
	break;
	case 66:
	{ mschlval.ival=kMTV_swing_medium; return INT; }
	break;
	case 67:
	{ mschlval.ival=kMTV_swing_long; return INT; }
	break;
	case 68:
	{ mschlval.ival=kMTV_true; return INT; }
	break;
	case 69:
	{ mschlval.ival=kMTV_fast; return INT; }
	break;
	case 70:
	{ mschlval.ival=kMTV_stationary; return INT; }
	break;
	case 71:
	{ mschlval.ival=kMTV_perpleft; return INT; }
	break;
	case 72:
	{ mschlval.ival=kMTV_perpright; return INT; }
	break;
	case 73:
	{ mschlval.ival=kMTV_searching; return INT; }
	break;
	case 74:
	{ mschlval.ival=kMTV_swordmelee; return INT; }
	break;
	case 75:
	{ mschlval.ival=kMTV_alert; return INT; }
	break;
	case 76:
	{ mschlval.ival=kMTV_swing; return INT; }
	break;
	case 77:
	{ mschlval.ival=kMTV_dodge; return INT; }
	break;
	case 78:
	{ mschlval.ival=kMTV_parry; return INT; }
	break;
	case 79:
	{ mschlval.ival=kMTV_pose_calib; return INT; }
	break;
	case 80:
	{ mschlval.ival=kMTV_pose_swordready; return INT; }
	break;
	case 81:
	{ mschlval.ival=kMTV_gesture; return INT; }
	break;
	case 82:
	{ mschlval.ival=kMTV_hurt; return INT; }
	break;
	case 83:
	{ mschlval.ival=kMTV_stunned; return INT; }
	break;
	case 84:
	{ mschlval.ival=kMTV_surprised; return INT; }
	break;
	case 85:
	{ mschlval.ival=kMTV_alerttrans1; return INT; }
	break;
	case 86:
	{ mschlval.ival=kMTV_alerttrans2; return INT; }
	break;
	case 87:
	{ mschlval.ival=kMTV_alerttrans3; return INT; }
	break;
	case 88:
	{ mschlval.ival=kMTV_die; return INT; }        
	break;
	case 89:
	{ mschlval.ival=kMTV_hurt_light; return INT; }
	break;
	case 90:
	{ mschlval.ival=kMTV_hurt_heavy; return INT; }
	break;
	case 91:
	{
   int val;

   ConfigSpew("lexspew", ("schlex: ident\n"));
   if (!IncTabLookup(mschtext, &val))
   {
      mschlval.strval = local_strdup(mschtext);
      return IDENT;
   }
   else
   {
      mschlval.ival = val;
      return INT;
   }
}
	break;
	case 92:
	{
   ConfigSpew("lexspew", ("schlex: string\n"));
   mschlval.strval = local_strdup(mschtext);
   return STRING;
}
	break;
	case 93:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.ival = atoi(mschtext);
   return INT;
}
	break;
	case 94:
	{
   ConfigSpew("lexspew", ("schlex: int\n"));
   mschlval.fval = atof(mschtext);
   return FLOAT;
}
	break;
	case 95:
	return EQUAL;
	break;
	case 96:
	return COLON;
	break;
	case 97:
	return LPAREN;
	break;
	case 98:
	return RPAREN;
	break;
	case 99:
	;
	break;


	}
	YY_SCANNER;
	i = mschleng;
	goto msch_again;			/* action fell though */

  msch_reject:
	YY_SCANNER;
	i = mscholeng;			/* restore original mschtext */
	if (++mschfmin < mschfmax)
		goto msch_found;		/* another final state, same length */
	else
		goto msch_search;		/* try shorter mschtext */

  msch_more:
	YY_SCANNER;
	i = mschleng;
	if (i > 0)
		msch_lastc = mschtext[i-1];
	goto msch_contin;
}
/*
 * Safely switch input stream underneath LEX
 */
typedef struct msch_save_block_tag {
	FILE	* oldfp;
	int	oldline;
	int	oldend;
	int	oldstart;
	int	oldlastc;
	int	oldleng;
	char	savetext[YYLMAX+1];
	msch_state_t	savestate[YYLMAX+1];
} YY_SAVED;

YY_SAVED *
mschSaveScan(fp)
FILE * fp;
{
	YY_SAVED * p;

	if ((p = (YY_SAVED *) malloc(sizeof(*p))) == NULL)
		return p;

	p->oldfp = mschin;
	p->oldline = mschlineno;
	p->oldend = msch_end;
	p->oldstart = msch_start;
	p->oldlastc = msch_lastc;
	p->oldleng = mschleng;
	(void) memcpy(p->savetext, mschtext, sizeof mschtext);
	(void) memcpy((char *) p->savestate, (char *) msch_sbuf,
		sizeof msch_sbuf);

	mschin = fp;
	mschlineno = 1;
	YY_INIT;

	return p;
}
/*f
 * Restore previous LEX state
 */
void
mschRestoreScan(p)
YY_SAVED * p;
{
	if (p == NULL)
		return;
	mschin = p->oldfp;
	mschlineno = p->oldline;
	msch_end = p->oldend;
	msch_start = p->oldstart;
	msch_lastc = p->oldlastc;
	mschleng = p->oldleng;

	(void) memcpy(mschtext, p->savetext, sizeof mschtext);
	(void) memcpy((char *) msch_sbuf, (char *) p->savestate,
		sizeof msch_sbuf);
	free(p);
}
/*
 * User-callable re-initialization of mschlex()
 */
void
msch_reset()
{
	YY_INIT;
	mschlineno = 1;		/* line number */
}
/* get input char with pushback */
YY_DECL int
input()
{
	int c;
#ifndef YY_PRESERVE
	if (msch_end > mschleng) {
		msch_end--;
		memmove(mschtext+mschleng, mschtext+mschleng+1,
			(size_t) (msch_end-mschleng));
		c = msch_save;
		YY_USER;
#else
	if (msch_push < msch_save+YYLMAX) {
		c = *msch_push++;
#endif
	} else
		c = mschgetc();
	msch_lastc = c;
	if (c == YYNEWLINE)
		mschlineno++;
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
	if (msch_end >= YYLMAX)
		YY_FATAL(gettext("Push-back buffer overflow"));
	if (msch_end > mschleng) {
		mschtext[mschleng] = msch_save;
		memmove(mschtext+mschleng+1, mschtext+mschleng,
			(size_t) (msch_end-mschleng));
		mschtext[mschleng] = 0;
	}
	msch_end++;
	msch_save = c;
#else
	if (msch_push <= msch_save)
		YY_FATAL(gettext("Push-back buffer overflow"));
	*--msch_push = c;
#endif
	if (c == YYNEWLINE)
		mschlineno--;
	return c;
}


BOOL OpenMschin(char *file_name)
{
   FILE *in_file;

   if (mschin != NULL)
      fclose(mschin);
   if (!(in_file = fopen(file_name, "r")))
   {
      Warning(("OpenMschIn: can't open file %s for reading\n",
               file_name));
      return FALSE;
   }
   mschin = in_file;
   return TRUE;
}

extern int mschparse();

void MschParseFile(char *schemaFile)
{
   mschin = NULL;
   msch_reset();
   if (!OpenMschin(schemaFile))
      return;
   while (!feof(mschin))
      mschparse();
   fclose(mschin);
}

int mschwrap(void) { return 1;}



