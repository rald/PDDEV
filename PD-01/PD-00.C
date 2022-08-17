#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <math.h>

#define false (0)
#define true (!false)
typedef int bool;

#define _NOCURSOR     0x3200
#define _NORMALCURSOR 0x0607
#define _SOLIDCURSOR  0x0007

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;


byte far *VGA=(byte far *)0xA0000000L;

dword pal[]={
	0x001A1C2C,
	0x005D275D,
	0x00B13E53,
	0x00EF7D57,
	0x00FFCD75,
	0x00A7F070,
	0x0038B764,
	0x00257179,
	0x0029366F,
	0x003B5DC9,
	0x0041A676,
	0x0073EFF7,
	0x00F4F4F4,
	0x0094B0C2,
	0x00566C86,
	0x00333C57
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

void SetMode(unsigned char mode) {
	union REGS regs;
	regs.h.ah=0x00;
	regs.h.al=mode;
	int86(0x10,&regs,&regs);
}


void SetPalette(byte idx,byte r,byte g,byte b) {
	outp(0x03c8,idx);
	outp(0x03c9,r);
	outp(0x03c9,g);
	outp(0x03c9,b);
}


void DrawPoint(int x,int y,int c) {
	if(x>=0 && x<320 && y>=0 && y<200) {
		VGA[320*y+x]=c;
	}
}

void DrawLine(int x0,int y0,int x1,int y1,int c) {
	int dx=abs(x1-x0),sx=x0<x1?1:-1;
	int dy=abs(y1-y0),sy=y0<y1?1:-1;
	int e1=(dx>dy?dx:-dy)/2,e2;
	for(;;) {
		DrawPoint(x0,y0,c);
		if(x0==x1 && y0==y1) break;
		e2=e1;
		if(e2>-dx) { e1-=dy; x0+=sx; }
		if(e2<dy)  { e1+=dx; y0+=sy; }
	}
}

void FillRect(int x,int y,int w,int h,int c) {
	int i,j;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
			DrawPoint(i+x,j+y,c);
}

void DrawRect(int x,int y,int w,int h,int c) {
	int i,j;
	for(i=0;i<w;i++) {
		DrawPoint(i+x,y,c);
		DrawPoint(i+x,y+h-1,c);
	}
	for(j=0;j<h;j++) {
		DrawPoint(x,y+j,c);
		DrawPoint(x+w-1,y+j,c);
	}
}

void DrawCanvas() {
	int i,j,k;
	for(j=0;j<ch;j++) {
		for(i=0;i<cw;i++) {
			k=cw*j+i;
			FillRect(i*cz+cx,j*cz+cy,cz,cz,canvas[k]);
			if(grid) DrawRect(i*cz+cx,j*cz+cy,cz,cz,15);
		}
	}
	DrawRect(0,0,cw*cz,ch*cz,12);
	DrawLine(tx*cz,ty*cz,tx*cz+cz-1,ty*cz+cz-1,12);
	DrawLine(tx*cz+cz-1,ty*cz,tx*cz,ty*cz+cz-1,12);
}

void DrawPalette(int x,int y) {
	int i;
	for(i=0;i<16;i++) {
		FillRect(i%8*16+x,i/8*16+y,16,16,i);
	}
	DrawRect(color%8*16+1+x,color/8*16+1+y,16-2,16-2,0);
	DrawRect(color%8*16+x,color/8*16+y,16,16,12);
}

void line(int x0,int y0,int x1,int y1,int c,bool b) {
	int dx=abs(x1-x0),sx=x0<x1?1:-1;
	int dy=abs(y1-y0),sy=y0<y1?1:-1;
	int e1=(dx>dy?dx:-dy)/2,e2;
	for(;;) {
		if(b) canvas[x0+y0*cw]=c;
		FillRect(x0*cz,y0*cz,cz,cz,c);
		if(x0==x1 && y0==y1) break;
		e2=e1;
		if(e2>-dx) { e1-=dy; x0+=sx; }
		if(e2<dy)  { e1+=dx; y0+=sy; }
	}

}


void
game(void) {
	int i,j,k;
	int x=160,y=100;

	_setcursortype(_NOCURSOR);
	clrscr();

	SetMode(0x13);

	for(i=0;i<16;i++) {
		int r=(pal[i] & 0x00FF0000) >> 16;
		int g=(pal[i] & 0x0000FF00) >> 8;
		int b=(pal[i] & 0x0000000FF);
		r=(int)(r/255.0*63.0);
		g=(int)(g/255.0*63.0);
		b=(int)(b/255.0*63.0);

		SetPalette(i,r,g,b);
	}

	for(j=0;j<ch;j++) {
		for(i=0;i<cw;i++) {
			k=cw*j+i;
			canvas[k]=0;
		}
	}


	DrawCanvas();

	DrawPalette(320-16*8,0);

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

		if(!hold)  {
			if(keys[ 1]) { color--; if(color<0) color=15; DrawPalette(320-16*8,0); hold=true; }
			if(keys[ 2]) { color++; if(color>15) color=0; DrawPalette(320-16*8,0); hold=true; }

			if(keys[ 0]) {
				grid=!grid; DrawCanvas(); hold=true;

				FillRect(x*cz,y*cz,cz,cz,(int)fmod((double)blink/1000,2)?0:color);
				DrawRect(x*cz,y*cz,cz,cz,(int)fmod((double)blink/1000,2)?0:15);
				px=x; py=y;

			}


			if(keys[ 5]) { if(move>250) { y--; move=0; } }
			if(keys[ 8]) { if(move>250) { x--; move=0; } }
			if(keys[ 9]) { if(move>250) { y++; move=0; } }
			if(keys[10]) { if(move>250) { x++; move=0; } }

			if(x<0) x=cw-1;
			if(x>=cw) x=0;
			if(y<0) y=ch-1;
			if(y>=ch) y=0;

			if(keys[ 3]) {
				FillRect(tx*cz,ty*cz,cz,cz,canvas[tx+ty*cw]);
				DrawRect(tx*cz,ty*cz,cz,cz,15);
				DrawRect(0,0,cw*cz,ch*cz,12);
        tx=x;
				ty=y;
				DrawLine(tx*cz,ty*cz,tx*cz+cz-1,ty*cz+cz-1,12);
				DrawLine(tx*cz+cz-1,ty*cz,tx*cz,ty*cz+cz-1,12);
			}

			if(keys[ 7]) {
				line(tx,ty,x,y,color,true);
				hold=true;
			}



		} else {
			if(!(
						keys[ 7] ||
						keys[ 0] ||
						keys[ 1] ||
						keys[ 2])) hold=false;

						if(keys[7]) {

						}

		}


		if(keys[ 6]) {
			if(canvas[x+y*cw]!=color) {
				canvas[x+y*cw]=color;
				FillRect(x*cz,y*cz,cz,cz,color);
			}
		}

		if(px!=x || py!=y) {
			FillRect(px*cz,py*cz,cz,cz,canvas[px+py*cw]);
			if(grid) DrawRect(px*cz,py*cz,cz,cz,15);
			DrawRect(0,0,cw*cz,ch*cz,12);
      blink=0;
		}

		if(blink<500) {
			DrawRect(x*cz,y*cz,cz,cz,12);
			DrawRect(x*cz+1,y*cz+1,cz-2,cz-2,0);
			DrawLine(tx*cz,ty*cz,tx*cz+cz-1,ty*cz+cz-1,12);
			DrawLine(tx*cz+cz-1,ty*cz,tx*cz,ty*cz+cz-1,12);
		} else {
			FillRect(x*cz,y*cz,cz,cz,color);
    }



		px=x; py=y;

/*
		gotoxy(1,1);
		for(i=0;i<16;i++) {
			if(keys[i]) putchar('1'); else putchar('0');
		}
*/

		move++;		if(move>1000) move=0;
		blink++;  if(blink>1000) blink=0;
		frame++;


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