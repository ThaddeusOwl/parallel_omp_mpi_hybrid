#include "mp_sort.c"
#include <omp.h>

int main(int argc, char ** argv){
   	int num_threads = atoi(argv[1]);
   	omp_set_num_threads(num_threads);
	double time = run_MPparallel_sort(argv[2]);
	printf("OMP t%d %s %f\n", num_threads, argv[2], time);	

}
