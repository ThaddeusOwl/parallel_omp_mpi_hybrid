import random
import os
import math

def write_file(filepath, array):

    # if file exists overwrite, otherwise create the file and write
    mode = 'w' if os.path.isfile(filepath) else 'x'

    with open(filepath, mode) as file:
        file.write(f'{len(array)}\n')
        str_array = [str(i) for i in array]
        file.write(f'{" ".join(str_array)}')


def generate_increasing_order(n):
    # return format: input, solution
    return range(n), range(n)

def generate_decreasing_order(n):
    # return format: input, solution
    decreasing_list = list(range(n)[::-1])
    return decreasing_list, range(n) 

def generate_random_order(n):
    # return format: input, solution
    s = 0
    sorted_list = [None]*n
    for i in range(n):
        s += random.randint(1, 10)
        sorted_list[i] = s

    shuffled_list = list(sorted_list)
    random.shuffle(shuffled_list)

    return shuffled_list, sorted_list

def weak_scaling_sizes():
    w = 4**12*math.log(4**12, 2)//32

    Ns = []
    for i in range(0, 6):
        l = 0
        target = w*(2**i)
        r = target/4**i
        # find an N such that 4**i divides N and (N log N)/2**i == w approximately
        while l + 1 < r:
            mid = (l + r)//2
            N_tmp = mid*4**i
            if (N_tmp*math.log(N_tmp, 2) < target):
                l = mid
            else:
                r = mid
        Ns.append(int(r*4**i))
    return Ns

if __name__ == '__main__':
    # dir variables
    project_dir = os.getcwd()
    input_dir = os.path.join(project_dir, 'input_data')
    sol_dir = os.path.join('solution_data')

    # seed random
    random.seed(0)
    a, b = generate_random_order(10)

    array_sizes = [4**i for i in range(3, 4)] # from 32 to 1 billion
    array_sizes = array_sizes + weak_scaling_sizes() # include weak_scaling_experiments
    # ways of generating data
    generation_types = {
        'increasing' : generate_increasing_order,
        'decreasing' : generate_decreasing_order,
        'random' : generate_random_order,  # Only random was used for final benchmarking
        }


    for array_size in array_sizes:
        for gen_type_name in generation_types:
            # generate testcase
            generator = generation_types[gen_type_name]
            input_arr, sol_array = generator(array_size)

            # name of testcase. both input and sol file
            testcase_name = f'{gen_type_name}_{array_size}'

            # write input file
            input_filepath = os.path.join(input_dir, testcase_name + '.in')
            write_file(input_filepath, input_arr)

            # write output file
            sol_filepath = os.path.join(sol_dir, testcase_name + '.out')
            write_file(sol_filepath, sol_array)

            
            
        
