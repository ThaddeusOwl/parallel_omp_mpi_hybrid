#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#define REPETITIONS 20


// function to print array elements
void printArray(int array[], int size) {
  for (int i = 0; i < size; ++i) {
    printf("%d  ", array[i]);
  }
  printf("\n");
}

int compare(const void* num1, const void* num2) {
	int a = *(int *) num1;
	int b = *(int *) num2;
	if (a > b) return 1;
	if (a < b) return -1;
	return 0;
}

void quickSort(int *arr, int low, int high) {
	qsort(arr + low, high - low, sizeof(int), compare);

}

int* read_array_from_file(char* filename, int *array_size){
	// open file
	FILE *file = fopen(filename, "r");
	// make sure file was opened successfully
	if (file == NULL){
      		perror("Error while opening the file.\n");
      		exit(EXIT_FAILURE);
   	}
	// read in array size
	fscanf(file, "%d", array_size);
	// read in the rest of the array
	int * arr = malloc((*array_size)*sizeof(int));
	for(int i = 0; i < (*array_size); ++i){
		fscanf(file, "%d", &arr[i]);
	}
	// close file
	fclose(file);

	return arr;

}

int *read_testcase_input(char * testcase_name, int *array_size){
	char input_file[256]; 
	// e.g. 
	// if testcase_name = 'test5'
	// then input_file = "input_data/test5.in"
	
	strcpy(input_file, "input_data/");
	strcat(input_file, testcase_name);
	strcat(input_file, ".in");
	
	int * arr = read_array_from_file(input_file, array_size);
	return arr;
}

int *read_testcase_sol(char * testcase_name, int *array_size){
	char sol_file[256]; 
	// e.g. 
	// if testcase_name = 'test5'
	// then sol_file = "solution_data/test5.in"
		
	strcpy(sol_file, "solution_data/");
	strcat(sol_file, testcase_name);
	strcat(sol_file, ".out");
	int * arr = read_array_from_file(sol_file, array_size);
	return arr;
}

void is_solution_valid(int *sorted_array, char * testcase_name){
	int sol_array_len;
	int * sol_array = read_testcase_sol(testcase_name, &sol_array_len);
	short is_valid = 1;
	for(int i = 0; i < sol_array_len; ++i){
		if(sorted_array[i] != sol_array[i]){
			is_valid = 0;
			printf("Expected %d but found %d\n", sol_array[i], sorted_array[i]);
			break;
		}
	}

	if (!is_valid){
		perror("Sorted array does not match memo solution!\n");
	}
	free(sol_array);
}

double get_best_time(double *time_taken){
	double best_time = time_taken[0];
	for(int i = 0; i < REPETITIONS; ++i){
		if (time_taken[i] < best_time) best_time = time_taken[i];
	}
	return best_time;
}
