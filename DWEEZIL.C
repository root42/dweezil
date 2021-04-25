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
#define NUM_PIECES_X 13
#define NUM_PIECES_Y 13

const word buf_width = PIECE_SIZE * NUM_PIECES_X;
const word buf_height = PIECE_SIZE * NUM_PIECES_Y;

byte far *framebuf;
byte far *data_chunks;
byte pal[768];

int rotation = 1;
int zoom = -1;
int do_shift = 1;

#define SETPIX(x,y,c) *(framebuf + (dword)buf_width * (y) + (x)) = c

void
draw_dweezil()
{
  int x, y, i;
  byte color;
  int xs, ys, xd, yd, s;
  int w = buf_width;
  int h = buf_height;
  byte shift;

  /* shuffle image center */
  shift = 0;
  if( do_shift ) {
    shift = random(16);
  }

  /* seed new random pixels in the center */
  for( y = 0; y <= PIECE_SIZE/2; y++ ) {
    for( x = 0; x <= PIECE_SIZE/2; x++) {
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

  memcpy(
    framebuf + shift + shift * buf_width,
    data_chunks,
    buf_width*buf_height
  );
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
  memset(framebuf, 0x00, buf_size);
  memset(data_chunks, 0x00, buf_size);

  set_graphics_mode();
  set_palette( fire_pal );

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
    for( y = PIECE_SIZE/2; y < (buf_height < 200-PIECE_SIZE/2 ? buf_height : 200-PIECE_SIZE/2); ++y ) {
      memcpy(
	VGA + (SCREEN_WIDTH / 2) - (buf_width - PIECE_SIZE) / 2 + y * SCREEN_WIDTH,
	framebuf + PIECE_SIZE + y * buf_width,
	buf_width - PIECE_SIZE
      );
    }
  }
  set_text_mode();
  return 0;
}

