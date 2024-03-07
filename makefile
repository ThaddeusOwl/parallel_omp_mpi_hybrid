SRCDIR=src
BINDIR=bin
INDATADIR=input_data
SOLDATADIR=solution_data
PYTHONENGINE=python3

all: $(SRCDIR)/mpi_sort $(SRCDIR)/seq_sort $(SRCDIR)/mp_sort $(SRCDIR)/hybrid_sort

$(SRCDIR)/mpi_sort:
	mpicc $(SRCDIR)/mpi_sort.c -o $(BINDIR)/mpi_sort

$(SRCDIR)/seq_sort:
	gcc -fopenmp $(SRCDIR)/sequential_sort.c -o $(BINDIR)/seq_sort

$(SRCDIR)/mp_sort:
	gcc -fopenmp $(SRCDIR)/Main_mp_sort.c -o $(BINDIR)/mp_sort

$(SRCDIR)/hybrid_sort:
	mpicc -fopenmp $(SRCDIR)/hybrid_sort.c -o $(BINDIR)/hybrid_sort -Wall

test: $(SRCDIR)/seq_sort $(SRCDIR)/mp_sort $(SRCDIR)/mpi_sort $(SRCDIR)/hybrid_sort
	bin/seq_sort random_1048576
	bin/mp_sort 8 random_1048576
	mpiexec -n 8 bin/mpi_sort random_1048576
	mpiexec -n 4 bin/hybrid_sort 2 random_1048576
	

clean:
	rm -f $(BINDIR)/*

clean-data:
	rm -f $(INDATADIR)/*
	rm -f $(SOLDATADIR)/*

data: clean-data
	$(PYTHONENGINE) src/generate_data.py
