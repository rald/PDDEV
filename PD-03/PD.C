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
	GAMESTATE_MAX
} GameState;

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

	bool hold=false;

	int blink=0;
	int move=0;

	GameState gameState=GAMESTATE_CANVAS;



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
	canvas=Canvas_New(0,0,16,16,8,true,15,12);

	Palette_Draw(VGA,palette);
	Canvas_Draw(VGA,canvas);


	DrawLine(
		VGA,
		tx*canvas->zoom+1,
		ty*canvas->zoom+1,
		tx*canvas->zoom+canvas->zoom-2,
		ty*canvas->zoom+canvas->zoom-2,
		canvas->cursorColor
	);

	DrawLine(
		VGA,
		tx*canvas->zoom+canvas->zoom-2,
		ty*canvas->zoom+1,
		tx*canvas->zoom+1,
		ty*canvas->zoom+canvas->zoom-2,
		canvas->cursorColor
	);

	getch();

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

	if(canvas->gridShow) {
		DrawRect(
			VGA,
			tx*canvas->zoom,
			ty*canvas->zoom,
			canvas->zoom,
			canvas->zoom,
			canvas->gridColor
		);
	}

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
				canvas->pixels[canvas->w*canvas->cursorY+canvas->cursorX]=8*palette->cursorY+palette->cursorX;
				FillRect(
					VGA,
					canvas->cursorX*canvas->zoom,
					canvas->cursorY*canvas->zoom,
					canvas->zoom,
					canvas->zoom,
					canvas->pixels[canvas->w*canvas->cursorY+canvas->cursorX]
				);
			}


			if(keys[13]) {

				FillRect(
					VGA,
					tx*canvas->zoom,
					ty*canvas->zoom,
					canvas->zoom,
					canvas->zoom,
					canvas->pixels[canvas->w*ty+tx]
				);

				if(canvas->gridShow) {
					DrawRect(
						VGA,
						tx*canvas->zoom,
						ty*canvas->zoom,
						canvas->zoom,
						canvas->zoom,
						canvas->gridColor
					);
        }


				tx=canvas->cursorX; ty=canvas->cursorY;

				DrawLine(
					VGA,
					tx*canvas->zoom,
					ty*canvas->zoom,
					tx*canvas->zoom+canvas->zoom-1,
					ty*canvas->zoom+canvas->zoom-1,
					canvas->cursorColor
				);

				DrawLine(
					VGA,
					tx*canvas->zoom+canvas->zoom-1,
					ty*canvas->zoom,
					tx*canvas->zoom,
					ty*canvas->zoom+canvas->zoom-1,
					canvas->cursorColor
				);

				if(canvas->gridShow) {
					DrawRect(
						VGA,
						tx*canvas->zoom,
						ty*canvas->zoom,
						canvas->zoom,
						canvas->zoom,
						canvas->gridColor
					);
				}
			}

			if(keys[ 5]) { if(move==0) canvas->cursorY--; }
			if(keys[ 8]) { if(move==0) canvas->cursorX--; }
			if(keys[ 9]) { if(move==0) canvas->cursorY++; }
			if(keys[10]) { if(move==0) canvas->cursorX++; }

			if(canvas->cursorX<0) canvas->cursorX=canvas->w-1;
			if(canvas->cursorY<0) canvas->cursorY=canvas->h-1;
			if(canvas->cursorX>=canvas->w) canvas->cursorX=0;
			if(canvas->cursorY>=canvas->h) canvas->cursorY=0;

			if(!hold) {
				if(keys[14]) {
					gameState++;
					if(gameState==GAMESTATE_MAX) gameState=0;
					hold=true;
				}
			} else {
				if(
					!(
						keys[14]
					)
				) hold=false;
      }

			if(blink<5000) {
				DrawRect(
					VGA,
					canvas->cursorX*canvas->zoom,
					canvas->cursorY*canvas->zoom,
					canvas->zoom,
					canvas->zoom,
					0
				);
			} else {
				DrawRect(
					VGA,
					canvas->cursorX*canvas->zoom,
					canvas->cursorY*canvas->zoom,
					canvas->zoom,
					canvas->zoom,
					canvas->cursorColor
				);
			}

			if(canvas->cursorPrevX!=canvas->cursorX || canvas->cursorPrevY!=canvas->cursorY) {

				FillRect(
					VGA,
					canvas->cursorX*canvas->zoom,
					canvas->cursorY*canvas->zoom,
					canvas->zoom,
					canvas->zoom,
					canvas->pixels[canvas->w*canvas->cursorY+canvas->cursorX]
				);


				DrawLine(
					VGA,
					tx*canvas->zoom,
					ty*canvas->zoom,
					tx*canvas->zoom+canvas->zoom-1,
					ty*canvas->zoom+canvas->zoom-1,
					canvas->cursorColor
				);

				DrawLine(
					VGA,
					tx*canvas->zoom+canvas->zoom-1,
					ty*canvas->zoom,
					tx*canvas->zoom,
					ty*canvas->zoom+canvas->zoom-1,
					canvas->cursorColor
				);

				if(canvas->gridShow) {
					DrawRect(
						VGA,
						canvas->cursorPrevX*canvas->zoom,
						canvas->cursorPrevY*canvas->zoom,
						canvas->zoom,
						canvas->zoom,
						canvas->gridColor
					);
				}
      }

			canvas->cursorPrevX=canvas->cursorX;
			canvas->cursorPrevY=canvas->cursorY;

			break;

		case GAMESTATE_PALETTE:

			if(keys[ 5]) { if(move==0) palette->cursorY--; }
			if(keys[ 8]) { if(move==0) palette->cursorX--; }
			if(keys[ 9]) { if(move==0) palette->cursorY++; }
			if(keys[10]) { if(move==0) palette->cursorX++; }


			if(palette->cursorX<0) palette->cursorX=7;
			if(palette->cursorY<0) palette->cursorY=1;
			if(palette->cursorX>7) palette->cursorX=0;
			if(palette->cursorY>1) palette->cursorY=0;


			if(blink<5000) {
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
      } else {
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
			}

			palette->cursorPrevX=palette->cursorX;
			palette->cursorPrevY=palette->cursorY;


			if(!hold) {
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
				if(
					!(
						keys[14]
					)
				) hold=false;
      }

			break;

		default: break;
		}


		blink++;
		move++;

		if(blink>=10000) blink=0;
		if(move>=10000) move=0;


/*
		gotoxy(1,1);
		for(i=0;i<16;i++) {
			if(keys[i]) putchar('1'); else putchar('0');
		}
*/

  }

		SetMode(0x03);

		gotoxy(1, 1);
    _setcursortype(_NORMALCURSOR);
}

int
main() {
    ctrlbrk(ctrlbrk_handler);
    hook_keyb_int();
    game();
    unhook_keyb_int();
    return 0;
}
