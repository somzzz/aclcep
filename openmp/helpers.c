// #define DEBUG_HISTOGRAM 1
// #define DEBUG_PEAKINESS 1
// #define DEBUG_ALPHA 1
#include <queue>
#include <omp.h>
struct Task
{
	int i_start;
	int i_stop;
	int j_start;
	int j_stop;
	int seed_i;
	int seed_j;
	int color;
	int chunck;
};

const int INF = 65535;
const int WHITE = 255;
const int BLACK = 0;
const int UNTOUCHED = 42;
const int MAXVAL = 200;
const int MINAREA = 100;

// Loop iterators.
int i, j, k, x;

void sniperBlur(grayscaleimage* image, int xp, int yp, int times); 

/* calculatePeakiness------------------------------------------------------*/
/* ------------------------------------------------------------------------*/
// {{{ 
int calculatePeakiness(grayscaleimage* image, int xstart, int xstop) 
{
	// Range of minimum distances to try.
	const int start_min_distance = 2;
	const int stop_min_distance  = 32;
	int used_distance = 0;

	// g1 : Highest histogram count.
	// gi : The gray value where g1 occurs; H(gi) = g1.
	int g1, gi; 

	// g2 : Second highest histogram count at least minimum_distance from g1..
	// gj : The gray value where g2 occurs: H(gj) = g2.
	int g2, gj; 

	// gmin: Smallest histogram value between gi and gj.
	// gk : The gray value where gmin occurs: H(gk) = gmin.
	int gmin, gk; 

	double peakiness = 0;
	double max_peakiness = 0;
	int threshold = 0;
	int sum = 0;
	int light = 0;
	int dark = 0;

	// Initialize histogram.
	unsigned int histogram[256];

	for (i = 0; i < 256; i++) {
		histogram[i] = 0;
	}

	for (i = 0; i < image->ydim; i++) {
		for (j = xstart; j < xstop; j++) {
			histogram[image->value[i][j]] += 1;
		}
	}

	// Grab highest value in the histogram.
	g1 = 0;
	gi = 0;
	for (i = 0; i < MAXVAL; i++) {
		if (histogram[i] > g1) {
			g1 = histogram[i];
			gi = i;
		}
	}

	for (i = 0; i < 256; i++) {
		if (histogram[i] > 10) {
			dark = i;
			break;
		}
	}

	for (i = 255; i > 0; i--) {
		if (histogram[i] > 10) {
			light = i;
			break;
		}
	}

#ifdef DEBUG_PEAKINESS
	printf("Light: %d\n", light);
	printf("Dark: %d\n", dark);
#endif

	if (!((xstart == 0) && (xstop == image->xdim))) {
		printf("You are ever called in calculatePeakiness.....\n");
		// Contrast normalization.
		for (i = 1; i < image->ydim; i++) {
			for (j = xstart + 1; j < xstop - 1; j++) {
				// Reduce black pixels, replacing them with background.
				if (image->value[i][j] <= dark + 18) {
					sniperBlur(image, i, j, 15);
				}

				// Reduce white pixels, replacing them with background.
				if (image->value[i][j] >= light - 18) {
					sniperBlur(image, i, j, 15);
				}
			}
		}

		// Reevaluate histogram after contrast normalization.
		for (i = 0; i < 256; i++) {
			histogram[i] = 0;
		}

		for (i = 0; i < image->ydim; i++) {
			for (j = xstart; j < xstop; j++) {
				if (image->value[i][j] < light - 25) {
					histogram[image->value[i][j]] += 1;
				}
			}
		}
	}

#ifdef DEBUG_HISTOGRAM
		for (i = 0; i < 256; i++) {
			printf("histogram[%d]: %d\n", i, histogram[i]);
		}
#endif

	// Grab highest value in the histogram.
	g1 = 0;
	gi = 0;
	for (i = 0; i < MAXVAL; i++) {
		if (histogram[i] > g1) {
			g1 = histogram[i];
			gi = i;
		}
	}

	for (x = start_min_distance; x < stop_min_distance; x++) {
		// Grab second highest value in the histogram that is at least 
		// minimum_distance away from the highest value.
		g2 = 0;
		gj = 0;
		for (j = 0; j < MAXVAL; j++) {
			if (j > gi - x && j < gi + x) {}
			else if (histogram[j] > g2) {
				g2 = histogram[j];
				gj = j;
			}
		}

		// Find the lowest point gmin in the histogram between g1 and g2.
		gmin = image->ydim * image->xdim;
		if (gi < gj) {
			for (k = gi; k < gj; k++) {
				if (histogram[k] < gmin) {
					gmin = histogram[k];
					gk = k;
				}
			}
		}
		else {
			for (k = gj; k < gi; k++) {
				if (histogram[k] < gmin) {
					gmin = histogram[k];
					gk = k;
				}
			}
		} 

#ifdef DEBUG_PEAKINESS
		printf("Highest value %d found at histogram[%d].\n", g1, gi);
		printf("2nd highest value %d found at histogram[%d].\n", g2, gj);
		printf("Smallest value %d found between at histogram[%d].\n", gmin, gk);
#endif

		// Find the maximum peakiness.
		/*if (g1 < g2) {
			peakiness = (double) g1 / (double) gmin;
		}
		else {
			peakiness = (double) g2 / (double) gmin;
		}*/
		if (gmin == 0)
		{
			gmin = 1;
			if (g1 < g2) {
			peakiness = (double) g1 / (double) gmin;
			}
			else {
				peakiness = (double) g2 / (double) gmin;
			}
		} else {
			if (g1 < g2) {
			peakiness = (double) g1 / (double) gmin;
			}
			else {
				peakiness = (double) g2 / (double) gmin;
			}
		}

		if (peakiness > max_peakiness && peakiness != 0 && peakiness < INF) {
			max_peakiness = peakiness;
			threshold = gk;
			used_distance = x;
		}

#ifdef DEBUG_PEAKINESS
		printf("Peakiness: %f\n", peakiness);
#endif
	} // end minimum distance for loop
#ifdef DEBUG_PEAKINESS
	printf("Maximum Peakiness: %f at %d with distance %d\n", 
			max_peakiness, threshold, used_distance);
#endif

	return threshold;
}
// }}}

/* normalizeBackground----------------------------------------------------*/
/* ------------------------------------------------------------------------*/
// {{{
void normalizeBackground(grayscaleimage* binary, int xstart, int xstop) {
	int area = 0;
	int i, j;
	for (i = 0; i < binary->ydim; i++) {
		for (j = xstart; j < xstop; j++) {
			if (binary->value[i][j] == BLACK) {
				area++;
			}
		}
	}

	// Swap foreground and background if there is more foreground than back.
	if (area > (binary->xdim * binary->ydim)/4) {
		printf("Inverting binary image.\n");
		for (i = 0; i < binary->ydim; i++) {
			for (j = xstart; j < xstop; j++) {
				if (binary->value[i][j] == BLACK) {
					binary->value[i][j] = WHITE;
				}
				else {
					binary->value[i][j] = BLACK;
				}
			}
		}
	}

	return;
}
// }}}


/* recursiveTouch----------------------------------------------------------*/
/* ------------------------------------------------------------------------*/
// {{{
int recursiveTouch(grayscaleimage *binary, rgbimage *output, 
		int i, int j, int color, unsigned long area) 
{
	binary->value[i][j] = BLACK;
	area++;

	// Fun colors!
	if (color % 7 == 0) {
		output->r[i][j] = 255;
		output->g[i][j] = 255;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 1) {
		output->r[i][j] = 0;
		output->g[i][j] = 255;
		output->b[i][j] = 0;
	}
	else if (color % 7 == 2) {
		output->r[i][j] = 0;
		output->g[i][j] = 0;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 3) {
		output->r[i][j] = 255;
		output->g[i][j] = 255;
		output->b[i][j] = 0;
	}
	else if (color % 7 == 4) {
		output->r[i][j] = 255;
		output->g[i][j] = 0;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 5) {
		output->r[i][j] = 0;
		output->g[i][j] = 255;
		output->b[i][j] = 255;
	}
	else {
		output->r[i][j] = 255;
		output->g[i][j] = 0;
		output->b[i][j] = 0;
	}

	if (j - 1 >= 0 && binary->value[i][j-1] == UNTOUCHED) {
		area = recursiveTouch(binary, output, i, j-1, color, area);
	}
	if (j + 1 < binary->xdim && binary->value[i][j+1] == UNTOUCHED) {
		area = recursiveTouch(binary, output, i, j+1, color, area);
	}

	if (i - 1 >= 0) {
		if (j - 1 >= 0 && binary->value[i-1][j-1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j-1, color, area);
		}
		if (binary->value[i-1][j] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j, color, area);
		}
		if (j + 1 < binary->xdim && binary->value[i-1][j+1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i-1, j+1, color, area);
		}
	}

	if (i + 1 < binary->ydim) {
		if (j - 1 >= 0 && binary->value[i+1][j-1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j-1, color, area);
		}
		if (binary->value[i+1][j] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j, color, area);
		}
		if (j + 1 < binary->xdim && binary->value[i+1][j+1] == UNTOUCHED) {
			area = recursiveTouch(binary, output, i+1, j+1, color, area);
		}
	}

	return area;
}
//}}}

/*-------------------------------------------*/
int searchWork(int *a, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if (a[i] == true)
		{
			return 1;
		}
	}
	return 0;
}

/*-----------------------set color------------------------*/

void set_color(rgbimage* output, int i, int j, int color)
{
	// Fun colors!
	if (color % 7 == 0) {
		output->r[i][j] = 255;
		output->g[i][j] = 255;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 1) {
		output->r[i][j] = 0;
		output->g[i][j] = 255;
		output->b[i][j] = 0;
	}
	else if (color % 7 == 2) {
		output->r[i][j] = 0;
		output->g[i][j] = 0;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 3) {
		output->r[i][j] = 255;
		output->g[i][j] = 255;
		output->b[i][j] = 0;
	}
	else if (color % 7 == 4) {
		output->r[i][j] = 255;
		output->g[i][j] = 0;
		output->b[i][j] = 255;
	}
	else if (color % 7 == 5) {
		output->r[i][j] = 0;
		output->g[i][j] = 255;
		output->b[i][j] = 255;
	}
	else {
		output->r[i][j] = 255;
		output->g[i][j] = 0;
		output->b[i][j] = 0;
	}
}

/*------------------------------------------------------*/

struct Task generateTask(int x_coord, int  y_coord, int color, int chunck, int max_x, int max_y)
{
	struct Task create_task;
	create_task.seed_i = x_coord;
	create_task.seed_j = y_coord;
	//rintf("Detect floatig point operation %d\n", chunck);
	if (chunck != 0) {
		create_task.i_start = (create_task.seed_i / chunck) * chunck;
		create_task.j_start = (create_task.seed_j / chunck) * chunck;
	}
	if (create_task.i_start + chunck > max_y)
	{
		create_task.i_stop = max_y; 
	} else {
		create_task.i_stop = create_task.i_start + chunck;
	}
	if (create_task.j_start + chunck > max_x)
	{
		create_task.j_stop = max_x; 
	} else {
		create_task.j_stop = create_task.j_start + chunck;
	}
	create_task.color = color;
	create_task.chunck = chunck;
	return create_task;
}

/*----------------------------------------------------*/
int is_in_chunck(int x, int y, struct Task* task_limits)
{
	return (x >= task_limits->i_start && x < task_limits->i_stop &&
			y >= task_limits->j_start && y < task_limits->j_stop); 
}


/*---------------------------------------------------*/
int is_in_limits_matrix(int x, int y, int x_dim, int y_dim)
{
	return (x >= 0 && x < x_dim && y >= 0 && y < y_dim); 
}

/*--------------------------------------------------
-------OpenMP touch--------------------------------*/

int openmp_Touch(grayscaleimage *binary, rgbimage *output, 
		int i, int j, int color, int area, struct Task* threads_task, std::queue<struct Task>* my_queue, int nr) 
{
	if (threads_task == NULL)
	{
		return 0;
	}
	nr = omp_get_thread_num();
	//struct Task new_task;
	binary->value[i][j] = BLACK;
	#pragma omp private(area)
		area++; 
	set_color(output, i, j, color);
	//go to the left
    if (j - 1 >= 0 && binary->value[i][j-1] == UNTOUCHED)
    {
    	if (j - 1 < threads_task->j_start)
    	{
    		//generare de task nou
    		struct Task new_task = generateTask(i , j - 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    		my_queue->push(new_task);
    	} else if (j - 1 >= threads_task->j_start) {
    		area = recursiveTouch(binary, output, i, j-1, color, area);	
    	}
    }

    //go to the right
    if (j + 1 < binary->xdim && binary->value[i][j+1] == UNTOUCHED)
    {
    	if (j + 1 >= threads_task->j_stop)
    	{
    		//generate a task
    		struct Task new_task = generateTask(i , j + 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    		my_queue->push(new_task);
    	} else if (j + 1 < threads_task->j_stop) {
    		area = recursiveTouch(binary, output, i, j+1, color, area);	
    	}
    }

    //go to previous line: left, above, right
	if (i - 1 >= 0) {
		if (i - 1 < threads_task->i_start) {
			if (j- 1 >= 0 && binary->value[i-1][j-1] == UNTOUCHED) {
				struct Task new_task = generateTask(i - 1, j - 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);
			}
			if (binary->value[i-1][j] == UNTOUCHED) {
				struct Task new_task = generateTask(i - 1, j, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);	
			}	
			if (j + 1 < binary->xdim && binary->value[i-1][j+1] == UNTOUCHED) {
				struct Task new_task = generateTask(i - 1, j + 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);
			}
		} else if (i - 1 >= threads_task->i_start) { // linia i-1 e in patratica threadului corespunzator
			if (j - 1 >= 0 && binary->value[i-1][j-1] == UNTOUCHED) {
				if (j - 1 >= threads_task->j_start ) {
					area = recursiveTouch(binary, output, i-1, j-1, color, area);
				} else if (j - 1 < threads_task->j_start) {
					struct Task new_task = generateTask(i - 1, j - 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    				my_queue->push(new_task);
				}
			}
			if (binary->value[i-1][j] == UNTOUCHED) {
				area = recursiveTouch(binary, output, i-1, j, color, area);
			}
			if (j + 1 < binary->xdim && binary->value[i-1][j+1] == UNTOUCHED) {
				if (j + 1 < threads_task->j_stop) {
					area = recursiveTouch(binary, output, i-1, j+1, color, area);	
				} else if (j + 1 >= threads_task->j_stop) {
					struct Task new_task = generateTask(i - 1, j + 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    				my_queue->push(new_task);	
				}
			}
		}
	}

	if (i + 1 < binary->ydim) {
		if (i + 1 >= threads_task->i_stop) { //linia i+1 e in afara patratului threadului respectiv
			if (j - 1 >= 0 && binary->value[i+1][j-1] == UNTOUCHED) {
				struct Task new_task = generateTask(i + 1, j - 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);
			}
			if (binary->value[i+1][j] == UNTOUCHED) {
				struct Task new_task = generateTask(i + 1, j, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);	
			}
			if (j + 1 < binary->xdim && binary->value[i+1][j+1] == UNTOUCHED) {
				struct Task new_task = generateTask(i + 1, j + 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
				my_queue->push(new_task);
			}	
		} else if (i + 1 < threads_task->i_stop ) { //linia i+1 este in patratica corespunzatoare threadului
			if (j - 1 >= 0 && binary->value[i+1][j-1] == UNTOUCHED) {
				if (j - 1 >= threads_task->j_start) {
					area = recursiveTouch(binary, output, i+1, j-1, color, area);		
				} else if (j - 1 < threads_task->j_start) {
					struct Task new_task = generateTask(i + 1, j - 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    				my_queue->push(new_task);	
				}		
			}
			if (binary->value[i+1][j] == UNTOUCHED) {
				area = recursiveTouch(binary, output, i+1, j, color, area);
			}
			if (j + 1 < threads_task->j_stop) {
				area = recursiveTouch(binary, output, i+1, j+1, color, area);
			} else if (j + 1 >= threads_task->j_stop) {
				struct Task new_task = generateTask(i + 1, j + 1, threads_task->color, threads_task->chunck, binary->xdim, binary->ydim);
    			my_queue->push(new_task);
			}
		}
	}
	return area;
}


/* removeSpecks------------------------------------------------------------*/
/* ------------------------------------------------------------------------*/
//{{{
void removeSpecks(grayscaleimage *binary, int i, int j) {
	binary->value[i][j] = WHITE;

	if (j - 1 >= 0 && binary->value[i][j-1] == BLACK) {
		removeSpecks(binary, i, j-1);
	}
	if (j + 1 < binary->xdim && binary->value[i][j+1] == BLACK) {
		removeSpecks(binary, i, j+1);
	}

	if (i - 1 >= 0) {
		if (j - 1 >= 0 && binary->value[i-1][j-1] == BLACK) {
			removeSpecks(binary, i-1, j-1);
		}
		if (binary->value[i-1][j] == BLACK) {
			removeSpecks(binary, i-1, j);
		}
		if (j + 1 < binary->xdim && binary->value[i-1][j+1] == BLACK) {
			removeSpecks(binary, i-1, j+1);
		}
	}

	if (i + 1 < binary->ydim) {
		if (j - 1 >= 0 && binary->value[i+1][j-1] == BLACK) {
			removeSpecks(binary, i+1, j-1);
		}
		if (binary->value[i+1][j] == BLACK) {
			removeSpecks(binary, i+1, j);
		}
		if (j + 1 < binary->xdim && binary->value[i+1][j+1] == BLACK) {
			removeSpecks(binary, i+1, j+1);
		}
	}

	return;
}
//}}}

/* findArea ---------------------------------------------------------------*/
/* ------------------------------------------------------------------------*/

//{{{
void sniperBlur(grayscaleimage* image, int xp, int yp, int times) {
	int iter = 0;
	for (iter = 0; iter < times; iter++) {
		// Accentuates white while reducing black.
		image->value[xp][yp] = 
			(10 * image->value[xp-1][yp-1] + 
			 5 * image->value[xp-1][yp] + 
			 5 * image->value[xp][yp-1]) / 18; 
		/*
		   (image->value[xp-1][yp-1] + 
		   image->value[xp-1][yp] + 
		   image->value[xp-1][yp+1] + 

		   image->value[xp][yp-1] + 
		   image->value[xp][yp+1] + 

		   image->value[xp+1][yp-1] + 
		   image->value[xp+1][yp] + 
		   image->value[xp+1][yp+1]) / 8; 
	   */
	}

	return;
}
//}}}


