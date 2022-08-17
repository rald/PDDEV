#ifndef GRID_H
#define GRID_H

#include "stdbool.h"
#include "graphics.h"
#include "canvas.h"

typedef struct Grid Grid;

struct Grid {
	int x,y;
	int w,h;
	bool show;
	byte color;
	int cursorX,cursorY;
	int cursorPrevX,cursorPrevY;
	byte cursorColor;
	int zoom;
	Canvas *canvas;
};

Grid *Grid_New(
		Canvas *canvas,
		int x,int y,
		int w,int h,
		int zoom,
		byte color,
		byte cursorColor,
		bool show
);

void Grid_Free(Grid *grid);
void Grid_Draw(byte *srf,Grid *grid);

#ifdef CANVAS_IMPLEMENTATION

Grid *Grid_New(
		Canvas *canvas,
		int x,int y,
		int w,int h,
		int zoom,
		byte color,
		byte cursorColor,
		bool show
) {
	Grid *grid=(Grid*)malloc(sizeof(Grid));

	if(grid) {
		grid->canvas=canvas;
		grid->x=x;
		grid->y=y;
		grid->w=w;
		grid->h=h;
		grid->show=show;
		grid->zoom=zoom;
		grid->cursorX=0;
		grid->cursorY=0;
		grid->cursorPrevX=0;
		grid->cursorPrevY=0;
		grid->color=color;
		grid->cursorColor=cursorColor;
	}

	return grid;
}

void Grid_Free(Grid *grid) {
	Canvas_Free(grid->canvas);
	grid->canvas=NULL;
	free(grid);
}

void Grid_Draw(byte *srf,Grid *grid) {
	int i,j,k;
	int x,y;

	for(j=0;j<grid->h;j++) {
		for(i=0;i<grid->w;i++) {

			x=i*grid->zoom+grid->x;
			y=j*grid->zoom+grid->y;

			if(grid->canvas && i>=0 && i<grid->canvas->w && j>=0 && j<grid->canvas->h) {
				k=grid->w*j+i;
				FillRect(
					srf,
					x,y,
					grid->zoom,
					grid->zoom,
					grid->canvas->pixels[k]
				);
			}

			if(grid->show) {
				DrawRect(
					srf,
					x,y,
					grid->zoom,
					grid->zoom,
					grid->color
				);
			}

    }
	}

}



#endif /* GRID_IMPLEMENTATION */



#endif /* GRID_H */


