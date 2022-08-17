	int i;
	for(i=0;i<16;i++) {
		FillRect(srf,i%8*16+x,i/8*16+y,16,16,i);
	}
	DrawRect(srf,color%8*16+1+x,color/8*16+1+y,16-2,16-2,0);
	DrawRect(srf,color%8*16+x,color/8*16+y,16,16,12);
