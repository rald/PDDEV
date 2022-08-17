#ifndef TOOL_H
#define TOOL_H

#include "canvas.h"

typedef struct Tool Tool;

struct Tool {
	int x,y;
	int cursorX,cursorY;
	int cursorPrevX,cursorPrevY;
	byte cursorColor;
	Canvas **canvases;
	int ncanvases;
};



Tool *Tool_New(int x,int y,byte cursorColor);
void Tool_Free(Tool *tool);
void Tool_Draw(byte *srf,Tool *tool);



#ifdef TOOL_IMPLEMENTATION



Tool *Tool_New(int x,int y,byte cursorColor) {
	Tool *tool=(Tool*)malloc(sizeof(Tool));

	char *files[]={
		"pen.cvs",
		"oval.cvs",
		"foval.cvs",
		"ffill.cvs",
		"line.cvs",
		"rect.cvs",
		"frect.cvs",
		"clear.cvs"
	};

	int nfiles=8;

	int i;

	if(tool) {
		tool->x=x;
		tool->y=y;
		tool->cursorX=0;
		tool->cursorY=0;
		tool->cursorPrevX=0;
		tool->cursorPrevY=0;
		tool->cursorColor=cursorColor;

		tool->ncanvases=nfiles;
		tool->canvases=(Canvas**)malloc(sizeof(Canvas*)*nfiles);

		for(i=0;i<nfiles;i++) {
			tool->canvases[i]=(Canvas*)Canvas_LoadCVS(files[i]);

			if(tool->canvases[i]==NULL) {
				printf("Error: Tool_New: Canvas_LoadCVS: %s\n",files[i]);
				exit(1);
			}
		}
	}

	return tool;
}

void Tool_Free(Tool *tool) {
	int i;
	for(i=0;i<tool->ncanvases;i++) {
		Canvas_Free(tool->canvases[i]);
		tool->canvases[i]=NULL;
	}
	tool->ncanvases=0;
	free(tool);
}

void Tool_Draw(byte *srf,Tool *tool) {
	int i;
	for(i=0;i<tool->ncanvases;i++) {
		tool->canvases[i]->x=(i%4)*16+tool->x;
		tool->canvases[i]->y=(i/4)*16+tool->y;
		Canvas_Draw(srf,tool->canvases[i]);
	}
}


#endif /* TOOL_IMPLEMENTATION */



#endif /* TOOL_H */


