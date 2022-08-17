#ifndef PALLETE_H
#define PALETTE_H

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

typedef struct Palette Palette;

struct Palette {
	int x,y;
	int color;
};

Palette *Palette_New(int x,int y,int color);
void Palette_Draw(byte far *srf,Palette *palette);


#ifdef PALETTE_IMPLEMENTATION

Palette *Palette_New(int x,int y,int color) {
	Palette *palette=(Palette*)malloc(sizeof(Palette));
	if(palette) {
		palette->x=x;
		palette->y=y;
		palette->color=color;
	}
	return palette;
}

void Palette_Draw(byte far *srf,Palette *palette) {
	int i;
	for(i=0;i<16;i++) {
		FillRect(srf,i%8*16+palette->x,i/8*16+palette->y,16,16,i);
	}
	DrawRect(srf,palette->color%8*16+1+palette->x,palette->color/8*16+1+palette->y,16-2,16-2,0);
	DrawRect(srf,palette->color%8*16+palette->x,palette->color/8*16+palette->y,16,16,12);
}

#endif /* PALETTE_IMPLEMENTATION */

#endif /* PALETTE_H */

