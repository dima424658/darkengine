/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/gifdump.c $
 * $Revision: 1.1 $
 * $Author: JAEMZ $
 * $Date: 1996/01/19 14:06:34 $
 *
 * Gif Dumper 
 *
*/

#include <io.h>  
#include <2d.h>

#define largest_code	4095		/* largest possible code */
#define table_size	5003		/* table dimensions */

struct {
	char sig[6];
	ushort screenwidth,screendepth;
	uchar flags,background,aspect;
} gif_GifHeader = {"GIF87a",320,200,0xF7,0,0};

struct {
	char separator;
	ushort left,top,width,depth;
	uchar flags;
} gif_ImageBlock = {',',0,0,320,200,7};

short *gif_oldcode;
short *gif_currentcode;
uchar *gif_code_buffer;
uchar *gif_newcode;
short gif_code_size;
short gif_clear_code;
short gif_eof_code;
short gif_bit_offset;
short gif_byte_offset;
short gif_bits_left;
short gif_max_code;
short gif_free_code;
short gif_line_offset;
short gif_line_number;

void flush(int fp,uchar n);
void write_code(int fp,int code);
void compressImage(int fp,uint min_code_size);
int readpixel(void);

/* mem_block needs to be 26K, you should have the screen set
 * to the current canvas
*/
int dmp_gif_dump_screen(int fp, uchar *buf)
{

	/* Allocate buffer space from one buffer */
	gif_oldcode = (short *)buf;       // table size big
	gif_currentcode = gif_oldcode+table_size; // table size big
	gif_newcode = (uchar *)(gif_currentcode+table_size); //table size big
	gif_code_buffer = gif_newcode+table_size; // who knows, probably the same

	/* Initialize screen buffer stuff */
	gif_line_offset = 0;
	gif_line_number = 0;
	
	/* write the header */
   gif_GifHeader.screenwidth = grd_bm.w;
   gif_GifHeader.screendepth = grd_bm.h;
	write(fp,(char *)&gif_GifHeader,sizeof(gif_GifHeader));

	/* write the color map 8 bit */
	write(fp,grd_pal,768);

	/* write the image descriptor */
   gif_ImageBlock.width = grd_bm.w;
   gif_ImageBlock.depth = grd_bm.h;
	write(fp,(char *)&gif_ImageBlock,sizeof(gif_ImageBlock));

	/* write the image */
	compressImage(fp,8);

	/* write the terminator */
	write(fp,";",1);

	close(fp);

   return 0;
}

/* initialize the code table */
static void init_table(int min_code_size)
{
	int i;

	gif_code_size=min_code_size+1;
	gif_clear_code=(1<<min_code_size);
	gif_eof_code=gif_clear_code+1;
	gif_free_code=gif_clear_code+2;
	gif_max_code=(1<<gif_code_size);

	for(i=0;i<table_size;i++) gif_currentcode[i]=0;
}

/* flush the code buffer */
/* Dump the block stored in code_buffer */
/* preceded by the length.  This should only */
/* need a maximum length of 255 */
static void flush(int fp,unsigned char n)
{
	write(fp,&n,1);
	write(fp,gif_code_buffer,n);
}

/* write a code to the code buffer */
/* The code buffer takes the n-length code */
/* and takes it onto the buffer */
/* Only need fp to pass it on */
static void write_code(int fp,int code)
{
	long temp;

	gif_byte_offset = gif_bit_offset >> 3;
	gif_bits_left = gif_bit_offset & 7;

	if(gif_byte_offset >= 254) {
		flush(fp,gif_byte_offset);
		gif_code_buffer[0] = gif_code_buffer[gif_byte_offset];
		gif_bit_offset = gif_bits_left;
		gif_byte_offset = 0;
	}

	if(gif_bits_left > 0) {
		temp = ((long) code << gif_bits_left) | gif_code_buffer[gif_byte_offset];
		gif_code_buffer[gif_byte_offset]=temp;
		gif_code_buffer[gif_byte_offset+1]=(temp >> 8);
		gif_code_buffer[gif_byte_offset+2]=(temp >> 16);
	}
	else {
		gif_code_buffer[gif_byte_offset] = code;
		gif_code_buffer[gif_byte_offset+1]=(code >> 8);
	}
	gif_bit_offset += gif_code_size;
}

/* compress an image */
static void compressImage(int fp,unsigned int min_code_size)
{
	int prefix_code;
	int suffix_char;
	int hx,d;

	/* write initial code size */
	write(fp,(char *)&min_code_size,1);

	/* initialize the encoder */
	gif_bit_offset=0;
	init_table(min_code_size);
	write_code(fp,gif_clear_code);

	/* initialize the prefix */
	prefix_code = readpixel();

	/* get a character to compress */
	while((suffix_char = readpixel()) != -1) {

		/* derive an index into the code table */
		hx=(prefix_code ^ (suffix_char << 5)) % table_size;
		d=1;

      while(TRUE) {

			/* see if the code is in the table */
			if(gif_currentcode[hx] == 0) {

				/* if not, put it there */
				write_code(fp,prefix_code);
				d = gif_free_code;

				/* find the next free code */
				if(gif_free_code <= largest_code) {
					gif_oldcode[hx] = prefix_code;
					gif_newcode[hx] = suffix_char;
					gif_currentcode[hx] = gif_free_code;
					gif_free_code++;
				}

				/* expand the code size or scrap the table */
				if(d == gif_max_code) {
					if(gif_code_size < 12) {
						gif_code_size++;
						gif_max_code <<= 1;
					}
					else {
						write_code(fp,gif_clear_code);
						init_table(min_code_size);
					}
				}
				prefix_code = suffix_char;
				break;
			}
			if(gif_oldcode[hx] == prefix_code &&
			   gif_newcode[hx] == suffix_char) {
				prefix_code = gif_currentcode[hx];
				break;
			}
			hx += d;
			d += 2;
			if(hx >= table_size) hx -= table_size;
		}
	}

	/* write the prefix code */
	write_code(fp,prefix_code);

	/* and the end of file code */
	write_code(fp,gif_eof_code);

	/* flush the buffer */
	if(gif_bit_offset > 0) flush(fp,(gif_bit_offset+7)/8);

	/* write a zero length block */
	flush(fp,0);
}

// If this is the only place gif_scrn_buf is used we can get
// along with out it
static int readpixel()
{
   // cr if end of line
	if (gif_line_offset == grd_bm.w) {
		gif_line_offset = 0;
		++gif_line_number;
	}

   // abort if done
   if (gif_line_number == grd_bm.h) return -1;

   return gr_get_pixel(gif_line_offset++,gif_line_number);
}
