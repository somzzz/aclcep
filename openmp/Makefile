all: 
	g++ -fopenmp -o img_seg img_seg.c vision.c -lm

debug:
	g++ -fopenmp -g -o img_seg img_seg.c vision.c -lm

clean:
	rm img_seg binary.pgm connected.ppm
