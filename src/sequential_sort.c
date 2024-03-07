#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <omp.h>
#include "common.c"

struct timeval tv;

double run_sequential_sort(char * testcase_name){
	int arr_len;
	int *arr = read_testcase_input(testcase_name, &arr_len);
	
	int *sorted_array = malloc(arr_len*sizeof(int));
	double time_taken[REPETITIONS];
	for(int i = 0; i < REPETITIONS; ++i){
		// copy the array
		for(int j = 0; j < arr_len; ++j) sorted_array[j] = arr[j];
		// begin clock
		double start = omp_get_wtime();
		// sort
		quickSort(sorted_array, 0, arr_len);
		
		double end = omp_get_wtime();
		time_taken[i] = end - start;
	}

	is_solution_valid(sorted_array, testcase_name);
	return get_best_time(time_taken);

}

int main(int argc, char* argv[]){;
	printf("Seq %s %f\n", argv[1], run_sequential_sort(argv[1]));
}
