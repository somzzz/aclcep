// Written by Margrit Betie for Image and Video Computing, January 2006
// Compile with 
//   g++ -o hw1-YourLastName  main.c vision.c -lm
// Run with:
//   ./hw1-YourLastName input-image.ppm

#include <math.h>
#include "vision.h"
#include "helpers.c"
#include <queue> 
#include <omp.h>
// #define DEBUG_HISTOGRAM 1
// #define DEBUG_BINARY 1
// #define DEBUG_ALPHA 1
// #define DEBUG_IMAGE 1

std::queue<struct Task> task_queue;

int main(int argc, char *argv[]) {
	bool use_peakiness = true;
	int num_images = 1;
	if (argc != 2) {
		printf("Usage: ./hw3-Chang inputimage.ppm \n");
		exit(1);
	}

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
	        printf("Threshold: %d\n", threshold);

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
	int nr_threads = 6;
	/*Prepare for parallel the region*/
	//calculeaza latura patratului
	int area_square = colorimage.xdim * colorimage.ydim;
	int latura = trunc(sqrt(area_square / nr_threads));
	printf("Latura patratului este %d\n", latura);
	int works_threads[nr_threads];
	for (i = 0; i < nr_threads; i++)
	{
		works_threads[i] = false;
	} 

	struct Task process_task;
	struct Task* my_task = NULL;
	int id;
	int ok;
	omp_set_num_threads(nr_threads);

	for (i = 0; i < colorimage.ydim; i++) {
			for (j = 0; j < colorimage.xdim; j++) {
				if (binary.value[i][j] == UNTOUCHED) {
					process_task = generateTask(i, j, color, latura, colorimage.xdim, colorimage.ydim);
					task_queue.push(process_task);
					//tmparea = recursiveTouch(&binary, &colorimage, i, j, color, 0);
					#pragma omp parallel private(my_task, id) firstprivate(i, j) shared(ok, task_queue, works_threads)
					{
						while(!task_queue.empty() || searchWork(works_threads, nr_threads))
						{
							#pragma omp critical
							{
								if (!task_queue.empty())
								{
									ok = true;
									my_task = &task_queue.front();
									task_queue.pop();
									id = omp_get_thread_num();
									works_threads[id] = true;
								} else {
									my_task = NULL;
								}
							}
							tmparea = openmp_Touch(&binary, &colorimage, i, j, color, 0, my_task, &task_queue, omp_get_thread_num());
							if (tmparea != 0)
								printf("I'm thread %d nr with area %d\n", omp_get_thread_num(), tmparea);
							works_threads[id] = false;
						}
					}					
					color++;
				}
			}
	}

	strcpy(colorimage.name, "connected.ppm");
	OutputPpm(&colorimage);

	//binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);

	printf("\n");

	/* ----- */
	return 0;
}