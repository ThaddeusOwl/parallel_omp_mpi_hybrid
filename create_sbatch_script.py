template = (
'''#!/bin/bash
#SBATCH --account icts
#SBATCH --partition=curie
#SBATCH --job-name=sort_arrays
#SBATCH --time=00:25:00
#SBATCH --nodes={NODES}
#SBATCH --ntasks-per-node={PROCS_PER_NODE}
#SBATCH --cpus-per-task={THREADS_PER_PROC}

module load compilers/gcc11.2
module load mpi/openmpi-3.1.2

cd /home/hpc19/Assignment3

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

'''
)

types = ['random', 'increasing', 'decreasing']

OUTPUT_FILE = 'weak_data.txt'
COMMAND_SUFFIX = ' >> ' + OUTPUT_FILE

def mpi(num_processes, file, size):
    global num_procs_requested
    command_template = (
        'mpiexec -n {num_processes} bin/mpi_sort {filename}'
        )
    filename = file + '_' + str(size)

    command_str = command_template.format(
        num_processes=num_processes,
        filename=filename
        )
    if num_processes > num_procs_requested:
        raise Exception(f"""You have only requested {num_procs_requested} processes. You cannot use {num_processes} in the command
{command_str + COMMAND_SUFFIX}""")
    return command_str + COMMAND_SUFFIX

def seq(file, size):
    command_template = (
        './bin/seq_sort {filename}'
        )
    filename = file + '_' + str(size)
    command_str = command_template.format(
        filename=filename
        )
    return command_str + COMMAND_SUFFIX

def mp(num_threads, file, size):
    command_template = (
        './bin/mp_sort {num_threads} {filename}'
        )
    filename = file + '_' + str(size)
    command_str = command_template.format(
        num_threads=num_threads,
        filename=filename
        )
    return command_str + COMMAND_SUFFIX

def hybrid(num_processes, num_threads, file, size):
    global num_procs_requested
    command_template = (
        'mpiexec -n {num_processes} bin/hybrid_sort {num_threads} {filename}'
        )
    filename = file + '_' + str(size)

    command_str = command_template.format(
        num_processes=num_processes,
        num_threads=num_threads,
        filename=filename
        )
    if num_processes > num_procs_requested:
        raise Exception(f"""You have only requested {num_procs_requested} processes. You cannot use {num_processes} in the command
{command_str + COMMAND_SUFFIX}""")
    return command_str + COMMAND_SUFFIX

# numcpus = nodes*proces_pernode*threads_per_proc, probably

NODES=2
PROCS_PER_NODE=4
THREADS_PER_PROC=4

num_procs_requested = NODES*PROCS_PER_NODE

filled_template = template.format(
    NODES=NODES, # max 4
    PROCS_PER_NODE=PROCS_PER_NODE,
    THREADS_PER_PROC=THREADS_PER_PROC # max 32 cpus
    )

run_commands = []

proc ={
    4: (2, 2),
    8: (4, 2),
    16: (4, 4),
    32: (8, 4),
    }

sizes = [1243040, 2376352, 4551232, 8731648, 16777216]

for i in range(1, len(sizes)):
    size = sizes[i]
    p, t = proc[2**(i + 1)]
    run_commands.append(hybrid(p, t, 'random', size))
    #run_commands.append(mp(2**(i + 1), 'random', size))
    #run_commands.append(mpi(2**(i + 1), 'random', size))
    #run_commands.append(seq('random', size))

print(filled_template + '\n'.join(run_commands))
