
#include <math.h>
#include <pthread.h>
#include <queue>
#include <iostream>

#include "vision.h"
#include "helpers.c"

#define BILLION  	1000000000L;


// #define DEBUG_HISTOGRAM 1
// #define DEBUG_BINARY 1
// #define DEBUG_ALPHA 1
// #define DEBUG_IMAGE 1

typedef struct {
	int block_x, block_y;
	int entry_x, entry_y;
	int color;
} Task;

grayscaleimage image;
grayscaleimage binary;
rgbimage colorimage;

std::queue<Task> tasks;

static bool is_in_matrix(int i, int j) {
	return (0 <= i && i < binary.ydim)
		&& (0 <= j && j < binary.xdim);
}

static void put_color(int i, int j, int color) {

	// Fun colors!
	if (color % 7 == 0) {
		colorimage.r[i][j] = 255;
		colorimage.g[i][j] = 255;
		colorimage.b[i][j] = 255;
	}
	else if (color % 7 == 1) {
		colorimage.r[i][j] = 0;
		colorimage.g[i][j] = 255;
		colorimage.b[i][j] = 0;
	}
	else if (color % 7 == 2) {
		colorimage.r[i][j] = 0;
		colorimage.g[i][j] = 0;
		colorimage.b[i][j] = 255;
	}
	else if (color % 7 == 3) {
		colorimage.r[i][j] = 255;
		colorimage.g[i][j] = 255;
		colorimage.b[i][j] = 0;
	}
	else if (color % 7 == 4) {
		colorimage.r[i][j] = 255;
		colorimage.g[i][j] = 0;
		colorimage.b[i][j] = 255;
	}
	else if (color % 7 == 5) {
		colorimage.r[i][j] = 0;
		colorimage.g[i][j] = 255;
		colorimage.b[i][j] = 255;
	}
	else {
		colorimage.r[i][j] = 255;
		colorimage.g[i][j] = 0;
		colorimage.b[i][j] = 0;
	}
}

int main(int argc, char *argv[]) {
	bool use_peakiness = true;
	int num_images = 1;

	if (argc != 2) {
		printf("Usage: ./hw3-Chang inputimage.ppm \n");
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

	struct timespec start, stop;
	double accum;

	srand(time(NULL));

	clock_gettime(CLOCK_REALTIME, &start);

	for (i = 0; i < colorimage.ydim; i++) {
		for (j = 0; j < colorimage.xdim; j++) {

			if (binary.value[i][j] == UNTOUCHED) {
				Task tsk;
				tsk.entry_y = i; tsk.entry_x = j;
				tsk.color = color;

				tasks.push(tsk);

				while (!tasks.empty()) {

						Task t = tasks.front();
						tasks.pop();

						int i = t.entry_y;
						int j = t.entry_x;

						// Found an untouched value in the current chunk => process it
						if ((binary.value[i][j] == UNTOUCHED || binary.value[i][j] == TOUCH_IN_PROGRESS)) {
					
							// Visit current cell
							binary.value[i][j] = BLACK;
							put_color(i, j, tsk.color);

							//std::cout << "thread " << my_id << " visiting cell i = " << i << " j = " << j << std::endl;

							// Add neighbouring cells of interest to the visit queue
							if (is_in_matrix(i, j - 1) && binary.value[i][j - 1] == UNTOUCHED) {
								t.entry_y = i; t.entry_x = j - 1;
								binary.value[i][j - 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i, j + 1) && binary.value[i][j + 1] == UNTOUCHED) {
								t.entry_y = i; t.entry_x = j + 1;
								binary.value[i][j + 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i - 1, j - 1) && binary.value[i - 1][j - 1] == UNTOUCHED) {
								t.entry_y = i - 1; t.entry_x = j - 1;
								binary.value[i - 1][j - 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i - 1, j) && binary.value[i - 1][j] == UNTOUCHED) {
								t.entry_y = i - 1; t.entry_x = j;
								binary.value[i - 1][j] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i - 1, j + 1) && binary.value[i - 1][j + 1] == UNTOUCHED) {
								t.entry_y = i - 1; t.entry_x = j + 1;
								binary.value[i - 1][j + 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i + 1, j - 1) && binary.value[i + 1][j - 1] == UNTOUCHED) {
								t.entry_y = i + 1; t.entry_x = j - 1;
								binary.value[i + 1][j - 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i + 1, j) && binary.value[i + 1][j] == UNTOUCHED) {
								t.entry_y = i + 1; t.entry_x = j;
								binary.value[i + 1][j] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
							if (is_in_matrix(i + 1, j + 1) && binary.value[i + 1][j + 1] == UNTOUCHED) {
								t.entry_y = i + 1; t.entry_x = j + 1;
								binary.value[i + 1][j + 1] = TOUCH_IN_PROGRESS;
								tasks.push(t);
							}
						} // end if

				} // endwhile object bfs
				color++;
			}
		}
	}


	clock_gettime(CLOCK_REALTIME, &stop);

	// Print time 
	accum = ( stop.tv_sec - start.tv_sec )
		+ (double)( stop.tv_nsec - start.tv_nsec )
			/ (double)BILLION;
	printf("[SERIAL] Image segmentation: %lf\n", accum);

	strcpy(colorimage.name, "connected.ppm");
	OutputPpm(&colorimage);

	binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);


	/* ----- */
	return 0;
}
