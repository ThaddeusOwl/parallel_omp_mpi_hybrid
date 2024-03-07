#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "common.c"

#define master_rank 0

int dihedral_pair(int ind, int iter, int size){
	int pair_ind = size - 1 - (ind + iter);
	if (pair_ind < 0) pair_ind += size;
	return pair_ind;
}
// phase 3 of PSRS
// This is probably unnecessarily complicated
int ** distribute_sorted_segments(int **sorted_seg_size_pointer, int* arr_seg, int seg_size, int* pivot_index){
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	int **sorted_seg = malloc(size*sizeof(int *));
	int *sorted_seg_size = malloc(size*sizeof(int));
	*(sorted_seg_size_pointer) = sorted_seg_size;

	// each process needs to send i messages
	for(int i = 0; i < size; ++i){
		// At each time step i, each process gets paired up with another process. Hopefully this is efficient.
		int paired_rank = dihedral_pair(rank, i, size);
		
		// subseg_start and subseg_end define an inclusive-inclusive interval
		int subseg_start, subseg_end;
		subseg_start = 0; 
		if (paired_rank > 0) subseg_start = pivot_index[paired_rank - 1] + 1;
		subseg_end = seg_size - 1;
		if (paired_rank != size - 1) subseg_end = pivot_index[paired_rank];

		int subseg_size = subseg_end - subseg_start + 1;
		if (rank == paired_rank){
			sorted_seg_size[paired_rank] = subseg_size;
			sorted_seg[paired_rank] = malloc(subseg_size*sizeof(int));
			for(int j = 0; j < subseg_size; ++j) 
				sorted_seg[paired_rank][j] = arr_seg[subseg_start + j];
			goto equals;
		}
		// min_rank = min(rank, paired_rank). This is sorted out later in the code.
		int min_rank = rank;
		if (rank < paired_rank){
			// if rank < paired_rank then we send first and receive second. I don't know if this matters. We should test.
			MPI_Ssend(&subseg_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
			MPI_Recv(&sorted_seg_size[paired_rank], 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		}
		else if (rank > paired_rank){
			min_rank = paired_rank;
			MPI_Recv(&sorted_seg_size[paired_rank], 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Ssend(&subseg_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
		}
		sorted_seg[paired_rank] = malloc(sorted_seg_size[paired_rank]*sizeof(int));
		if (rank < paired_rank){
			MPI_Ssend(&arr_seg[subseg_start], subseg_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
			MPI_Recv(sorted_seg[paired_rank], sorted_seg_size[paired_rank], MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		else if (rank > paired_rank){
			MPI_Recv(sorted_seg[paired_rank], sorted_seg_size[paired_rank], MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Ssend(&arr_seg[subseg_start], subseg_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
		}
	equals:
		MPI_Barrier(MPI_COMM_WORLD);
	}
	return sorted_seg;

}
// given p sorted arrays. Perform a merge sort merging all elements to a single array
// This code could be reused for the OMP version
int * p_merge(int** arrays, int* array_size, int num_arrays, int *final_size){
	
	int *counter = malloc(num_arrays*sizeof(int));
	// compute the final size of the merged array
	*final_size = 0;
	for(int i = 0; i < num_arrays; ++i){
		counter[i] = 0;
		*final_size += array_size[i];
	}
	
	int *merged_array = malloc((*final_size)*sizeof(int));
	// maybe this is faster with more cores?
	// Technically I we should be using a heap for large number of threads
	if (1){
		int c = 0;
		for(int i = 0; i < num_arrays; ++i){
			for(int j = 0; j < array_size[i]; ++j){
				merged_array[c++] = arrays[i][j];
			}
		}
		qsort(merged_array, *final_size, sizeof(int), compare);
		free(counter);
		return merged_array;
	}
	
	// p merge sort
	for(int i = 0; i < (*final_size); ++i){
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
	free(counter);
	return merged_array;
}

int * MPI_sort_array(int * arr, int arr_len){
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
	// broadcast the array size so each process can allocate memory
	MPI_Bcast(&arr_len, 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	int seg_size = arr_len/size;
	int *arr_seg = malloc(seg_size*sizeof(int));
	
	// send array to each process
	MPI_Scatter(arr, seg_size, MPI_INTEGER, arr_seg, seg_size, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	
	// sort partition
	quickSort(arr_seg, 0, seg_size);
	
	
	// gets regular sample
	int *regular_sample = malloc((size - 1)*sizeof(int));
	for(int i = 1 ; i < size; ++i) regular_sample[i-1] = arr_seg[seg_size/size*i];
	
	// master process needs buffer to store regular samples
	int *regular_samples = NULL;
	if (rank == master_rank) regular_samples = malloc(size*(size - 1)*sizeof(int));
	
	// gather regular samples to master process
	MPI_Gather(regular_sample, size - 1, MPI_INTEGER, regular_samples, size-1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	free(regular_sample);

	// master process sorts regular samples
	if (rank == master_rank) quickSort(regular_samples, 0, size*(size - 1));
	
	// all processes create buffer for pivots. Master rank computes pivots
	int *pivots = malloc((size - 1)*sizeof(int));
	if(rank == master_rank){
		for(int i = 0; i < size - 1; i++) pivots[i] = regular_samples[i*size + size/2];
	}
	free(regular_samples);

	// broadcast pivots to all processes
	MPI_Bcast(pivots, size - 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	
	// each process partitions their segment by the pivots using a binary search
	int *pivot_index = malloc((size - 1)*sizeof(int));
	for(int i = 0; i < size - 1; ++i){
		int l = -1; int r = seg_size;
		while(l + 1 < r){
			int mid = (l + r)/2;
			if (arr_seg[mid] <= pivots[i]) l = mid;
			else r = mid;
		}
		pivot_index[i] = l; 
	}
	free(pivots);
	
	// PSRS phase 3: distributing pivot divided segments to appropriate process
	int *sorted_seg_size;

	int **sorted_seg = distribute_sorted_segments(&sorted_seg_size, arr_seg, seg_size, pivot_index);

        free(arr_seg); free(pivot_index);

        MPI_Barrier(MPI_COMM_WORLD);
        // do p_merge sort on each sorted segment
	int p_merged_size;
	int * p_merged = p_merge(sorted_seg, sorted_seg_size, size, &p_merged_size);



        // free sorted_seg and sorted_seg_size
	for(int i = 0 ; i < size; ++i) free(sorted_seg[i]);
	free(sorted_seg); free(sorted_seg_size);


	// now all the sorted segments need to be gathered
	//
	// gather the number of elements in each processes sorted segments
	int * receive_counts = NULL;
	if (rank == master_rank) receive_counts = malloc(size*sizeof(int));
	MPI_Gather(&p_merged_size, 1, MPI_INTEGER, receive_counts, 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);

	// preprare buffers to gather all the segments
	int * stride = NULL;
	int * sorted_array = NULL;
	if (rank == master_rank){
		sorted_array = malloc(arr_len*sizeof(int));
		stride = malloc(size*sizeof(int));
		stride[0] = 0;
		for(int i = 1; i < size; ++i) stride[i] = receive_counts[i - 1] + stride[i - 1];
	}
	
	// gather all the segments
	MPI_Gatherv(p_merged, p_merged_size, MPI_INTEGER, sorted_array, receive_counts, stride, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	// free memory
	free(p_merged); free(stride); free(receive_counts);
	return sorted_array;
}

double run_MPIparallel_sort(char * testcase_name){
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int arr_len;
	int *arr=NULL;
	// read in input from file
	if (rank == master_rank){
		arr = read_testcase_input(testcase_name, &arr_len);
	}	


	//sort array
	int *sorted_array = NULL;
	double time_taken[REPETITIONS];
	for(int i = 0; i < REPETITIONS; ++i){
		free(sorted_array);
		// begin clock
		MPI_Barrier(MPI_COMM_WORLD); 
		double start = MPI_Wtime();
		
		sorted_array =  MPI_sort_array(arr, arr_len);
		
		// end clock
		MPI_Barrier(MPI_COMM_WORLD); 
		double end = MPI_Wtime();

		time_taken[i] = end - start;
	}
	
	if (rank == master_rank){
		is_solution_valid(sorted_array, testcase_name);
	}
	free(arr);
    free(sorted_array);
	return get_best_time(time_taken);
	

}

int main(int argc, char* argv[]){
	MPI_Init(&argc, &argv);

	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
	double time = run_MPIparallel_sort(argv[1]);
	
	if (rank == master_rank) printf("MPI p%d %s %f\n", size, argv[1], time);
	
	MPI_Finalize();
	return 0;
}
