Instructions:

To compile code run

```
make all
```

To clean compiled code run

```
make clean
```

To generate test data run (Note this can take up to 5 minutes)

```
make data
```

To delete generated test data run

```
make clean-data
```

To run the sequential sort run

```
./bin/seq_sort <input_file>
```
e.g
```
./bin/seq_sort random_1048576
```

To run the mpi sort run

```
mpiexec -n <num_processes> bin/mpi_sort <input_file>
```

To run the mp sort run

```
./bin/mp_sort <num_threads> <input_file>
```

To run the hybrid sort run

```
mpiexec -n <num_processes> bin/hybrid_sort <num_threads> <input_file>
```

You can run

```
make test
```
to compile the code and run the code on an array of size 1 million. This will use 
* 8 threads for OpenMP
* 8 processes for MPI
* 4 processes and 2 threads for hybrid

You can use the create_sbatch_script.py to create sbatch scripts to run the code on the hpc.

On the HPC:

Make sure to edit the .sh files for your HPC environment. That is change the user name and path.

To make data do

```
sbatch make_data.sh
```

To compile code do

```
sbatch compile.sh
```