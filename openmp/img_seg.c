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
	/*if (argc != 2) {
		printf("Usage: ./hw3-Chang inputimage.ppm \n");
		exit(1);
	}*/

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
	/*printf("--------------------------------------------\n");
	for (i = 0; i < binary.ydim; i++)
	{
	 	for (j = 0; j < binary.xdim; j++)
	 	{
	 		printf("%d ", binary.value[i][j]);
	 	}
	 	printf("\n");
	}*/
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
	struct Task* my_task = NULL;//(struct Task*)malloc(sizeof(struct Task));
	int id;
	int ok;
	//int x, y;
	omp_set_num_threads(nr_threads);

	for (i = 0; i < colorimage.ydim; i++) {
			for (j = 0; j < colorimage.xdim; j++) {
				if (binary.value[i][j] == UNTOUCHED) {
					//ok = false;
					process_task = generateTask(i, j, color, latura, colorimage.xdim, colorimage.ydim);
					//recursiveTouch(&binary, &colorimage, i, j, color, 0);
					//printf("Print all parameters:seed: %d %d i,j start: %d %d, i,j stop: %d %d, chunck: %d, color %d, dims: %d %d\n",
					// process_task.seed_i, process_task.seed_j, process_task.i_start, process_task.j_start, 
					// process_task.i_stop, process_task.j_stop, process_task.chunck, process_task.color,
					// colorimage.xdim, colorimage.ydim);
					task_queue.push(process_task);
					#pragma omp parallel private(my_task, id, x, y) firstprivate(i, j) shared(task_queue, works_threads, binary)
					{
						while(1)
						{
							#pragma omp critical
							{
								if (!task_queue.empty())
								{
									id = omp_get_thread_num();
									works_threads[id] = true;
									my_task = &task_queue.front();
									task_queue.pop();
									//printf("Thread %d get from queue %d %d\n", id, my_task->seed_i, my_task->seed_j);
								} else {
									id = omp_get_thread_num();
									works_threads[id] = false;
									my_task = NULL;
									//printf("Thread %d doesn't anything from queue \n", id);
								}
							}
							if (my_task != NULL) 
							{
								//local bfs in chunck + create tasks for others
								std::queue<struct Task> chunck_bfs;
								struct Task local;// = *my_task;
								memcpy(&local, my_task, sizeof(struct Task));
								chunck_bfs.push(local);
								//printf("I'm thread %d and I have seed %d %d %d %d\n", id, my_task->seed_i, my_task->seed_j,
								//local.seed_i, local.seed_j);
								while (!chunck_bfs.empty()) {
									struct Task local_task = chunck_bfs.front();
									chunck_bfs.pop();
									x = local_task.seed_i; 
									y = local_task.seed_j;
									if (is_in_limits_matrix(x, y, colorimage.ydim, colorimage.xdim) && !is_in_chunck(x, y, my_task) && binary.value[x][y] == UNTOUCHED)
									{
											//create new task for others threads
											struct Task new_task = generateTask(x, y, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
											task_queue.push(new_task);
											//printf("I'm thread %d and seed not in chunck %d %d\n", id, x, y);
									}
									if (is_in_chunck(x, y, my_task) && binary.value[x][y] == UNTOUCHED)
									{
											binary.value[x][y] = BLACK;
											set_color(&colorimage, x, y, my_task->color);
											//printf("Other hand thread %d with seed in chunck %d %d\n", id, x, y);
											if (is_in_limits_matrix(x, y - 1, colorimage.ydim, colorimage.xdim) && binary.value[x][y-1] == UNTOUCHED) {
												struct Task new_task = generateTask(x, y - 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);
											}
											if (is_in_limits_matrix(x, y + 1, colorimage.ydim, colorimage.xdim) && binary.value[x][y+1] == UNTOUCHED) {
												struct Task new_task = generateTask(x, y + 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
											if (is_in_limits_matrix(x - 1, y - 1, colorimage.ydim, colorimage.xdim) && binary.value[x-1][y-1] == UNTOUCHED) {
												struct Task new_task = generateTask(x - 1, y - 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);
											}
											if (is_in_limits_matrix(x - 1, y, colorimage.ydim, colorimage.xdim) && binary.value[x-1][y] == UNTOUCHED) {
												struct Task new_task = generateTask(x - 1, y, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
											if (is_in_limits_matrix(x - 1, y + 1, colorimage.ydim, colorimage.xdim) && binary.value[x-1][y+1] == UNTOUCHED) {
												struct Task new_task = generateTask(x - 1, y + 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
											if (is_in_limits_matrix(x + 1, y - 1, colorimage.ydim, colorimage.xdim) && binary.value[x+1][y-1] == UNTOUCHED) {
												struct Task new_task = generateTask(x + 1, y - 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
											if (is_in_limits_matrix(x + 1, y, colorimage.ydim, colorimage.xdim) && binary.value[x+1][y] == UNTOUCHED) {
												struct Task new_task = generateTask(x + 1, y, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
											if (is_in_limits_matrix(x + 1, y + 1, colorimage.ydim, colorimage.xdim) && binary.value[x+1][y+1] == UNTOUCHED) {
												struct Task new_task = generateTask(x + 1, y + 1, my_task->color, my_task->chunck, colorimage.xdim, colorimage.ydim);
												chunck_bfs.push(new_task);	
											}
									}
								}//wnd while bfs
								memset(my_task, 0, sizeof(struct Task));
							}//end if my_task*/
							//printf("Thread %d finish his job\n", id);
							works_threads[id] = false;	
							if (task_queue.empty() && !searchWork(works_threads, nr_threads))
								break;
						}//end while 1
					}//parallel region					
					color++;
				}//end if
			}
	}

	strcpy(colorimage.name, "connected.ppm");
	OutputPpm(&colorimage);

	//binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);

	printf("\n");

	return 0;
}