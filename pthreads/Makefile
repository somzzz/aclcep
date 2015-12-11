all: 
	g++ -o img_seg img_seg.c vision.c -pthread -lm

debug:
	g++ -o img_seg img_seg.c vision.c -pthread -lm -g -Wall

original:
	g++ -o orig original_trimmed.c vision.c -pthread -lm
clean:
	rm img_seg binary.pgm connected.ppm
