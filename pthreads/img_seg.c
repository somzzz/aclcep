
#include <math.h>
#include <pthread.h>
#include <queue>

#include "vision.h"
#include "helpers.c"
#include "errors.h"

#define NUM_THREADS 8


// #define DEBUG_HISTOGRAM 1
// #define DEBUG_BINARY 1
// #define DEBUG_ALPHA 1
// #define DEBUG_IMAGE 1

typedef struct {
	int block_x, block_y;
	int width, height;
	int entry_x, entry_y;
} Task;

// Global stuff for all threads

// Workpool
std::queue<Task> tasks;

static void *do_work(void *args) {
	return 0;
}

int main(int argc, char *argv[]) {

	// Default case we are treating.
	bool use_peakiness = true;
	int num_images = 1;

	if (argc != 2) {
		printf("Usage: ./img_seg inputimage.pgm \n");
		exit(1);
	}

	// Loop iterators.
	int i, j, k, x, y, z; 
	int threshold = 0;

	// Threads vars
	int status;
	long t;
    pthread_t threads[NUM_THREADS];
    pthread_attr_t attr;

	// Start threads
	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    for (t = 0; t < NUM_THREADS; t++) {
        status = pthread_create(&threads[t], &attr, do_work, (void *)t);
        if (status) err_abort(status, "create thread");
    }

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
	int remove1 = 0;
	int remove2 = 0;

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
	unsigned long maxarea = 0;
	int maxi, maxj;
	int maxx, maxy;

	if (num_images == 1) {
		for (i = 0; i < colorimage.ydim; i++) {
			for (j = 0; j < colorimage.xdim; j++) {
				if (binary.value[i][j] == UNTOUCHED) {
					// Recurse around that pixel's neighbors.
					tmparea = recursiveTouch(&binary, &colorimage, i, j, color, 0);
					//printf("Segment has area %d with color %d.\n", tmparea, color % 6); //flag

					object1++;

					if (tmparea < MINAREA && tmparea > 0) {
						recursiveTouch(&binary, &colorimage, i, j, 0, 0);
						removeSpecks(&binary, i, j);
						remove1++;
					}

					if (tmparea > maxarea) {
						maxarea = tmparea;
						maxi = i;
						maxj = j;
					}

					color++;
				}
			}
		}
	}

	strcpy(colorimage.name, "connected.ppm");
	OutputPpm(&colorimage);

	binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);


	/* ----- Orientation ----- 
	 * -----------------------
	 */

	printf("\n");

	// Only calculate orientation and circularity for largest blob.
	// Find largest blob and mark it specially.
	if (num_images == 1) {
		int singlearea = findArea(&mainobject, &binary, 0, binary.xdim);
		calculateOrientation(&mainobject, 0, mainobject.xdim, singlearea);
		printf("Components detected: %d\n", object1);
	}
	else {
		int area1 = findArea(&mainobject, &binary, 0, binary.xdim/2);
		binaryTouch(&mainobject, maxi, maxj);
		calculateOrientation(&mainobject, 0, binary.xdim/2, area1);
		// printf("Components detected: %d\n", object1 - remove1);

		printf("\n");

		int area2 = findArea(&mainobject, &binary, binary.xdim/2, binary.xdim);
		binaryTouch(&mainobject, maxx, maxy);
		calculateOrientation(&mainobject, binary.xdim/2, binary.xdim, area2);
		// printf("Components detected: %d\n", object2 - remove2);

		printf("\nTotal components detected: %d\n", object1 + object2);
	}

	/* Wait for all threads to complete */
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    printf ("Main(): Waited on %d threads. Done.\n", NUM_THREADS);

    /* Clean up and exit */
    pthread_attr_destroy(&attr);
    pthread_exit(NULL);

	return 0;
}
