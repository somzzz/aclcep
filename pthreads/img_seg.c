
#include <math.h>
#include <pthread.h>
#include <queue>
#include <iostream>

#include "vision.h"
#include "helpers.c"
#include "errors.h"

#define BILLION  	1000000000L;
#define NUM_THREADS 5
#define CHUNK_SIZE	100
#define MASTER 		0

// #define DEBUG_HISTOGRAM 1
// #define DEBUG_BINARY 1
// #define DEBUG_ALPHA 1
// #define DEBUG_IMAGE 1

typedef struct {
	int block_x, block_y;
	int entry_x, entry_y;
	int color;
} Task;

// Global stuff for all threads
pthread_barrier_t workers_barrier1, workers_barrier2;
pthread_barrier_t global_barrier1, global_barrier2; 

pthread_mutex_t work_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t work_cv = PTHREAD_COND_INITIALIZER;

pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t done_cv = PTHREAD_COND_INITIALIZER;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visited_mutex = PTHREAD_MUTEX_INITIALIZER;   

grayscaleimage image;
grayscaleimage binary;
grayscaleimage mainobject;

rgbimage color_image;
int color;

long visited;
bool stop_signal, work;
int count;

// Workpool
std::queue<Task> tasks;

static bool is_in_chunk(int i, int j, int csy, int csx) {
	int csxi = j / CHUNK_SIZE * CHUNK_SIZE;
	int csyi = i / CHUNK_SIZE * CHUNK_SIZE;
	return (csxi == csx) && (csyi == csy);
}

static bool is_in_matrix(int i, int j) {
	return (0 <= i && i < binary.ydim)
		&& (0 <= j && j < binary.xdim);
}

static void put_color(int i, int j, int color) {

	// Fun colors!
	if (color % 7 == 0) {
		color_image.r[i][j] = 255;
		color_image.g[i][j] = 255;
		color_image.b[i][j] = 255;
	}
	else if (color % 7 == 1) {
		color_image.r[i][j] = 0;
		color_image.g[i][j] = 255;
		color_image.b[i][j] = 0;
	}
	else if (color % 7 == 2) {
		color_image.r[i][j] = 0;
		color_image.g[i][j] = 0;
		color_image.b[i][j] = 255;
	}
	else if (color % 7 == 3) {
		color_image.r[i][j] = 255;
		color_image.g[i][j] = 255;
		color_image.b[i][j] = 0;
	}
	else if (color % 7 == 4) {
		color_image.r[i][j] = 255;
		color_image.g[i][j] = 0;
		color_image.b[i][j] = 255;
	}
	else if (color % 7 == 5) {
		color_image.r[i][j] = 0;
		color_image.g[i][j] = 255;
		color_image.b[i][j] = 255;
	}
	else {
		color_image.r[i][j] = 255;
		color_image.g[i][j] = 0;
		color_image.b[i][j] = 0;
	}
}

// Thread work
static void *do_work(void *args) {
	int status;
	int x, y;
	bool has_task;

	long my_id = (long)args;

	Task tsk;
	std::queue<Task> local_tasks;
	long local_visited;

	// do work
	while (1) {
		//printf("Thread %ld waiting on local barrier 0\n", my_id);
		pthread_barrier_wait (&workers_barrier1);
		//printf("Thread %ld after local barrier 0\n", my_id);

		// Wait for master to put something in queue
		if (tasks.empty()) {
			printf("Thread %ld waiting on global barrier 1\n", my_id);
			pthread_barrier_wait (&global_barrier1);
			printf("Thread %ld after global barrier 1\n", my_id);
			
			// wait for work request
			status = pthread_mutex_lock(&work_mutex);
	        if (status) err_abort(status, "lock mutex");

	        count++;
	        status = pthread_cond_wait(&work_cv, &work_mutex);
	        if (status) err_abort(status, "wait for condition");

	        status = pthread_mutex_unlock(&work_mutex);
	        if (status) err_abort(status, "unlock mutex");

			if (stop_signal) break;

		}

		//printf("Thread %ld waiting on local barrier 1\n", my_id);
		pthread_barrier_wait (&workers_barrier1);
		//printf("Thread %ld after local barrier 1\n", my_id);

		// get a task
		status = pthread_mutex_lock(&queue_mutex);
		if (status) err_abort(status, "lock mutex");

		if (tasks.size() != 0) {
			tsk = tasks.front();
			tasks.pop();
			has_task = true;
		} else {
			has_task = false;
		}

		status = pthread_mutex_unlock(&queue_mutex);
		if (status) err_abort(status, "unlock mutex");

		//printf("Thread %ld waiting on local barrier 1.1\n", my_id);
		pthread_barrier_wait (&workers_barrier1);
		//printf("Thread %ld after local barrier 1.1\n", my_id);

		if (has_task) { 
			// got a task
			// => do a bfs in the assigned chunk
			// => create new tasks for the neighbour chunks

			//printf("Thread %ld\n", my_id);
			//printf("Thread %ld, got task with blk_x = %d, blk_y = %d, ex = %d, ey = %d\n",
			//	my_id, tsk.block_x, tsk.block_y, tsk.entry_x, tsk.entry_y);

			local_visited = 0;
			std::queue<Task> local_bfs;

			Task root;
			root.entry_x = tsk.entry_x; //coloane
			root.entry_y = tsk.entry_y; // linii
			local_bfs.push(root);

			while (!local_bfs.empty()) {
				local_visited++;

				Task t = local_bfs.front();
				local_bfs.pop();

				int i = t.entry_y;
				int j = t.entry_x;

				//printf("Thread %ld, task i=  %d j = %d cell =%d \n", my_id, i, j, binary.value[i][j]);

				// Found an untouched value in a neighbour chunk => give work to another thread
				if (binary.value[i][j] == UNTOUCHED && !is_in_chunk(i, j, tsk.block_y, tsk.block_x)) {
					Task new_tsk;
					new_tsk.entry_y = i; new_tsk.entry_x = j;
					new_tsk.color = tsk.color;
					new_tsk.block_y = i / CHUNK_SIZE * CHUNK_SIZE;
					new_tsk.block_x = j / CHUNK_SIZE * CHUNK_SIZE;
						
					//printf("Thread %ld, created new task with blk_x = %d, blk_y = %d, ex = %d, ey = %d\n",
					//	my_id, new_tsk.block_x, new_tsk.block_y, new_tsk.entry_x, new_tsk.entry_y);

					local_tasks.push(new_tsk);
				}

				// Found an untouched value in the current chunk => process it
				if (binary.value[i][j] == UNTOUCHED && is_in_chunk(i, j, tsk.block_y, tsk.block_x)) {
			
					// Visit current cell
					binary.value[i][j] = BLACK;
					put_color(i, j, tsk.color);

					//std::cout << "thread " << my_id << " visiting cell i = " << i << " j = " << j << std::endl;

					// Add neighbouring cells of interest to the visit queue
					if (is_in_matrix(i, j - 1) && binary.value[i][j - 1] == UNTOUCHED) {
						t.entry_y = i; t.entry_x = j - 1;
						local_bfs.push(t);
					}
					if (is_in_matrix(i, j + 1) && binary.value[i][j + 1] == UNTOUCHED) {
						t.entry_y = i; t.entry_x = j + 1;
						local_bfs.push(t);
					}
					if (is_in_matrix(i - 1, j - 1) && binary.value[i - 1][j - 1] == UNTOUCHED) {
						t.entry_y = i - 1; t.entry_x = j - 1;
						local_bfs.push(t);
					}
					if (is_in_matrix(i - 1, j) && binary.value[i - 1][j] == UNTOUCHED) {
						t.entry_y = i - 1; t.entry_x = j;
						local_bfs.push(t);
					}
					if (is_in_matrix(i - 1, j + 1) && binary.value[i - 1][j + 1] == UNTOUCHED) {
						t.entry_y = i - 1; t.entry_x = j + 1;
						local_bfs.push(t);
					}
					if (is_in_matrix(i + 1, j - 1) && binary.value[i + 1][j - 1] == UNTOUCHED) {
						t.entry_y = i + 1; t.entry_x = j - 1;
						local_bfs.push(t);
					}
					if (is_in_matrix(i + 1, j) && binary.value[i + 1][j] == UNTOUCHED) {
						t.entry_y = i + 1; t.entry_x = j;
						local_bfs.push(t);
					}
					if (is_in_matrix(i + 1, j + 1) && binary.value[i + 1][j + 1] == UNTOUCHED) {
						t.entry_y = i + 1; t.entry_x = j + 1;
						local_bfs.push(t);
					}
				} // end if

			} // endwhile local bfs

			// add work found to queue
			status = pthread_mutex_lock(&queue_mutex);
			if (status) err_abort(status, "lock mutex");

			while (!local_tasks.empty()) {
				tasks.push(local_tasks.front());
				local_tasks.pop();
			}

			status = pthread_mutex_unlock(&queue_mutex);
			if (status) err_abort(status, "unlock mutex");
		} // if has task

		// Wait all threads to finish computation round
		//printf("Thread %ld waiting on local barrier 2\n", my_id);
		pthread_barrier_wait (&workers_barrier1);
		//printf("Thread %ld after local barrier 2\n", my_id);

		//printf("Thread %ld with tasks %d\n", my_id, tasks.size());
		// check stop condition && signal master
		// if (tasks.empty()) {
		// 							printf("Thread %ld waiting on global barrier 2\n", my_id);
		// 	pthread_barrier_wait (&global_barrier2);
		// 							printf("Thread %ld after global barrier 2\n", my_id);
		// }

	} // endwhile thread

    // work done
	printf("Threads %ld, exiting \n", my_id );
    pthread_exit(NULL);

	return 0;
}

static void *master_work(void *args) {
	int i, j;
	int status;
	long my_id = (long)args;

    stop_signal = false;
    count = 0;
    
    status = pthread_mutex_lock(&work_mutex);
    if (status) err_abort(status, "lock mutex");

    while (!work) {
        status = pthread_cond_wait(&work_cv, &work_mutex);
        if (status) err_abort(status, "wait for condition");
    }

    status = pthread_mutex_unlock(&work_mutex);
    if (status) err_abort(status, "unlock mutex");

	for (i = 0; i < color_image.ydim; i++) {
		for (j = 0; j < color_image.xdim; j++) {

			if (binary.value[i][j] == UNTOUCHED) {
				Task tsk;
    			tsk.block_y = i / CHUNK_SIZE * CHUNK_SIZE;
    			tsk.block_x = j / CHUNK_SIZE * CHUNK_SIZE;
    			tsk.entry_y = i; tsk.entry_x = j;
    			tsk.color = color;

				//printf("===========>Thread %ld creating TASk bx = %d by = %d ex = %d ey = %d -\n",
				//my_id,tsk.block_x, tsk.block_y, tsk.entry_x, tsk.entry_y);
    			tasks.push(tsk);

				// Signal threads to start work
				printf("Thread %ld waiting on global barrier 1\n", my_id);
				pthread_barrier_wait (&global_barrier1);
				printf("Thread %ld after global barrier 1\n", my_id);

       			while (count < NUM_THREADS - 1);
                status = pthread_mutex_lock(&work_mutex);
                if (status) err_abort(status, "lock mutex");

                count = 0;
                status = pthread_cond_broadcast(&work_cv);
                if (status) err_abort(status, "signal condition");

                status = pthread_mutex_unlock(&work_mutex);
                if (status) err_abort(status, "unlock mutex");
    			
    			// Wait for threads to finish object
				//printf("Thread %ld waiting on global barrier 2\n", my_id);
				//pthread_barrier_wait (&global_barrier2);
				//printf("Thread %ld after global barrier 2\n", my_id);
                
                color++;
			}
		}
	}

	printf("Thread %ld waiting on global barrier 1\n", my_id);
	pthread_barrier_wait (&global_barrier1);
	printf("Thread %ld after global barrier 1\n", my_id);

	while (count < NUM_THREADS - 1);
	status = pthread_mutex_lock(&work_mutex);
	if (status) err_abort(status, "lock mutex");
                
	count = 0;
	stop_signal = true;
	status = pthread_cond_broadcast(&work_cv);
	if (status) err_abort(status, "signal condition");

	status = pthread_mutex_unlock(&work_mutex);
	if (status) err_abort(status, "unlock mutex");

	// Signal threads to do one last round of work to see the stop_signal
	// and exit properly
	printf("Threads %ld, exiting \n", my_id );
    pthread_exit(NULL);
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

    work = false;

	// Start threads & create barrier
	pthread_barrier_init (&workers_barrier1, NULL, NUM_THREADS - 1);
	pthread_barrier_init (&workers_barrier2, NULL, NUM_THREADS - 1);
	pthread_barrier_init (&global_barrier1, NULL, NUM_THREADS);
	pthread_barrier_init (&global_barrier2, NULL, NUM_THREADS);

	pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    status = pthread_create(&threads[0], &attr, master_work, (void *)0);
    if (status) err_abort(status, "create thread");

    for (t = 1; t < NUM_THREADS; t++) {
        status = pthread_create(&threads[t], &attr, do_work, (void *)t);
        if (status) err_abort(status, "create thread");
    }

	// Grab image, place in memory.

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
	color = 1;
	color_image.xdim = image.xdim;
	color_image.ydim = image.ydim;
	color_image.highestvalue = WHITE;

	GetImagePpm(&color_image);
	for (i = 0; i < color_image.ydim; i++) {
		for (j = 0; j < color_image.xdim; j++) {
			color_image.r[i][j] = 255;
			color_image.g[i][j] = 255;
			color_image.b[i][j] = 255;
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


    // Signal master to start working
    status = pthread_mutex_lock(&work_mutex);
    if (status) err_abort(status, "lock mutex");

    work = true;

    status = pthread_cond_signal(&work_cv);
    if (status) err_abort(status, "signal condition");

    status = pthread_mutex_unlock(&work_mutex);
    if (status) err_abort(status, "unlock mutex");


	// Wait for all threads to complete
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }
    printf ("Main(): Waited on %d threads. Done.\n", NUM_THREADS);

    clock_gettime(CLOCK_REALTIME, &stop);


    // Print time 
    accum = ( stop.tv_sec - start.tv_sec )
        + (double)( stop.tv_nsec - start.tv_nsec )
        	/ (double)BILLION;
    printf("[PTHREADS] Image segmentation: %lf\n", accum);


    // Create output images
	strcpy(color_image.name, "connected.ppm");
	OutputPpm(&color_image);

	binary.highestvalue = WHITE;
	strcpy(binary.name, "binary.pgm");
	OutputPgm(&binary);


    /* Clean up and exit */
    pthread_attr_destroy(&attr);

    pthread_mutex_destroy(&work_mutex);
    pthread_cond_destroy(&work_cv);

    pthread_mutex_destroy(&done_mutex);
    pthread_cond_destroy(&done_cv);

    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&visited_mutex);

    pthread_exit(NULL);

	return 0;
}
