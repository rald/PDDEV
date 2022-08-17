#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <dos.h>
#include <mem.h>

#include "stdbool.h"

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

#define CANVAS_IMPLEMENTATION
#include "canvas.h"

dword pal[]={
	0x001A1C2CL,
	0x005C275CL,
	0x00B13E53L,
	0x00EF7C57L,
	0x00FFCC75L,
	0x00A7F070L,
	0x0038B764L,
	0x00257179L,
	0x0029366FL,
	0x003B5CC9L,
	0x0041A676L,
	0x0073EFF7L,
	0x00F4F4F4L,
	0x0094B0C2L,
	0x00566C86L,
	0x00333C57L
};



int main() {

	int i,j,k;

	Canvas *canvas[6];

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

	canvas[0]=Canvas_LoadCVS("pen.cvs");
	canvas[1]=Canvas_LoadCVS("line.cvs");
	canvas[2]=Canvas_LoadCVS("rect.cvs");
	canvas[3]=Canvas_LoadCVS("frect.cvs");
	canvas[4]=Canvas_LoadCVS("oval.cvs");
	canvas[5]=Canvas_LoadCVS("foval.cvs");

	for(i=0;i<6;i++) {
		canvas[i]->zoom=1;
		canvas[i]->x=i*canvas[i]->zoom*(16+1);
		canvas[i]->y=0;
		Canvas_Draw(VGA,canvas[i]);
	}

	getchar();

	SetMode(0x03);

	return 0;
}