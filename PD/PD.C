#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <dos.h>
#include <mem.h>

#include "stdbool.h"

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

#define PALETTE_IMPLEMENTATION
#include "palette.h"

#define CANVAS_IMPLEMENTATION
#include "canvas.h"

#define GRID_IMPLEMENTATION
#include "grid.h"

#define TOOL_IMPLEMENTATION
#include "tool.h"



#define _NOCURSOR     0x3200
#define _NORMALCURSOR 0x0607
#define _SOLIDCURSOR  0x0007

dword pal[]={
	0x001A1C2CL,
	0x005D275DL,
	0x00B13E53L,
	0x00EF7D57L,
	0x00FFCD75L,
	0x00A7F070L,
	0x0038B764L,
	0x00257179L,
	0x0029366FL,
	0x003B5DC9L,
	0x0041A676L,
	0x0073EFF7L,
	0x00F4F4F4L,
	0x0094B0C2L,
	0x00566C86L,
	0x00333C57L
};

typedef enum GameState {
	GAMESTATE_CANVAS=0,
	GAMESTATE_PALETTE,
	GAMESTATE_TOOL,
	GAMESTATE_MAX
} GameState;

typedef enum ToolType {
  TOOLTYPE_PEN=0,
	TOOLTYPE_OVAL,
	TOOLTYPE_FOVAL,
  TOOLTYPE_FFILL,
	TOOLTYPE_LINE,
	TOOLTYPE_RECT,
	TOOLTYPE_FRECT,
  TOOLTYPE_CLEAR,
	TOOLTYPE_MAX
} ToolType;

byte normal_keys[0x60];
byte extended_keys[0x60];

byte keys[16];

static void _setcursortype(int type) {
	union REGS in,out;
	in.x.cx=type;
	in.h.ah=1;
	int86(0x10,&in,&out);
}

static void interrupt
keyb_int() {
    static unsigned char buffer;
    unsigned char rawcode;
    unsigned char make_break;
    int scancode;

    rawcode = inp(0x60); /* read scancode from keyboard controller */
    make_break = !(rawcode & 0x80); /* bit 7: 0 = make, 1 = break */
    scancode = rawcode & 0x7F;

    if (buffer == 0xE0) { /* second byte of an extended key */
        if (scancode < 0x60) {
            extended_keys[scancode] = make_break;
        }
        buffer = 0;
    } else if (buffer >= 0xE1 && buffer <= 0xE2) {
        buffer = 0; /* ingore these extended keys */
    } else if (rawcode >= 0xE0 && rawcode <= 0xE2) {
        buffer = rawcode; /* first byte of an extended key */
    } else if (scancode < 0x60) {
        normal_keys[scancode] = make_break;
    }

    outp(0x20, 0x20); /* must send EOI to finish interrupt */
}

static void interrupt (*old_keyb_int)();

void
hook_keyb_int(void) {
    old_keyb_int = getvect(0x09);
    setvect(0x09, keyb_int);
}

void
unhook_keyb_int(void) {
    if (old_keyb_int != NULL) {
        setvect(0x09, old_keyb_int);
        old_keyb_int = NULL;
    }
}

int
ctrlbrk_handler(void) {
    unhook_keyb_int();
    _setcursortype(_NORMALCURSOR);
    return 0;
}

static
putkeys(int y, unsigned char const *keys) {
    int i;
    gotoxy(1, y);
    for (i = 0; i < 0x30; i++) {
        putch(keys[i] + '0');
    }
}

void
game(void) {
	int i,j,k;

	int tx=0,ty=0;

	Palette *palette=NULL;
	Canvas *canvas=NULL;
	Grid *grid=NULL;
	Tool *tool=NULL;

	bool hold=false;
	bool select=false;

	int blink=0;
  int blinkMax=500;
	int move=0;
  int moveMax=100;
  int blinkDelay=blinkMax/2;
  int moveDelay=moveMax/2;
  int moveCounter=0;

	int currentTool=TOOLTYPE_PEN;
	int currentColor=0;

	GameState gameState=GAMESTATE_CANVAS;

	for(i=0;i<16;i++) keys[i]=0;


	_setcursortype(_NOCURSOR);

	clrscr();

	SetMode(0x13);

	for(i=0;i<16;i++) {
		int r=(pal[i] & 0x00FF0000L) >> 16;
		int g=(pal[i] & 0x0000FF00L) >> 8;
		int b=(pal[i] & 0x000000FFL);
		r=(int)(r/255.0*63.0);
		g=(int)(g/255.0*63.0);
		b=(int)(b/255.0*63.0);

		SetPalette(i,r,g,b);
	}

	for(i=17;i<256;i++) {
		SetPalette(i,0,0,0);
	}

	palette=Palette_New(0,SCREEN_HEIGHT-32,12);

	canvas=Canvas_LoadCVS("CANVAS.CVS");
	if(canvas==NULL) {
		printf("Error: Canvas_LoadCVS\n");
		exit(1);
	}

	grid=Grid_New(canvas,0,0,canvas->w,canvas->h,8,15,12,true);

	tool=Tool_New(16*8,SCREEN_HEIGHT-32,12);

	Palette_Draw(VGA,palette);
	Grid_Draw(VGA,grid);
	Tool_Draw(VGA,tool);

	DrawLine(
		VGA,
		tx*grid->zoom+1,
		ty*grid->zoom+1,
		tx*grid->zoom+grid->zoom-2,
		ty*grid->zoom+grid->zoom-2,
		canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
	);

	DrawLine(
		VGA,
		tx*grid->zoom+grid->zoom-2,
		ty*grid->zoom+1,
		tx*grid->zoom+1,
		ty*grid->zoom+grid->zoom-2,
		canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
	);

	DrawRect(
		VGA,
		palette->cursorX*16+palette->x,
		palette->cursorY*16+palette->y,
		16,16,
		palette->cursorColor
	);

	DrawRect(
		VGA,
		palette->cursorX*16+palette->x+1,
		palette->cursorY*16+palette->y+1,
		16-2,16-2,
		0
	);

	if(grid->show) {
		DrawRect(
			VGA,
			tx*grid->zoom,
			ty*grid->zoom,
			grid->zoom,
			grid->zoom,
			grid->color
		);
	}

	DrawRect(
		VGA,
		tool->cursorX*16+tool->x,
		tool->cursorY*16+tool->y,
		16,16,
		tool->cursorColor
	);


	while(!normal_keys[1]) {

/*
		putkeys(1, normal_keys);
		putkeys(2, normal_keys + 0x30);
		putkeys(4, extended_keys);
		putkeys(5, extended_keys + 0x30);
*/

		keys[ 0]=normal_keys[ 2]; /* 1 */
		keys[ 1]=normal_keys[ 3]; /* 2 */
		keys[ 2]=normal_keys[ 4]; /* 3 */
		keys[ 3]=normal_keys[ 5]; /* 4 */
		keys[ 4]=normal_keys[16]; /* Q */
		keys[ 5]=normal_keys[17]; /* W */
		keys[ 6]=normal_keys[18]; /* E */
		keys[ 7]=normal_keys[19]; /* R */
		keys[ 8]=normal_keys[30]; /* A */
		keys[ 9]=normal_keys[31]; /* S */
		keys[10]=normal_keys[32]; /* D */
		keys[11]=normal_keys[33]; /* F */
		keys[12]=normal_keys[44]; /* Z */
		keys[13]=normal_keys[45]; /* X */
		keys[14]=normal_keys[46]; /* C */
		keys[15]=normal_keys[47]; /* V */



		switch(gameState) {

		case GAMESTATE_CANVAS:

      if(keys[15]) {
				select=true;
        hold=true;
			}

			if(select) {
				switch(currentTool) {
				case TOOLTYPE_PEN:

					k=canvas->w*grid->cursorY+grid->cursorX;

					canvas->pixels[k]=currentColor;

					FillRect(
						VGA,
						grid->cursorX*grid->zoom,
						grid->cursorY*grid->zoom,
						grid->zoom,
						grid->zoom,
						canvas->pixels[k]
          );

          break;

        case TOOLTYPE_RECT: {
            int x0=tx,y0=ty,x1=grid->cursorX,y1=grid->cursorY,t;

            if(x0>x1) { t=x0; x0=x1; x1=t; }
            if(y0>y1) { t=y0; y0=y1; y1=t; }

            Canvas_DrawRect(
              canvas,
              x0,
              y0,
              x1-x0+1,
              y1-y0+1,
              currentColor
            );
            Grid_Draw(VGA,grid);
            select=false;
          }
          break;
				case TOOLTYPE_OVAL: {
						int x0=tx,y0=ty,x1=grid->cursorX,y1=grid->cursorY;

						if(x0>x1) { int t=x0; x0=x1; x1=t; }
						if(y0>y1) { int t=y0; y0=y1; y1=t; }

						Canvas_DrawOval(canvas,x0,y0,x1,y1,currentColor,false);
            Grid_Draw(VGA,grid);
					}
					break;
				case TOOLTYPE_LINE:
          Canvas_DrawLine(
						canvas,
						grid->cursorX,
						grid->cursorY,
						tx,ty,
						currentColor
					);
					Grid_Draw(VGA,grid);
					select=false;
          break;
        case TOOLTYPE_FRECT: {
            int x0=tx,y0=ty,x1=grid->cursorX,y1=grid->cursorY,t;

            if(x0>x1) { t=x0; x0=x1; x1=t; }
            if(y0>y1) { t=y0; y0=y1; y1=t; }

            Canvas_FillRect(
              canvas,
              x0,
              y0,
              x1-x0+1,
              y1-y0+1,
              currentColor
            );
            Grid_Draw(VGA,grid);
            select=false;
          }
					break;
				case TOOLTYPE_FOVAL: {
						int x0=tx,y0=ty,x1=grid->cursorX,y1=grid->cursorY;

						if(x0>x1) { int t=x0; x0=x1; x1=t; }
						if(y0>y1) { int t=y0; y0=y1; y1=t; }

						Canvas_DrawOval(canvas,x0,y0,x1,y1,currentColor,true);
            Grid_Draw(VGA,grid);
					}
					break;
        case TOOLTYPE_FFILL:
          break;
        case TOOLTYPE_CLEAR:
          break;
				default:
					break;
				}
			}



			if(keys[13]) {

				FillRect(
					VGA,
					tx*grid->zoom,
					ty*grid->zoom,
					grid->zoom,
					grid->zoom,
					canvas->pixels[canvas->w*ty+tx]
				);

				if(grid->show) {
					DrawRect(
						VGA,
						tx*grid->zoom,
						ty*grid->zoom,
						grid->zoom,
						grid->zoom,
						grid->color
					);
        }

				tx=grid->cursorX; ty=grid->cursorY;

				DrawLine(
					VGA,
					tx*grid->zoom+1,
					ty*grid->zoom+1,
					tx*grid->zoom+grid->zoom-2,
					ty*grid->zoom+grid->zoom-2,
					canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
				);

				DrawLine(
					VGA,
					tx*grid->zoom+grid->zoom-2,
					ty*grid->zoom+1,
					tx*grid->zoom+1,
					ty*grid->zoom+grid->zoom-2,
					canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
				);

				if(grid->show) {
					DrawRect(
						VGA,
						tx*grid->zoom,
						ty*grid->zoom,
						grid->zoom,
						grid->zoom,
						grid->color
					);
				}
			}

      if(keys[ 5]) { if(move==0) grid->cursorY--; }
      if(keys[ 8]) { if(move==0) grid->cursorX--; }
      if(keys[ 9]) { if(move==0) grid->cursorY++; }
      if(keys[10]) { if(move==0) grid->cursorX++; }


			if(!hold) {

				if(keys[14]) {
					gameState++;
					if(gameState==GAMESTATE_MAX) gameState=0;
					moveDelay=0;
					hold=true;
				}


			} else {

/*
        if(moveCounter<moveDelay) {
          moveCounter++;
				} else {
          if(keys[ 5]) { if(move==0) grid->cursorY--; }
					if(keys[ 8]) { if(move==0) grid->cursorX--; }
					if(keys[ 9]) { if(move==0) grid->cursorY++; }
					if(keys[10]) { if(move==0) grid->cursorX++; }
				}
*/


				if(
					!(
						keys[ 5] ||
						keys[ 8] ||
						keys[ 9] ||
						keys[10] ||
						keys[14] ||
						keys[15]
					)
				) {
					hold=false;
          select=false;
          moveCounter=0;
				}

			}

			if(grid->cursorX<0) grid->cursorX=canvas->w-1;
			if(grid->cursorY<0) grid->cursorY=canvas->h-1;
			if(grid->cursorX>=canvas->w) grid->cursorX=0;
			if(grid->cursorY>=canvas->h) grid->cursorY=0;

      if(blink<blinkDelay) {
				DrawRect(
					VGA,
					grid->cursorX*grid->zoom,
					grid->cursorY*grid->zoom,
					grid->zoom,
					grid->zoom,
					grid->cursorColor
				);
			} else {
				DrawRect(
					VGA,
					grid->cursorX*grid->zoom,
					grid->cursorY*grid->zoom,
					grid->zoom,
					grid->zoom,
					0
				);
			}

			if(grid->cursorPrevX!=grid->cursorX || grid->cursorPrevY!=grid->cursorY) {

				FillRect(
					VGA,
					grid->cursorX*grid->zoom,
					grid->cursorY*grid->zoom,
					grid->zoom,
					grid->zoom,
					canvas->pixels[canvas->w*grid->cursorY+grid->cursorX]
				);


				DrawLine(
					VGA,
					tx*grid->zoom+1,
					ty*grid->zoom+1,
					tx*grid->zoom+grid->zoom-2,
					ty*grid->zoom+grid->zoom-2,
					canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
				);

				DrawLine(
					VGA,
					tx*grid->zoom+grid->zoom-2,
					ty*grid->zoom+1,
					tx*grid->zoom+1,
					ty*grid->zoom+grid->zoom-2,
					canvas->pixels[canvas->w*ty+tx]==grid->cursorColor?0:grid->cursorColor
				);

				if(grid->show) {
					DrawRect(
						VGA,
						grid->cursorPrevX*grid->zoom,
						grid->cursorPrevY*grid->zoom,
						grid->zoom,
						grid->zoom,
						grid->color
					);
				}

        blink=0;
      }

			grid->cursorPrevX=grid->cursorX;
			grid->cursorPrevY=grid->cursorY;



			break;

		case GAMESTATE_PALETTE:

      if(keys[15]) {
				hold=true;
				select=true;
      }



      if(blink<blinkDelay) {
				DrawRect(
					VGA,
					palette->cursorX*16+palette->x,
					palette->cursorY*16+palette->y,
					16,16,
					palette->cursorColor
				);
				DrawRect(
					VGA,
					palette->cursorX*16+palette->x+1,
					palette->cursorY*16+palette->y+1,
					16-2,16-2,
					0
				);
      } else {
				DrawRect(
					VGA,
					palette->cursorX*16+palette->x,
					palette->cursorY*16+palette->y,
					16,16,
					0
				);
				DrawRect(
					VGA,
					palette->cursorX*16+palette->x+1,
					palette->cursorY*16+palette->y+1,
					16-2,16-2,
					0
				);
			}

			if(	palette->cursorPrevX!=palette->cursorX ||
					palette->cursorPrevY!=palette->cursorY) {

				FillRect(
					VGA,
					palette->cursorPrevX*16+palette->x,
					palette->cursorPrevY*16+palette->y,
					16,16,
					8*palette->cursorPrevY+palette->cursorPrevX
				);

				currentColor=8*palette->cursorY+palette->cursorX;

				blink=0;
			}

			palette->cursorPrevX=palette->cursorX;
			palette->cursorPrevY=palette->cursorY;


			if(!hold) {

				if(keys[ 5]) { palette->cursorY--; hold=true; }
				if(keys[ 8]) { palette->cursorX--; hold=true; }
				if(keys[ 9]) { palette->cursorY++; hold=true; }
        if(keys[10]) { palette->cursorX++; hold=true; }

				if(keys[14]) {

					gameState++;

					if(gameState==GAMESTATE_MAX) gameState=0;

					DrawRect(
						VGA,
						palette->cursorX*16+palette->x,
						palette->cursorY*16+palette->y,
						16,16,
						palette->cursorColor
					);

					DrawRect(
						VGA,
						palette->cursorX*16+palette->x+1,
						palette->cursorY*16+palette->y+1,
						16-2,16-2,
						0
					);

					hold=true;

				}

			} else {

        if(moveDelay<10) {
					moveDelay++;
				} else {
					if(keys[ 5]) { if(move==0) palette->cursorY--; }
					if(keys[ 8]) { if(move==0) palette->cursorX--; }
					if(keys[ 9]) { if(move==0) palette->cursorY++; }
					if(keys[10]) { if(move==0) palette->cursorX++; }
        }

				if(
					!(
						keys[ 5] ||
						keys[ 8] ||
						keys[ 9] ||
						keys[10] ||
            keys[14] ||
            keys[15]
					)
				) {
					if(select) {
						currentColor=8*palette->cursorY+palette->cursorX;
						select=false;
						gameState=GAMESTATE_CANVAS;
					}
					hold=false;
					moveDelay=0;
				}

      }

			if(palette->cursorX<0) palette->cursorX=7;
			if(palette->cursorY<0) palette->cursorY=1;
			if(palette->cursorX>7) palette->cursorX=0;
			if(palette->cursorY>1) palette->cursorY=0;


			break;



		case GAMESTATE_TOOL:



			if(keys[15]) {
        currentTool=4*tool->cursorY+tool->cursorX;
				select=true;
				hold=true;
			}

      if(blink<blinkDelay) {
				DrawRect(
					VGA,
					tool->cursorX*16+tool->x,
					tool->cursorY*16+tool->y,
					16,16,
					tool->cursorColor
				);
      } else {
				DrawRect(
					VGA,
					tool->cursorX*16+tool->x,
					tool->cursorY*16+tool->y,
					16,16,
					0
				);
			}

			if(	tool->cursorPrevX!=tool->cursorX ||
					tool->cursorPrevY!=tool->cursorY) {
				DrawRect(
					VGA,
					tool->cursorPrevX*16+tool->x,
					tool->cursorPrevY*16+tool->y,
					16,16,
					0
				);
        currentTool=4*tool->cursorY+tool->cursorX;
        blink=0;
			}

			tool->cursorPrevX=tool->cursorX;
			tool->cursorPrevY=tool->cursorY;


			if(!hold) {

				if(keys[ 5]) { tool->cursorY--; hold=true; }
				if(keys[ 8]) { tool->cursorX--; hold=true; }
				if(keys[ 9]) { tool->cursorY++; hold=true; }
				if(keys[10]) { tool->cursorX++; hold=true; }

				if(keys[14]) {

					gameState++;

					if(gameState==GAMESTATE_MAX) gameState=0;

					DrawRect(
						VGA,
						tool->cursorX*16+tool->x,
						tool->cursorY*16+tool->y,
						16,16,
						tool->cursorColor
					);

					hold=true;

				}

			} else {

        if(moveCounter<moveDelay) {
          moveCounter++;
				} else {
					if(keys[ 5]) { if(move==0) tool->cursorY--; }
					if(keys[ 8]) { if(move==0) tool->cursorX--; }
					if(keys[ 9]) { if(move==0) tool->cursorY++; }
					if(keys[10]) { if(move==0) tool->cursorX++; }
        }

				if(
					!(
						keys[ 5] ||
						keys[ 8] ||
						keys[ 9] ||
						keys[10] ||
						keys[14] ||
						keys[15]
					)
				) {
          if(select) {

            if(currentTool==TOOLTYPE_CLEAR) {
              Canvas_FillRect(canvas,0,0,canvas->w,canvas->h,currentColor);
              Grid_Draw(VGA,grid);
            }

						DrawRect(
							VGA,
							tool->cursorX*16+tool->x,
							tool->cursorY*16+tool->y,
							16,16,
							tool->cursorColor
						);

            currentTool=4*tool->cursorY+tool->cursorX;
						select=false;
						gameState=GAMESTATE_CANVAS;
					}

					hold=false;
					moveDelay=0;
				}

      }

			if(tool->cursorX<0) tool->cursorX=3;
			if(tool->cursorY<0) tool->cursorY=1;
			if(tool->cursorX>3) tool->cursorX=0;
			if(tool->cursorY>1) tool->cursorY=0;


			break;

		default: break;
		}


		blink++;
		move++;

    if(blink>=blinkMax) blink=0;
    if(move>=moveMax) move=0;


/*
		gotoxy(1,1);
		for(i=0;i<16;i++) {
			if(keys[i]) putchar('1'); else putchar('0');
		}
*/

	}

	Tool_Free(tool);
	Grid_Free(grid);
	Palette_Free(palette);

	SetMode(0x03);

	_setcursortype(_NORMALCURSOR);

	clrscr();
}

int
main() {
    ctrlbrk(ctrlbrk_handler);
    hook_keyb_int();
    game();
    unhook_keyb_int();
    return 0;
}
