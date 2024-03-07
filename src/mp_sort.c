/* Code adapted from code by Shane Fitzpatrick*/

#include <omp.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <sys/time.h>
#include "common.c"


/* headers */
int compareints(const void * ptr2num1, const void * ptr2num2);
void calc_partition_borders(int array[], int start, int end, int sublist_sizes[], int at, int pivot_values[], int first_p, int last_p);
void MP_sort_array(int *a, int n);
void allocate_memory(int ***local_a_ptrs, int **sample, int **partition_borders, int **bucket_sizes, int **result_positions, int **pivot_values, int p, int sample_size);
void calculate_start_end(int thread_num, int size, int n, int *start, int *end, int *local_size);
int *create_local_copy(int *a, int start, int local_size, int **local_a_ptrs, int thread_num);
void perform_sampling(int *local_a, int rsize, int end, int *sample, int thread_num, int p);
void select_pivot_values(int *sample, int sample_size, int *pivot_values, int p);
void calculate_partition_borders(int *local_a, int local_size, int *partition_borders, int thread_num, int *pivot_values, int p);
void calculate_bucket_sizes(int *partition_borders, int *bucket_sizes, int thread_num, int p);
void calculate_result_positions(int *bucket_sizes, int *result_positions, int p);
void collect_local_arrays(int *a, int *result_positions, int **local_a_ptrs, int *partition_borders, int thread_num, int p, int n, int *this_result_size);
void free_allocated_memory(int **local_a_ptrs, int *sample, int *partition_borders, int *bucket_sizes, int *result_positions, int *pivot_values);


void MP_sort_array(int *a, int n) {


if(n > 1){
    int p, size, rsize, sample_size;
    int *sample, *pivot_values;
    int *partition_borders, *bucket_sizes, *result_positions;
    int **local_a_ptrs;
#pragma omp parallel
    {
#pragma omp single
    p = omp_get_num_threads();
    }

    size  = (n + p - 1) / p;
    rsize = (size + p - 1) / p;
    sample_size = p * (p - 1);

    allocate_memory(&local_a_ptrs, &sample, &partition_borders, &bucket_sizes, &result_positions, &pivot_values, p, sample_size);

    #pragma omp parallel
    {
        int thread_num, start, end, local_size, this_result_size;
        int *local_a;

        thread_num = omp_get_thread_num();
        calculate_start_end(thread_num, size, n, &start, &end, &local_size);
	// step 1
        local_a = create_local_copy(a, start, local_size, local_a_ptrs, thread_num);
	// step 2
	quickSort(local_a, 0, local_size);
	// step 3
        perform_sampling(local_a, rsize, end, sample, thread_num, p);

        #pragma omp barrier

        #pragma omp single
        {
            select_pivot_values(sample, sample_size, pivot_values, p);
        }

        #pragma omp barrier
	// step 4
        calculate_partition_borders(local_a, local_size, partition_borders, thread_num, pivot_values, p);

        #pragma omp barrier

        calculate_bucket_sizes(partition_borders, bucket_sizes, thread_num, p);

        #pragma omp barrier

        #pragma omp single
        {
            calculate_result_positions(bucket_sizes, result_positions, p);
        }
	// step 5 & 6
        #pragma omp barrier
        collect_local_arrays(a, result_positions, local_a_ptrs, partition_borders, thread_num, p, n, &this_result_size);
	
        #pragma omp barrier
        
        free(local_a);
    }

    free_allocated_memory(local_a_ptrs, sample, partition_borders, bucket_sizes, result_positions, pivot_values);
}
}


void allocate_memory(int ***local_a_ptrs, int **sample, int **partition_borders, int **bucket_sizes, int **result_positions, int **pivot_values, int p, int sample_size) {
    *local_a_ptrs = malloc(p * sizeof(int *));
    *sample = malloc(sample_size * sizeof(int));
    *partition_borders = malloc(p * (p + 1) * sizeof(int));
    *bucket_sizes = malloc(p * sizeof(int));
    *result_positions = malloc(p * sizeof(int));
    *pivot_values = malloc((p - 1) * sizeof(int));
}

void calculate_start_end(int thread_num, int size, int n, int *start, int *end, int *local_size) {
    *start = thread_num * size;
    *end = *start + size - 1;
    if(*end >= n) *end = n - 1;
    *local_size = (*end - *start + 1);
    *end = *end % size;
}

int *create_local_copy(int *a, int start, int local_size, int **local_a_ptrs, int thread_num) {
    int *local_a = malloc(local_size * sizeof(int));
    memcpy(local_a, a + start, local_size * sizeof(int));
    //local_a = a + start;
    local_a_ptrs[thread_num] = local_a;
    return local_a;
}

void perform_sampling(int *local_a, int rsize, int end, int *sample, int thread_num, int p) {
    int i, offset = thread_num * (p - 1) - 1;
    for(i = 1; i < p; i++) {
        if(i * rsize <= end) {
        sample[offset + i] = local_a[i * rsize - 1];
        } else {
        sample[offset + i] = local_a[end];
        }
    }
}

void select_pivot_values(int *sample, int sample_size, int *pivot_values, int p) {
    int i;
    quickSort(sample, 0, sample_size);
    for(i = 0; i < p - 1; i++) {
        pivot_values[i] = sample[i * p + p / 2];
    }
}

void calculate_partition_borders(int *local_a, int local_size, int *partition_borders, int thread_num, int *pivot_values, int p) {
    int offset = thread_num * (p + 1);
    partition_borders[offset] = 0;
    partition_borders[offset + p] = local_size;
    calc_partition_borders(local_a, 0, local_size-1, partition_borders, offset, pivot_values, 1, p-1);
}

void calculate_bucket_sizes(int *partition_borders, int *bucket_sizes, int thread_num, int p) {
    int i, max = p * (p + 1);
    bucket_sizes[thread_num] = 0;
    for(i = thread_num; i < max; i += p + 1) {
        bucket_sizes[thread_num] += partition_borders[i + 1] - partition_borders[i];
    }
}

void calculate_result_positions(int *bucket_sizes, int *result_positions, int p) {
    int i;
    result_positions[0] = 0;
    for(i = 1; i < p; i++) {
        result_positions[i] = bucket_sizes[i-1] + result_positions[i-1];
    }
}

void in_p_merge(int *merged_array, int** arrays, int* array_size, int num_arrays, int p_merge_size){
	int counter[num_arrays]; for(int i = 0; i < num_arrays; ++i) counter[i] = 0;
	// p merge sort
	for(int i = 0; i < p_merge_size; ++i){
		int smallest = INT_MAX;
		int smallest_ind = -1;
		for(int j = 0; j < num_arrays; ++j){
			int ind = counter[j];
			// make sure the array is not empty
			if(ind < array_size[j]){
				if(arrays[j][ind] < smallest){
					smallest = arrays[j][ind];
					smallest_ind = j;
				}
			}
		}
		// mark element as used
		counter[smallest_ind]++;
    	merged_array[i] = smallest;
	}
}


void collect_local_arrays(int *a, int *result_positions, int **local_a_ptrs, int *partition_borders, int thread_num, int p, int n, int *this_result_size) {
    int i;
    int *this_result = a + result_positions[thread_num];

    if(thread_num == p-1) {
        *this_result_size = n - result_positions[thread_num];
    } else {
        *this_result_size = result_positions[thread_num+1] - result_positions[thread_num];
    }
	int **p_merge_arrays = malloc(p*sizeof(int*));
	int *p_merge_arrays_size = malloc(p*sizeof(int));
	int p_merge_size = 0;
    for(i = 0; i < p; i++) {
        int low, high, partition_size;
        int offset = i * (p + 1) + thread_num;
        low = partition_borders[offset];
        high = partition_borders[offset+1];
        partition_size = (high - low);
	
	p_merge_arrays[i] = &local_a_ptrs[i][low];
	p_merge_arrays_size[i] = partition_size;
        p_merge_size += partition_size;
    }
    in_p_merge(this_result, p_merge_arrays, p_merge_arrays_size, p, p_merge_size);
    // free memory;
    free(p_merge_arrays); free(p_merge_arrays_size);
}

void free_allocated_memory(int **local_a_ptrs, int *sample, int *partition_borders, int *bucket_sizes, int *result_positions, int *pivot_values) {
    free(local_a_ptrs);
    free(sample);
    free(partition_borders);
    free(bucket_sizes);
    free(result_positions);
    free(pivot_values);
}


void calc_partition_borders(int array[], int start, int end, int result[], int at, int pivot_values[], int first_pv, int last_pv)          
{
    int mid, lowerbound, upperbound, center;
    int pv;

    mid = (first_pv + last_pv) / 2;
    pv = pivot_values[mid-1];
    lowerbound = start;
    upperbound = end;
    while(lowerbound <= upperbound) {
        center = (lowerbound + upperbound) / 2;
        if(array[center] > pv) {
            upperbound = center - 1;
        } else {
            lowerbound = center + 1;
        }
    }
    result[at + mid] = lowerbound;

    if(first_pv < mid) {
        calc_partition_borders(array, start, lowerbound - 1, result, at, pivot_values, first_pv, mid - 1);
    }
    if(mid < last_pv) {
        calc_partition_borders(array, lowerbound, end, result, at, pivot_values, mid + 1, last_pv);
    }
}


double run_MPparallel_sort(char * testcase_name){
	int arr_len;
	int *arr = read_testcase_input(testcase_name, &arr_len);

	int * sorted_array=NULL;
	double time_taken[REPETITIONS];
	for(int i = 0; i < REPETITIONS; ++i){
		free(sorted_array);
		sorted_array = malloc(arr_len*sizeof(int));
		// copy array
		for(int j = 0; j < arr_len; j++) sorted_array[j] = arr[j];
		
		// begin clock
		double start = omp_get_wtime();
        
        	MP_sort_array(sorted_array, arr_len);
		
		
		// end clock
		double end = omp_get_wtime();
		time_taken[i] = end - start;
	}
	is_solution_valid(sorted_array, testcase_name);
	return get_best_time(time_taken);
}


