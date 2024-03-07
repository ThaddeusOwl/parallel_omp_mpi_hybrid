#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "mp_sort.c"

#define master_rank 0

// unfortunately I was swapping between my PC and laptop coding this. And my vim is setup to use different number of spaces to tabs. So this code is horrible.

int dihedral_pair(int ind, int iter, int size){
	int pair_ind = size - 1 - (ind + iter);
	if (pair_ind < 0) pair_ind += size;
	return pair_ind;
}
// This distributes the sub-subarrays to the correct process
// Do not look at this code
int ***distribute_sorted_segments(int ***sorted_segs_size_ptr, int **process_thread_segs, int**pivot_indexes, int thread_seg_size){
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	int process_threads = omp_get_num_threads();
	int num_threads = size*process_threads;

	int ***process_sorted_segs = malloc(process_threads*sizeof(int**));
	int **sorted_seg_size = malloc(process_threads*sizeof(int*));
    for(int i = 0; i < process_threads; ++i){ 
        process_sorted_segs[i] = malloc(num_threads*sizeof(int*));
        sorted_seg_size[i] = malloc(num_threads*sizeof(int));
    }
	*sorted_segs_size_ptr = sorted_seg_size;

	for(int i = 0; i < size; ++i){
		// At each time step i, each process gets paired up with another process. Hopefully this is efficient.
		
        int compressed_size;
        int inner_seg_start[process_threads][process_threads];
        int inner_seg_end[process_threads][process_threads];
        int *compressed_data = NULL;
        
    	int paired_rank = dihedral_pair(rank, i, size);
	int paired_inner_seg_start[process_threads][process_threads];
	int paired_inner_seg_end[process_threads][process_threads];
	int section_start = process_threads*paired_rank;
	int section_end = process_threads*(paired_rank + 1);
	int paired_compressed_size = 0;
	// compute what each "thread" needs to be send to. This is all done by the master thread. this could be parallelised but it's unneccessary.
        for(int section = section_start; section < section_end; ++section){
		    int section_ind = section - section_start;
			for(int thread = 0; thread < process_threads; ++thread){
			    int subseg_start, subseg_end;
			    subseg_start = 0; 
			    if (section > 0) subseg_start = pivot_indexes[thread][section - 1] + 1;
			    subseg_end = thread_seg_size - 1;
        	    if (section != num_threads - 1) subseg_end = pivot_indexes[thread][section];

			    int subseg_size = subseg_end - subseg_start + 1;

                paired_inner_seg_start[section_ind][thread] = subseg_start;
			    paired_inner_seg_end[section_ind][thread] = subseg_end;
			    paired_compressed_size += subseg_size;
		    }
	    }
    
		int *paired_compressed_data = malloc(paired_compressed_size*sizeof(int));
		int c = 0;
		for(int section = section_start; section < section_end; ++section){
			int section_ind = section - section_start;
			for(int thread = 0; thread < process_threads; ++thread){
				for(int j = paired_inner_seg_start[section_ind][thread]; j <= paired_inner_seg_end[section_ind][thread]; ++j){
					paired_compressed_data[c++] = process_thread_segs[thread][j]; // serialisation
				}
			}
		}
		int min_rank = rank;
	// send info to reconstruct 2D array
       	int array_dim = process_threads*process_threads;
		if (rank < paired_rank){
			MPI_Send(&paired_compressed_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
			MPI_Recv(&compressed_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            	MPI_Send(&paired_inner_seg_start[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
			MPI_Recv(&inner_seg_start[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            	MPI_Send(&paired_inner_seg_end[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
			MPI_Recv(&inner_seg_end[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		}
		if(paired_rank < rank){
			min_rank = paired_rank;
			MPI_Recv(&compressed_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Send(&paired_compressed_size, 1, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);

			MPI_Recv(&inner_seg_start[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
           	 	MPI_Send(&paired_inner_seg_start[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
            
			MPI_Recv(&inner_seg_end[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            		MPI_Send(&paired_inner_seg_end[0][0], array_dim, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
		}
		if (rank == paired_rank){
         	   compressed_size = paired_compressed_size;
         	   memcpy(&inner_seg_start, &paired_inner_seg_start, array_dim*sizeof(int));
         	   memcpy(&inner_seg_end, &paired_inner_seg_end, array_dim*sizeof(int));
         	   compressed_data = paired_compressed_data;

		}
        if (rank != paired_rank) compressed_data = malloc(compressed_size*sizeof(int)); // array buffer to receive serialised array
	// send Serialised array
        if(rank < paired_rank){
            MPI_Send(paired_compressed_data, paired_compressed_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
            MPI_Recv(compressed_data, compressed_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if(paired_rank < rank){
            MPI_Recv(compressed_data, compressed_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(paired_compressed_data, paired_compressed_size, MPI_INTEGER, paired_rank, min_rank, MPI_COMM_WORLD);
        }
	// reconstruct 2D array
        c = 0;
        for(int thread = 0; thread < process_threads; ++thread){
        	for(int section = section_start; section < section_end; ++section){
            	int section_ind = section - section_start;
                sorted_seg_size[thread][section] = inner_seg_end[thread][section_ind] - inner_seg_start[thread][section_ind] + 1;
                process_sorted_segs[thread][section] = malloc(sorted_seg_size[thread][section]*sizeof(int));
                for(int j = 0; j < sorted_seg_size[thread][section]; ++j){
                	process_sorted_segs[thread][section][j] = compressed_data[c++];
                }
            }
        }
        free(paired_compressed_data);
        if (rank != paired_rank) free(compressed_data);
	MPI_Barrier(MPI_COMM_WORLD); // This barrier is almost certainly not needed. It's a relic from bugfixing.
	}
	
	return process_sorted_segs;
}

void p_merge(int *merged_array, int** arrays, int* array_size, int num_arrays, int p_merge_size, int shift){
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
    	merged_array[i + shift] = smallest;
	}
}

int * MPI_sort_array(int * arr, int arr_len){
	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
	int process_threads;
	int num_threads;
	int seg_size; // size of the segment that each process gets
	int thread_seg_size; // size of the segment each thread gets
	int *arr_seg = NULL; // the array that each process gets
	int **processes_thread_segs;
	int *processes_regular_samples = NULL;
	int *regular_samples = NULL;
	int *pivots = NULL;
	int **pivot_indexes = NULL;
	int **sorted_segs_size = NULL;
	int ***process_sorted_segs = NULL;
	int *p_merged = NULL;
	int *p_merged_size = NULL;
	int *p_merged_shift = NULL;
	int process_p_merged_size;
	#pragma omp parallel
	{
	int thread_num = omp_get_thread_num();
	#pragma omp master
	{
		process_threads = omp_get_num_threads();
		num_threads = size*process_threads;

		processes_thread_segs = malloc(process_threads*sizeof(int*));
		// step 1: Process level
		// broadcast the array size so each process can allocate memory
		MPI_Bcast(&arr_len, 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
		seg_size = arr_len/size;
		arr_seg = malloc(seg_size*sizeof(int));

		processes_regular_samples = malloc((num_threads - 1)*process_threads*sizeof(int));
	
		// send array to each process
		MPI_Scatter(arr, seg_size, MPI_INTEGER, arr_seg, seg_size, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
	}
	#pragma omp barrier
	thread_seg_size = seg_size/process_threads;

	int start_ind = thread_num*thread_seg_size;
	// step 1: Thread level
	int * thread_arr_seg = create_local_copy(arr_seg, start_ind, thread_seg_size, processes_thread_segs ,thread_num);
	
	// step 2
	quickSort(thread_arr_seg, 0, thread_seg_size);
	
	// step 3: thread level
	// gets regular samples to the process
	int thread_shift = thread_num*(num_threads - 1);
	for(int i = 1 ; i < num_threads; ++i) 
		processes_regular_samples[i-1 + thread_shift] = thread_arr_seg[thread_seg_size/num_threads*i];

	#pragma omp barrier
	// step 3: process level
	#pragma omp master
	{
		free(arr_seg);
		// number of regular samples per process
		int nrspp = process_threads*(num_threads - 1);
		// master process needs buffer to store regular samples
		if (rank==master_rank) regular_samples = malloc(size*nrspp*sizeof(int));

		// gather regular samples to master process
		MPI_Gather(processes_regular_samples, nrspp, MPI_INTEGER, regular_samples, nrspp, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
		if (rank == master_rank) quickSort(regular_samples, 0, nrspp*size);
		
		// all processes create buffer for pivots. Master rank computes pivots
		pivots = malloc((num_threads - 1)*sizeof(int));
		if(rank == master_rank){
			for(int i = 0; i < num_threads - 1; i++) 
				pivots[i] = regular_samples[i*num_threads + num_threads/2];
		}
		

		// broadcast pivots to all processes
		MPI_Bcast(pivots, num_threads - 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);

		pivot_indexes = malloc(process_threads*sizeof(int*));
	}
	#pragma omp barrier
	// step 4
	// each thread partitions their segment by the pivots using a binary search
	int * pivot_index = malloc((num_threads - 1)*sizeof(int));
	for(int i = 0; i < num_threads - 1; ++i){
		int l = -1; int r = thread_seg_size;
		while (l + 1 < r){
			int mid = (l + r)/2;
			if (thread_arr_seg[mid] <= pivots[i]) l = mid;
			else r = mid;
		}
		pivot_index[i] = l;
	}
	pivot_indexes[thread_num] = pivot_index;
    	#pragma omp barrier
	// step 5
	#pragma omp master
	{

        	free(pivots);
		process_sorted_segs = distribute_sorted_segments(&sorted_segs_size, processes_thread_segs, pivot_indexes, thread_seg_size); // this is all of step 5
        	// lots of setup for step 6
		process_p_merged_size = 0; 
		p_merged_size = malloc(process_threads*sizeof(int));
		p_merged_shift = malloc(process_threads*sizeof(int));
		p_merged_shift[0] = 0;
		for(int thread = 0; thread < process_threads; ++thread){
            		free(pivot_indexes[thread]);
            		free(processes_thread_segs[thread]);
			p_merged_size[thread] = 0;
			for(int section = 0; section < num_threads; ++ section) p_merged_size[thread] += sorted_segs_size[thread][section];
			process_p_merged_size += p_merged_size[thread];
			if (thread != 0) p_merged_shift[thread] = p_merged_shift[thread - 1] + p_merged_size[thread - 1];
		}
		p_merged = malloc(process_p_merged_size*sizeof(int));
        	free(pivot_indexes);
        	free(processes_thread_segs);
	}
	#pragma omp barrier
	// step 6: p-merge sort
	p_merge(p_merged, process_sorted_segs[thread_num], sorted_segs_size[thread_num], num_threads, p_merged_size[thread_num], p_merged_shift[thread_num]);
	}
    for(int thread = 0; thread < process_threads; ++thread){
        for(int section = 0; section< num_threads; ++section){
            free(process_sorted_segs[thread][section]);
        }
        free(process_sorted_segs[thread]);
        free(sorted_segs_size[thread]);
    }
    	free(process_sorted_segs); free(sorted_segs_size);
	free(p_merged_size); free(p_merged_shift);

	// step 6: concatenating the lists
	
	// now all the sorted segments need to be gathered
	//
	// gather the number of elements in each processes sorted segments
	int * receive_counts = NULL;
	if (rank == master_rank) receive_counts = malloc(num_threads*sizeof(int));
	MPI_Gather(&process_p_merged_size, 1, MPI_INTEGER, receive_counts, 1, MPI_INTEGER, master_rank, MPI_COMM_WORLD);

	// preprare buffers to gather all the segments
	int * stride = NULL;
	int * sorted_array = NULL;
	if (rank == master_rank){
		sorted_array = malloc(arr_len*sizeof(int));
		stride = malloc(num_threads*sizeof(int));
		stride[0] = 0;
		for(int i = 1; i < size; ++i) stride[i] = receive_counts[i - 1] + stride[i - 1];
	}
	
	// gather all the segments
	MPI_Gatherv(p_merged, process_p_merged_size, MPI_INTEGER, sorted_array, receive_counts, stride, MPI_INTEGER, master_rank, MPI_COMM_WORLD);
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
	free(arr);
	if (rank == master_rank){
	    is_solution_valid(sorted_array, testcase_name);
	}
    	//free(sorted_array);
	return get_best_time(time_taken);
	

}

int main(int argc, char* argv[]){
	int provided_support;
   	int process_threads = atoi(argv[1]);
   	omp_set_num_threads(process_threads);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided_support);

	int rank; MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size; MPI_Comm_size(MPI_COMM_WORLD, &size);
    	double time = run_MPIparallel_sort(argv[2]);
	if (rank == master_rank) printf("Hyb p%d t%d %s %f\n", size, process_threads, argv[2], time);
	
	MPI_Finalize();
	return 0;
}
