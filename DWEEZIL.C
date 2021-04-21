#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <mem.h>

#include "types.h"
#include "vga.h"
#include "pal.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define PIECE_SIZE 16
#define NUM_PIECES_X 15
#define NUM_PIECES_Y 15

const word buf_width = PIECE_SIZE * NUM_PIECES_X;
const word buf_height = PIECE_SIZE * NUM_PIECES_Y;

byte far *framebuf;
byte far *data_chunks;
byte pal[768];

int rotation = 1;
int zoom = -1;
int do_shift = 1;

#define SETPIX(x,y,c) *(framebuf + (dword)buf_width * (y) + (x)) = c
#define GETPIX(x,y) *(framebuf + (dword)buf_width * (y) + (x))

void
draw_dweezil()
{
  int x, y, i;
  byte color;
  static int pos = 0;
  int xs, ys, xd, yd, s;
  int w = buf_width;
  int h = buf_height;
  byte shift;

  /* shuffle image center deterministically */
  pos++;
  shift = 0;
  if( do_shift ) {
    for( i = 0; i < 4; i++ ) {
      shift <<= 1;
      shift |= (pos >> i) & 1;
    }

    /* inject some long-term variation */
    shift = shift ^ (pos >> 4) & 0x0f;
  }

  /* seed new random pixels in the center */
  for( y = -(PIECE_SIZE>>1); y <= (PIECE_SIZE>>1); y++ ) {
    for( x = -(PIECE_SIZE>>1); x <= (PIECE_SIZE>>1); x++) {
      /* color = (color + 1) % 256; */
      color = random(256);
      SETPIX( w/2 + x, h/2 + y, color );
    }
  }

  for( y = 0; y < NUM_PIECES_Y; y++ ) {
    for( x = 0; x < NUM_PIECES_X; x++ ) {
      xs = x * PIECE_SIZE + shift + (x - (NUM_PIECES_X / 2)) * zoom - (y - (NUM_PIECES_Y / 2)) * rotation;
      ys = y * PIECE_SIZE + shift + (y - (NUM_PIECES_Y / 2)) * zoom + (x - (NUM_PIECES_X / 2)) * rotation;
      xd = x * PIECE_SIZE;
      yd = y * PIECE_SIZE;
      memcpy_rect(
	data_chunks, framebuf,
	buf_width, buf_height,
	xs, ys,
	xd, yd,
	PIECE_SIZE, PIECE_SIZE
      );
    }
  }

  for( y = 0; y < NUM_PIECES_Y; y++ ) {
    for( x = 0; x < NUM_PIECES_X; x++ ) {
      xs = x * PIECE_SIZE;
      ys = y * PIECE_SIZE;
      xd = x * PIECE_SIZE + shift;
      yd = y * PIECE_SIZE + shift;
      memcpy_rect(
	framebuf, data_chunks,
	buf_width, buf_height,
	xs, ys,
	xd, yd,
	PIECE_SIZE, PIECE_SIZE
      );
    }
  }
}

int
main()
{
  char kc = 0;
  int i, y;
  const word buf_size = buf_width * buf_height;

  framebuf = farmalloc( buf_size );
  data_chunks = farmalloc( buf_size );
  if(framebuf == NULL || data_chunks == NULL) {
    printf("not enough memory\n");
    return 1;
  }

  set_graphics_mode();
  set_palette( fire_pal );
  memset(framebuf, 0x00, buf_size);
  memset(data_chunks, 0x00, buf_size);
  while( kc != 0x1b ) {
    if( kbhit() ) {
      kc = getch();
      switch( kc ) {
      case 'r':
	rotation++;
	if( rotation > 1 ) rotation = -1;
	break;
      case 's':
	do_shift = !do_shift;
	break;
      case 'z':
	zoom++;
	if( zoom > 1 ) zoom = -1;
	break;
      default:
	break;
      }
    }
    draw_dweezil();
    wait_for_retrace();
    for( y = 0; y < (buf_height < 200 ? buf_height : 200); ++y ) {
      memcpy(
	VGA + y * SCREEN_WIDTH,
	framebuf + y * buf_width,
	buf_width
      );
    }
  }
  set_text_mode();
  return 0;
}
