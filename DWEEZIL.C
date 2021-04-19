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
#define NUM_PIECES 15
#define BUF_WIDTH 15*16

byte far *framebuf;
byte far *data_chunks;
byte pal[768];

#define SETPIX(x,y,c) *(framebuf + (dword)BUF_WIDTH * (y) + (x)) = c
#define GETPIX(x,y) *(framebuf + (dword)BUF_WIDTH * (y) + (x))

void
draw_dweezil()
{
  int x, y, i;
  byte color;
  static int pos = 0;
  int xs, ys, xd, yd, s;
  int w = PIECE_SIZE * NUM_PIECES;
  int h = PIECE_SIZE * NUM_PIECES;
  byte shift;
  int rotation = 1;
  int zoom = -1;

  /* shuffle image center deterministically */
  pos++;
  shift = 0;
  for( i = 0; i < 4; i++ ) {
    shift <<= 1;
    shift |= (pos >> i) & 1;
  }

  /* inject some long-term variation */
  shift = shift ^ (pos >> 4) & 0x0f;

  /* seed new random pixels in the center */
  for( y = -(PIECE_SIZE>>1); y <= (PIECE_SIZE>>1); y++ ) {
    for( x = -(PIECE_SIZE>>1); x <= (PIECE_SIZE>>1); x++) {
      /* color = (color + 1) % 256; */
      color = random(256);
      SETPIX( w/2 + x, h/2 + y, color );
    }
  }

  for( y = 0; y < NUM_PIECES; y++ ) {
    for( x = 0; x < NUM_PIECES; x++ ) {
      xs = x * PIECE_SIZE + shift + (x - (NUM_PIECES>>1)) * zoom - (y - (NUM_PIECES>>1)) * rotation;
      ys = y * PIECE_SIZE + shift + (y - (NUM_PIECES>>1)) * zoom + (x - (NUM_PIECES>>1)) * rotation;
      xd = x * PIECE_SIZE;
      yd = y * PIECE_SIZE;
      memcpy_rect(
	data_chunks, framebuf,
	BUF_WIDTH, BUF_WIDTH,
	xs, ys,
	xd, yd,
	PIECE_SIZE, PIECE_SIZE
      );
    }
  }

  for( y = 0; y < NUM_PIECES; y++ ) {
    for( x = 0; x < NUM_PIECES; x++ ) {
      xs = x * PIECE_SIZE;
      ys = y * PIECE_SIZE;
      xd = x * PIECE_SIZE + shift;
      yd = y * PIECE_SIZE + shift;
      memcpy_rect(
	framebuf, data_chunks,
	BUF_WIDTH, BUF_WIDTH,
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
  byte do_quit = 0;
  int i, y;
  const int sz = PIECE_SIZE * NUM_PIECES;
  word buf_size = sz * sz;

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
  while( !do_quit ) {
    if(kbhit()){
      getch();
      do_quit = 1;
    }
    draw_dweezil();
    wait_for_retrace();
    for( y = 0; y < (sz < 200 ? sz : 200); ++y ) {
      memcpy(
	VGA + y * SCREEN_WIDTH,
	framebuf + y * sz,
	sz
      );
    }
  }
  set_text_mode();
  return 0;
}
