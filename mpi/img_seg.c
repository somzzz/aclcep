
#include <cstdlib>
#include <math.h>
#include <queue>
#include <iostream>

#include "vision.h"
#include "helpers.c"
#include <mpi.h>

#define BILLION  	1000000000L;
#define MASTER 		0
#define FIRST_COLOR 1


typedef struct {
	int block_x, block_y;
	int entry_x, entry_y;
	int color;
} Task;

grayscaleimage image;
grayscaleimage binary;
grayscaleimage mainobject;

rgbimage color_image;
int color;
int top_line, bottom_line;

int recursiveTouchMPI(grayscaleimage *binary, rgbimage *output, 
		int i, int j, int color, unsigned long area, int rank) 
{
	binary->value[i * binary->xdim + j] = BLACK;
	area++;

	// Fun colors!
	if (color % 7 == 0) {
		output->r[i * binary->xdim + j] = 255;
		output->g[i * binary->xdim + j] = 255;
		output->b[i * binary->xdim + j] = 255;
	}
	else if (color % 7 == 1) {
		output->r[i * binary->xdim + j] = 0;
		output->g[i * binary->xdim + j] = 255;
		output->b[i * binary->xdim + j] = 0;
	}
	else if (color % 7 == 2) {
		output->r[i * binary->xdim + j] = 0;
		output->g[i * binary->xdim + j] = 0;
		output->b[i * binary->xdim + j] = 255;
	}
	else if (color % 7 == 3) {
		output->r[i * binary->xdim + j] = 255;
		output->g[i * binary->xdim + j] = 255;
		output->b[i * binary->xdim + j] = 0;
	}
	else if (color % 7 == 4) {
		output->r[i * binary->xdim + j] = 255;
		output->g[i * binary->xdim + j] = 0;
		output->b[i * binary->xdim + j] = 255;
	}
	else if (color % 7 == 5) {
		output->r[i * binary->xdim + j] = 0;
		output->g[i * binary->xdim + j] = 255;
		output->b[i * binary->xdim + j] = 255;
	}
	else {
		output->r[i * binary->xdim + j] = 255;
		output->g[i * binary->xdim + j] = 0;
		output->b[i * binary->xdim + j] = 0;
	}

	if (j - 1 >= 0 && binary->value[i * binary->xdim + j - 1] == UNTOUCHED) {
		area = recursiveTouch(binary, output, i, j-1, color, area);
	}
	if (j + 1 < binary->xdim && binary->value[i * binary->xdim + j + 1] == UNTOUCHED) {
		area = recursiveTouch(binary, output, i, j+1, color, area);
	}

	if (i - 1 > top_line) {
		if (j - 1 >= 0 && binary->value[(i-1) * binary->xdim + j-1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j-1, color, area);
		}
		if (binary->value[(i-1) * binary->xdim + j] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j, color, area);
		}
		if (j + 1 < binary->xdim && binary->value[(i-1) * binary->xdim + j + 1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j+1, color, area);
		}
	}

	if (i + 1 < bottom_line) {
		if (j - 1 >= 0 && binary->value[(i+1) * binary->xdim + j - 1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j-1, color, area);
		}
		if (binary->value[(i+1) * binary->xdim + j] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j, color, area);
		}
		if (j + 1 < binary->xdim && binary->value[(i+1) * binary->xdim + j + 1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j+1, color, area);
		}
	}

	return area;
}

int main(int argc, char *argv[]) {

	// Default case we are treating.
	bool use_peakiness = true;
	int num_images = 1;
	MPI_Status status;

	if (argc != 2) {
		printf("Usage: ./img_seg inputimage.pgm\n");
		exit(1);
	}

	// Loop iterators.
	int i, j, k, x, y, z; 
	int threshold = 0;
	int rank, numprocs;
	int num_lines_proc;
	int tag = 0;
	int trunc_number_lines;
	struct timespec start, stop;
	double accum;

	// Grab image, place in memory.
	char imagename[30];
	strcpy(image.name, argv[1]);
	strcpy(binary.name, argv[1]);
	strcpy(mainobject.name, argv[1]);
	ScanPgm(&image);
	ScanPgm(&binary);
	
	//ScanPgm(&mainobject);

	/* ----- Peakiness Detection for Appropriate Threshold Selection ----- 
	 * -------------------------------------------------------------------
	 */

	if (use_peakiness && num_images == 1) {
		threshold = calculatePeakiness(&image, 0, image.xdim);

		// Create a binary output image based on the threshold created.
		for (i = 0; i < binary.ydim; i++) {
			for (j = 0; j < binary.xdim; j++) {
				if ((unsigned int) image.value[i * image.xdim + j] >= threshold) {
					binary.value[i * image.xdim + j] = WHITE;
				}
				else {
					binary.value[i * image.xdim + j] = BLACK;
				}
			}
		}

		normalizeBackground(&binary, 0, binary.xdim);

	} // PEAKINESS , num_images 1



	/* ----- Connected Component -----
	 * -------------------------------
	 */
	
	// Initialize by negation.
	for (i = 0; i < binary.ydim; i++) {
		for (j = 0; j < binary.xdim; j++) {
			if (binary.value[i * image.xdim + j] == BLACK) {
				binary.value[i * image.xdim + j] = UNTOUCHED;
			}
		}
	}

	// 	binary.highestvalue = WHITE;
	// strcpy(binary.name, "interm_binary.pgm");
	// OutputPgm(&binary);

	// Create a color copy.
	color = 2;
	color_image.xdim = image.xdim;
	color_image.ydim = image.ydim;
	color_image.highestvalue = WHITE;

	GetImagePpm(&color_image);
	for (i = 0; i < color_image.ydim; i++) {
		for (j = 0; j < color_image.xdim; j++) {
			color_image.r[i * image.xdim + j] = 255;
			color_image.g[i * image.xdim + j] = 255;
			color_image.b[i * image.xdim + j] = 255;
		}
	}

	
	MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	srand(time(NULL));

	clock_gettime(CLOCK_REALTIME, &start);
	trunc_number_lines  = image.ydim / numprocs;
	num_lines_proc = image.ydim / numprocs;
	if (rank == numprocs - 1) {
		num_lines_proc += image.ydim % numprocs;
	}

	//first and last lines for each process
	top_line = rank * trunc_number_lines;
	bottom_line = top_line + num_lines_proc - 1;
	//printf("rank %d; top line = %d, bottom_line = %d\n", rank, top_line, bottom_line);
    
	// Start real work
	for (j = 0; j < image.xdim; j++) {
		if (binary.value[top_line * image.xdim + j] == UNTOUCHED) {
			recursiveTouchMPI(&binary, &color_image, top_line, j, FIRST_COLOR, 0, rank);
		}
		if (binary.value[bottom_line * image.xdim + j] == UNTOUCHED) {
			recursiveTouchMPI(&binary, &color_image, bottom_line, j, FIRST_COLOR, 0, rank);
		}
	}
	color++;
	for (i = top_line + 1; i < bottom_line; i++) {
		for (j = 0; j < image.xdim; j++) {
			if (binary.value[i * image.xdim + j] == UNTOUCHED) {
				recursiveTouchMPI(&binary, &color_image, i, j, color, 0, rank);
				color++;
			}
		}
	}
	MPI_Gather(&(color_image.r[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR,
				&(color_image.r[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR, MASTER, MPI_COMM_WORLD);
	MPI_Gather(&(color_image.g[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR,
				&(color_image.g[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR, MASTER, MPI_COMM_WORLD);
	MPI_Gather(&(color_image.b[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR,
				&(color_image.b[top_line * image.xdim]), trunc_number_lines * image.xdim, MPI_UNSIGNED_CHAR, MASTER, MPI_COMM_WORLD);
	if (rank == numprocs - 1) {
		if (image.ydim % numprocs) {
			int first_line_to_send = bottom_line - (image.ydim % numprocs) + 1;
			
			MPI_Send(&(color_image.r[first_line_to_send * image.xdim]), (image.ydim % numprocs) * image.xdim, 
					MPI_UNSIGNED_CHAR, MASTER, tag, MPI_COMM_WORLD);
			MPI_Send(&(color_image.g[first_line_to_send * image.xdim]), (image.ydim % numprocs) * image.xdim, 
					MPI_UNSIGNED_CHAR, MASTER, tag, MPI_COMM_WORLD);
			MPI_Send(&(color_image.b[first_line_to_send * image.xdim]), (image.ydim % numprocs) * image.xdim, 
					MPI_UNSIGNED_CHAR, MASTER, tag, MPI_COMM_WORLD);
		}
	}

	if (rank == MASTER) {
		if (image.ydim % numprocs) {
			int first_line_to_receive = image.ydim - (image.ydim % numprocs);
			MPI_Recv(&(color_image.r[first_line_to_receive * image.xdim]), (image.ydim % numprocs) * image.xdim,
						MPI_UNSIGNED_CHAR, numprocs - 1, tag, MPI_COMM_WORLD, &status);
			MPI_Recv(&(color_image.g[first_line_to_receive * image.xdim]), (image.ydim % numprocs) * image.xdim,
						MPI_UNSIGNED_CHAR, numprocs - 1, tag, MPI_COMM_WORLD, &status);
			MPI_Recv(&(color_image.b[first_line_to_receive * image.xdim]), (image.ydim % numprocs) * image.xdim,
						MPI_UNSIGNED_CHAR, numprocs - 1, tag, MPI_COMM_WORLD, &status);
		}
	}

	clock_gettime(CLOCK_REALTIME, &stop);
	
	if (rank == 0) {
		// Print time 
		accum = ( stop.tv_sec - start.tv_sec )
			+ (double)( stop.tv_nsec - start.tv_nsec )
				/ (double)BILLION;
		printf("[Open MPI] Image segmentation: %lf\n", accum);


		// Create output images
		strcpy(color_image.name, "connected.ppm");
		OutputPpm(&color_image);

		binary.highestvalue = WHITE;
		strcpy(binary.name, "binary.pgm");
		OutputPgm(&binary);
	}

	MPI_Finalize();

	return 0;
}
