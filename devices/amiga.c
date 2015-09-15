#include <stdio.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "minilisp.h"
#include "alloc.h"

#define WIDTH 320
#define HEIGHT 200
#define BPP 1
#define DEPTH 8

struct Library *intuition_base;
struct Library *gfx_base;
struct Window *window;
struct Cell* buffer_cell;

Cell* amiga_fbfs_open() {
  return alloc_int(1);
}

Cell* amiga_fbfs_read() {
  return alloc_int(0);
}

Cell* amiga_fbfs_write(Cell* arg) {
  // TODO: render buffer to window rastport using chunky->planar conversion

  uint8_t* src  = (uint8_t*)buffer_cell->ar.addr;
  uint8_t* dest1 = (uint8_t*)window->RPort->BitMap->Planes[0];
  uint8_t* dest2 = (uint8_t*)window->RPort->BitMap->Planes[1];
  int i, j, x, y;

  uint32_t screenw = window->WScreen->Width;
  uint32_t pitch;
  uint32_t offset;
  int bitplanes;
  int bpr;

  bitplanes=window->RPort->BitMap->Depth;
  bpr=window->RPort->BitMap->BytesPerRow;
  offset=window->LeftEdge/8 + ((window->TopEdge)*bpr);
  pitch=bpr - window->Width/8;

  /*printf("topedge: %d barheight: %d\r\n",window->TopEdge,window->WScreen->BarLayer->Height);
  printf("screenw: %d winw: %d\r\n",screenw,window->Width);
  printf("pitch: %d\r\n",pitch);
  printf("offset: %d\r\n",offset);
  printf("src: %p\r\n",src);
  printf("bitplane: %p\r\n",dest1);*/

  dest1+=offset;
  dest2+=offset;

  j=0;
  // 8 bytes become 1
  for (y=0; y<HEIGHT; y++) {
    for (i=0; i<WIDTH; i+=8) {
      uint8_t d = 0;
      d|=((*src++)&1)<<7;
      d|=((*src++)&1)<<6;
      d|=((*src++)&1)<<5;
      d|=((*src++)&1)<<4;
      d|=((*src++)&1)<<3;
      d|=((*src++)&1)<<2;
      d|=((*src++)&1)<<1;
      d|=((*src++)&1);
    
      *dest1++ = d;
      *dest2++ = d;
    }
    dest1+=pitch;
    dest2+=pitch;
    //printf("line: %d\r\n",y);
  }
  
  return NULL;
}

Cell* amiga_fbfs_mmap(Cell* arg) {
  long sz = WIDTH*HEIGHT*BPP;
  int x,y;
  uint8_t* dest;
  buffer_cell = alloc_num_bytes(sz);
  printf("[amiga_fbfs_mmap] buffer_cell->addr: %p\n",buffer_cell->ar.addr);
  
  window = OpenWindowTags(NULL, WA_Title, (ULONG) "interim/amiga",
    WA_Left, 320,
    WA_Top, 20,
    WA_Width, WIDTH,
    WA_Height, HEIGHT,
    WA_Flags, WFLG_CLOSEGADGET | WFLG_ACTIVATE | WFLG_NOCAREREFRESH,
    WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP,
                          TAG_DONE);
  
  return buffer_cell;
}

void mount_amiga_fbfs() {
  fs_mount_builtin("/fb", amiga_fbfs_open, amiga_fbfs_read, amiga_fbfs_write, 0, amiga_fbfs_mmap);
}

void uart_puts(char* str) {
  printf(str);
}

void uart_putc(char c) {
  printf("%c",c);
}

void handle_window_events(struct Window *);

void cleanup_amiga() {
  if (window) CloseWindow(window);
  if (gfx_base) CloseLibrary(gfx_base);
  if (intuition_base) CloseLibrary(intuition_base);
}

void mount_amiga() {
  // TODO: exit if stack too small
  
  atexit(cleanup_amiga);
  intuition_base = OpenLibrary("intuition.library", 37);
  
  if (intuition_base == NULL) {
    printf("error: could not open intuition.library v37\r\n");
    return;
  }

  mount_amiga_fbfs();
  //amiga_fbfs_mmap(NULL);
}
