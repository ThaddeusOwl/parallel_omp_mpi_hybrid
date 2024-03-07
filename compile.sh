#!/bin/sh
#BATCH --acount=icts
#SBATCH --partition=curie
#SBATCH --time=00:30
#SBATCH --nodes=1 --ntasks=1 --ntasks-per-node=1 --cpus-per-task=1
#SBATCH --job-name="CmonMakefileMyDay"
cd /home/hpc19/Assignment3

module load compilers/gcc11.2
module load mpi/openmpi-3.1.2

make clean
make

