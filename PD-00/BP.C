#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <math.h>

#include "stdbool.h"

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

#define PALETTE_IMPLEMENTATION
#include "palette.h"


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



unsigned char normal_keys[0x60];
unsigned char extended_keys[0x60];

unsigned char keys[16];

char k[] = "1234QWERASDFZXCV";
int  c[] = {
 2, 3, 4, 5,
16,17,18,19,
30,31,32,33,
44,45,46,47
};

char v[] = "123C456D789EA0BF";

int cx=0,cy=0;
int cw=16,ch=16;
int cz=8;

int x=0,y=0;
int px=0,py=0;
int tx=0,ty=0;

byte canvas[16*16];
bool grid=true;
bool hold=false;
bool band=false;

int blink=0;
int frame=0;
int color=0;
int move=0;



int k2c(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==k[i]) {
			return c[i];
		}
	}
	return -1;
}

int c2k(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==c[i]) {
			return k[i];
		}
	}
	return -1;
}

int k2v(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==k[i]) {
			return v[i];
		}
	}
	return -1;
}

int v2k(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==v[i]) {
			return k[i];
		}
	}
	return -1;
}

int c2v(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==c[i]) {
			return v[i];
		}
	}
	return -1;
}

int v2c(int x) {
	int i;
	for(i=0;i<16;i++) {
		if(x==v[i]) {
			return c[i];
		}
	}
	return -1;
}

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


void DrawCanvas(byte far *srf,int x,int y) {
	int i,j,k;
	for(j=0;j<ch;j++) {
		for(i=0;i<cw;i++) {
			k=cw*j+i;
			FillRect(srf,i*cz+cx+x,j*cz+cy+x,cz,cz,canvas[k]);
			if(grid) DrawRect(srf,i*cz+cx+x,j*cz+cy+x,cz,cz,15);
		}
	}
	DrawRect(srf,x,y,cw*cz,ch*cz,12);
}

void
game(void) {
	int i,j,k;
	int x=160,y=100;

	Palette *palette=NULL;

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

	palette=Palette_New(0,0,0);


	Palette_Draw(VGA,palette);

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