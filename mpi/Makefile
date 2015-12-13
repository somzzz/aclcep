all: 
	mpiCC -o img_seg img_seg.c vision.c -lm -w

debug:
	mpiCC -g -o img_seg img_seg.c vision.c -lm

clean:
	rm img_seg binary.pgm connected.ppm
