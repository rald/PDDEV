#ifndef PALLETE_H
#define PALETTE_H

typedef struct Palette Palette;

struct Palette {
	int x,y;
	int cursorX,cursorY;
	int cursorPrevX,cursorPrevY;
	byte cursorColor;
};

Palette *Palette_New(int x,int y,byte cursorColor);
void Palette_Draw(byte *srf,Palette *palette);


#ifdef PALETTE_IMPLEMENTATION

Palette *Palette_New(int x,int y,byte cursorColor) {
	Palette *palette=(Palette*)malloc(sizeof(Palette));
	if(palette) {
		palette->x=x;
		palette->y=y;
		palette->cursorX=0;
		palette->cursorY=0;
		palette->cursorPrevX=0;
		palette->cursorPrevY=0;
		palette->cursorColor=cursorColor;
	}
	return palette;
}

void Palette_Free(Palette **palette) {
	free(*palette);
	(*palette)=NULL;
}

void Palette_Draw(byte *srf,Palette *palette) {
	int i;
	for(i=0;i<16;i++) {
		FillRect(srf,i%8*16+palette->x,i/8*16+palette->y,16,16,i);
	}
}

#endif /* PALETTE_IMPLEMENTATION */

#endif /* PALETTE_H */

