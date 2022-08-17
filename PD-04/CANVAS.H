#ifndef CANVAS_H
#define CANVAS_H

typedef struct Canvas Canvas;

struct Canvas {
	int x,y;
	int w,h;
	byte *pixels;
	byte color;
	bool gridShow;
	byte gridColor;
	int cursorX,cursorY;
	int cursorPrevX,cursorPrevY;
	byte cursorColor;
	int zoom;
};

Canvas *Canvas_New(int x,int y,int w,int h,int zoom,bool gridShow,byte gridColor,byte cursorColor);
void Canvas_Free(Canvas **canvas);
void Canvas_Draw(byte *srf,Canvas *canvas);

#ifdef CANVAS_IMPLEMENTATION

Canvas *Canvas_New(
		int x,int y,
		int w,int h,
		int zoom,
    bool gridShow,
		byte gridColor,
		byte cursorColor
) {
	int i;
	Canvas *canvas=(Canvas*)malloc(sizeof(Canvas));
	if(canvas) {
		canvas->x=x;
		canvas->y=y;
		canvas->w=w;
		canvas->h=h;
		canvas->gridShow=gridShow;
		canvas->zoom=zoom;
		canvas->cursorX=0;
		canvas->cursorY=0;
		canvas->cursorPrevX=0;
		canvas->cursorPrevY=0;
		canvas->gridColor=gridColor;
		canvas->cursorColor=cursorColor;

		canvas->pixels=(byte*)malloc(sizeof(byte)*(w*h));

		for(i=0;i<w*h;i++) {
			canvas->pixels[i]=0;
		}
	}
	return canvas;
}

void Canvas_Free(Canvas **canvas) {
	free(((Canvas*)canvas)->pixels);
	((Canvas*)canvas)->pixels=NULL;
	free(*canvas);
	(*canvas)=NULL;
}

void Canvas_Draw(byte *srf,Canvas *canvas) {
	int i,j,k;
	int x,y;
	for(j=0;j<canvas->h;j++) {
		for(i=0;i<canvas->w;i++) {
			k=canvas->w*j+i;
			x=i*canvas->zoom+canvas->x;
			y=j*canvas->zoom+canvas->y;
			FillRect(srf,x,y,canvas->zoom,canvas->zoom,canvas->pixels[k]);
			if(canvas->gridShow) {
				DrawRect(srf,x,y,canvas->zoom,canvas->zoom,canvas->gridColor);
			}
		}
	}
}

void Canvas_DrawPoint(Canvas *canvas,int x,int y,int c) {
	if(x>=0 && x<canvas->w && y>=0 && y<canvas->h) {
		canvas->pixels[canvas->w*y+x]=c;
	}
}

void Canvas_DrawLine(Canvas *canvas,int x0,int y0,int x1,int y1,int c) {
	int dx=abs(x1-x0),sx=x0<x1?1:-1;
	int dy=abs(y1-y0),sy=y0<y1?1:-1;
	int e1=(dx>dy?dx:-dy)/2,e2;
	for(;;) {
		Canvas_DrawPoint(canvas,x0,y0,c);
		if(x0==x1 && y0==y1) break;
		e2=e1;
		if(e2>-dx) { e1-=dy; x0+=sx; }
		if(e2<dy)  { e1+=dx; y0+=sy; }
	}
}

void Canvas_FillRect(Canvas *canvas,int x,int y,int w,int h,int c) {
	int i,j;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
			Canvas_DrawPoint(canvas,i+x,j+y,c);
}

void Canvas_DrawRect(Canvas *canvas,int x,int y,int w,int h,int c) {
	int i,j;
	for(i=0;i<w;i++) {
		Canvas_DrawPoint(canvas,i+x,y,c);
		Canvas_DrawPoint(canvas,i+x,y+h-1,c);
	}
	for(j=0;j<h;j++) {
		Canvas_DrawPoint(canvas,x,y+j,c);
		Canvas_DrawPoint(canvas,x+w-1,y+j,c);
	}
}


#endif /* CANVAS_IMPLEMENTATION */



#endif /* CANVAS_H */


