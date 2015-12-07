all: 
	g++ -o img_seg img_seg.c vision.c -pthread -lm

debug:
	g++ -o img_seg img_seg.c vision.c -pthread -lm -g -Wall

clean:
	rm img_seg binary.pgm connected.ppm
