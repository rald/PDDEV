#ifndef GRAPHICS_H
#define GRAPHICS_H

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long dword;

void SetMode(unsigned char mode);
void SetPalette(byte idx,byte r,byte g,byte b);
void DrawPoint(byte far *srf,int x,int y,int c);
void DrawLine(byte far *srf,int x0,int y0,int x1,int y1,int c);
void FillRect(byte far *srf,int x,int y,int w,int h,int c);
void DrawRect(byte far *srf,int x,int y,int w,int h,int c);

byte far *VGA=(byte far *)0xA0000000L;


#ifdef GRAPHICS_IMPLEMENTATION


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


void DrawPoint(byte far *srf,int x,int y,int c) {
	if(x>=0 && x<320 && y>=0 && y<200) {
		srf[320*y+x]=c;
	}
}

void DrawLine(byte far *srf,int x0,int y0,int x1,int y1,int c) {
	int dx=abs(x1-x0),sx=x0<x1?1:-1;
	int dy=abs(y1-y0),sy=y0<y1?1:-1;
	int e1=(dx>dy?dx:-dy)/2,e2;
	for(;;) {
		DrawPoint(srf,x0,y0,c);
		if(x0==x1 && y0==y1) break;
		e2=e1;
		if(e2>-dx) { e1-=dy; x0+=sx; }
		if(e2<dy)  { e1+=dx; y0+=sy; }
	}
}

void FillRect(byte far *srf,int x,int y,int w,int h,int c) {
	int i,j;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
			DrawPoint(srf,i+x,j+y,c);
}

void DrawRect(byte far *srf,int x,int y,int w,int h,int c) {
	int i,j;
	for(i=0;i<w;i++) {
		DrawPoint(srf,i+x,y,c);
		DrawPoint(srf,i+x,y+h-1,c);
	}
	for(j=0;j<h;j++) {
		DrawPoint(srf,x,y+j,c);
		DrawPoint(srf,x+w-1,y+j,c);
	}
}

#endif /* GRAPHICS_IMPLEMENTATION */

#endif /* GRAPHICS_H */

