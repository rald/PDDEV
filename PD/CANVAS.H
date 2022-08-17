#ifndef CANVAS_H
#define CANVAS_H

#include "stdbool.h"
#include "graphics.h"

typedef struct Canvas Canvas;

struct Canvas {
	int x,y;
	int w,h;
	byte *pixels;
	int transparent;
	int zoom;
};



Canvas *Canvas_New(
	int x,int y,
	int w,int h,
	int zoom,
	int transparent
);

void Canvas_Free(Canvas *canvas);
void Canvas_Draw(byte *srf,Canvas *canvas);

void Canvas_DrawPoint(Canvas *canvas,int x,int y,int c);
void Canvas_DrawLine(Canvas *canvas,int x0,int y0,int x1,int y1,int c);
void Canvas_FillRect(Canvas *canvas,int x,int y,int w,int h,int c);
void Canvas_DrawRect(Canvas *canvas,int x,int y,int w,int h,int c);

void Canvas_DrawRow(Canvas *canvas,int x0,int x1,int y,int c);
void Canvas_DrawOval(Canvas *canvas,int x0,int y0,int x1,int y1,int c,bool f);


Canvas *Canvas_LoadCVS(char *filename);



#ifdef CANVAS_IMPLEMENTATION



Canvas *Canvas_New(
		int x,int y,
		int w,int h,
		int zoom,
		int transparent
) {
	int i;

	Canvas *canvas=(Canvas*)malloc(sizeof(Canvas));

	if(canvas) {
		canvas->x=x;
		canvas->y=y;
		canvas->w=w;
		canvas->h=h;
		canvas->zoom=zoom;
		canvas->transparent=transparent;

		canvas->pixels=(byte*)malloc(sizeof(byte)*(w*h));

		for(i=0;i<w*h;i++) {
			if(transparent!=-1) {
				canvas->pixels[i]=transparent;
			} else {
				canvas->pixels[i]=0;
			}
		}

	}

	return canvas;
}

void Canvas_Free(Canvas *canvas) {
	free(canvas->pixels);
	canvas->pixels=NULL;
	free(canvas);
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

void Canvas_FillRect(Canvas *canvas,int x,int y,int w,int h,int c) {
	int i,j;
	for(j=0;j<h;j++)
		for(i=0;i<w;i++)
			Canvas_DrawPoint(canvas,i+x,j+y,c);
}




void Canvas_DrawRow(Canvas *canvas,int x0,int x1,int y,int c) {
	int i;

	if(x0>x1) { int t=x0; x0=x1; x1=t; }

	for(i=x0;i<x1;i++) {
		Canvas_DrawPoint(canvas,i,y,c);
	}
}


void Canvas_DrawOval(Canvas *canvas,int x0,int y0,int x1,int y1,int c,bool f) {
		int xb, yb, xc, yc;
		int qa,qb;
		int dx,dy;
		int qx,qy;
		long qt;

    yb = yc = (y0 + y1) / 2;
		qb = (y0 < y1) ? (y1 - y0) : (y0 - y1);
		qy = qb;
		dy = qb / 2;
		if (qb % 2 != 0) yc++;

    xb = xc = (x0 + x1) / 2;
		qa = (x0 < x1) ? (x1 - x0) : (x0 - x1);
		qx = qa % 2;
		dx = 0;
		qt = (long)qa*qa + (long)qb*qb -2L*qa*qa*qb;
    if (qx != 0) {
			xc++;
			qt += 3L*qb*qb;
    }

	while (qy >= 0 && qx <= qa) {
		if (!f) {
			Canvas_DrawPoint(canvas,xb-dx,yb-dy,c);
			if (dx != 0 || xb != xc) {
				Canvas_DrawPoint(canvas,xc+dx,yb-dy,c);
				if (dy != 0 || yb != yc)
				Canvas_DrawPoint(canvas,xc+dx,yc+dy,c);
			}
			if (dy != 0 || yb != yc)
				Canvas_DrawPoint(canvas,xb-dx,yc+dy,c);
		}

		if (qt + 2L*qb*qb*qx + 3L*qb*qb <= 0L ||
			qt + 2L*qa*qa*qy - (long)qa*qa <= 0L) {
			qt += 8L*qb*qb + 4L*qb*qb*qx;
			dx++;
			qx += 2;
		} else if (qt - 2L*qa*qa*qy + 3L*qa*qa > 0L) {
				if (f) {
					Canvas_DrawRow(canvas,xb-dx,xc+dx,yc+dy,c);
					if (dy != 0 || yb != yc)
						Canvas_DrawRow(canvas,xb-dx,xc+dx,yb-dy,c);
				}
				qt += 8L*qa*qa - 4L*qa*qa*qy;
				dy--;
				qy -= 2;
		} else {
			if (f) {
				Canvas_DrawRow(canvas,xb-dx,xc+dx,yc+dy,c);
					if (dy != 0 || yb != yc)
						Canvas_DrawRow(canvas,xb-dx,xc+dx,yb-dy,c);
			}
			qt += 8L*qb*qb + 4L*qb*qb*qx + 8L*qa*qa - 4L*qa*qa*qy;
			dx++;
			qx += 2;
			dy--;
			qy -= 2;
    }
	}
}




Canvas *Canvas_LoadCVS(char *filename) {
	Canvas *canvas=(Canvas*)malloc(sizeof(Canvas));

	char *hex="0123456789ABCDEF";

	int i=0,j=0,k=0,l=0;
	int x=0,y=0;
	int c;

	FILE *fin=NULL;

	if(canvas) {

		canvas->x=0;
		canvas->y=0;
		canvas->zoom=1;

		if((fin=fopen(filename,"rt"))==NULL) {
			perror(filename);
			exit(1);
		}

		if(fscanf(fin,"%d,%d,%d\n",&canvas->w,&canvas->h,&canvas->transparent)!=3) {
			printf("Error:%s: header at line %d column %d\n",filename,l+1,k+1);
			exit(1);
		}

		canvas->pixels=(byte*)calloc(canvas->w*canvas->h,sizeof(byte));

		l++;

		while((c=fgetc(fin))!=EOF) {
			if(c=='\n') {
				k=0;
				l++;
			} else {
				j=-1;
				for(i=0;i<16;i++) {
					if(c==hex[i]) {
						j=i;
						break;
					}
				}

				if(j!=-1) {
					canvas->pixels[canvas->w*y+x]=j;

					x++;
					if(x==16) { x=0; y++; if(y==canvas->h) break; }

					k++;
				} else {
					printf("Error:%s: invalld character at line %d column %d\n",filename,l,k);
					exit(1);
				}
			}
		}

		fclose(fin);
	}

	return canvas;
}


#endif /* CANVAS_IMPLEMENTATION */



#endif /* CANVAS_H */


