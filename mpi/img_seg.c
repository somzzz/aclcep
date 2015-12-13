// Written by Margrit Betie for Image and Video Computing, January 2006
// Compile with 
//   g++ -o hw1-YourLastName  main.c vision.c -lm
// Run with:
//   ./hw1-YourLastName input-image.ppm

#include <math.h>
#include "vision.h"
#include "helpers.c"
#include <mpi.h>
// #define DEBUG_HISTOGRAM 1
// #define DEBUG_BINARY 1
// #define DEBUG_ALPHA 1
// #define DEBUG_IMAGE 1

int main(int argc, char *argv[]) {
	bool use_peakiness = true;
	int num_images = 1;

	if (argc != 2) {
		printf("Usage: ./img_seg inputimage.ppm \n");
		exit(1);
	}
	/*
	if (argc == 3 && strcmp(argv[2], "iterative") == 0) {
		use_peakiness = false;
	}
	*/

	// Loop iterators.
	int i, j, k, x, y, z; 
	int threshold = 0;

	// Grab image, place in memory.
	grayscaleimage image;
	grayscaleimage binary;
	grayscaleimage mainobject;
	char imagename[30];
	strcpy(image.name, argv[1]);
	strcpy(binary.name, argv[1]);
	strcpy(mainobject.name, argv[1]);
	ScanPgm(&image);
	ScanPgm(&binary);
	ScanPgm(&mainobject);

	/* ----- Peakiness Detection for Appropriate Threshold Selection ----- 
	 * -------------------------------------------------------------------
	 */

	if (use_peakiness && num_images == 1) {
		threshold = calculatePeakiness(&image, 0, image.xdim);
		// printf("Threshold: %d\n", threshold);

		// Create a binary output image based on the threshold created.
		for (i = 0; i < binary.ydim; i++) {
			for (j = 0; j < binary.xdim; j++) {
				if ((unsigned int) image.value[i][j] >= threshold) {
					binary.value[i][j] = WHITE;
				}
				else {
					binary.value[i][j] = BLACK;
				}
			}
		}

		normalizeBackground(&binary, 0, binary.xdim);

	} // PEAKINESS , num_images 1

	
	/* ----- Connected Component -----
	 * -------------------------------
	 */

	bool stillUntouched = true;

	// Initialize by negation.
	for (i = 0; i < binary.ydim; i++) {
		for (j = 0; j < binary.xdim; j++) {
			if (binary.value[i][j] == BLACK) {
				binary.value[i][j] = UNTOUCHED;
			}
		}
	}

	// Prepare to count objects.
	int object1 = 0;
	int object2 = 0;

	// Create a color copy.
	int color = 1;
	rgbimage colorimage;
	colorimage.xdim = image.xdim;
	colorimage.ydim = image.ydim;
	colorimage.highestvalue = WHITE;

	GetImagePpm(&colorimage);
	for (i = 0; i < colorimage.ydim; i++) {
		for (j = 0; j < colorimage.xdim; j++) {
			colorimage.r[i][j] = 255;
			colorimage.g[i][j] = 255;
			colorimage.b[i][j] = 255;
		}
	}

	// Find the first untouched pixel.
	unsigned long tmparea = 0;

	for (i = 0; i < colorimage.ydim; i++) {
		for (j = 0; j < colorimage.xdim; j++) {
			if (binary.value[i][j] == UNTOUCHED) {
				// Recurse around that pixel's neighbors.
				tmparea = recursiveTouch(&binary, &colorimage, i, j, color, 0);
				color++;
			}
		}
	}

	strcpy(colorimage.name, "connected.ppm");
	OutputPpm(&colorimage);

	//binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);

	/*if (num_images == 1) {
		int singlearea = findArea(&mainobject, &binary, 0, binary.xdim);
		calculateOrientation(&mainobject, 0, mainobject.xdim, singlearea);
		printf("Components detected: %d\n", object1);
	}*/

	/* ------ Region Merging ------
	 * ----------------------------
	 */


	printf("\n");

	/* ----- */
	return 0;
}
