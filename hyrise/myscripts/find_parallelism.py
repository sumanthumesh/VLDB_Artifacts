import os
import sys
from itertools import combinations
import csv
import multiprocessing

def load_columns(column_file):
    '''
    Reads a file containing column,code
    Converts it into a dictionary
    '''
    with open(column_file) as file:
        lines = file.readlines()

    columns = dict()
    for line in lines:
        column = line.split(',')[0]
        code = line.split(',')[1][:-1]
        columns.update({column:code})
    return columns

def columns_in_query(query,columns):
    with open(query) as file:
        lines = file.readlines()
    
    found_columns = set()
    for line in lines:
        for col in columns:
            if col in line:
                found_columns.add(col)

    return found_columns

def find_all_transitions(start,end,trace_file,output_file,result_queue):
    
    line_num = 0
    transitions = []
    time = 0
    start_line_num = 0

    last_access_is_start = False

    fout = open(output_file,"w")
    writer = csv.writer(fout)

    gap_0_100 = 0
    gap_100_200 = 0

    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            line_num += 1
            ins_gap = int(line.split(' ')[2])
            time += ins_gap

            # if '0xa' not in line:
            #     continue

            if start in line:
                start_line_num = line_num
                time = 0
                last_access_is_start = True
            elif end in line:
                if last_access_is_start:
                    transitions.append((start_line_num,line_num,time))
                    if time >= 0 and time < 100:
                        gap_0_100 += 1
                    elif time >= 100 and time < 200:
                        gap_100_200 += 1
                last_access_is_start = False

    writer.writerows(transitions)

    result_queue.put((output_file,gap_0_100,gap_100_200,len(transitions)))

def print_list(l):
    for idx,ele in enumerate(l):
        print(f"{idx}:{ele}")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Expected 2 or 3 arguments, got {sys.argv[1:]}")
        exit(1)

    # File containgin list of columns
    column_file_path = 'tpch_columns.dat'

    # Load columns
    tpch_columns = load_columns(column_file_path)

    # CLI args
    query_file = sys.argv[1]
    trace_file = sys.argv[2]

    # Figure out output directory
    output_dir = "/".join(trace_file.split('/')[:-1])

    # Open query file and extract all tpch columns used in it
    query_columns = columns_in_query(query_file,tpch_columns.keys())
    print(f"Found these columns in the query")
    print(query_columns)

    # Find all combinations of these columns for parallelism
    column_pairs = list(combinations(query_columns,2))
    print(f"Found these pairs in the query")
    print_list(column_pairs)

    # parallel_workers = []
    # for idx,pair in column_pairs:
    #     process = multiprocessing.Process(target=find_all_transitions,args=(pair[0],pair[1],trace_file,f"transition_{pair[0]}_{pair[1]}.csv"))
    #     parallel_workers.append()

    # Go over each combination and find a list of transitions and record that data in .csv files
    # Gonna create a pool of processes, each process assigned a single combination pair
    # To use multiprocess.pool need to create a list of arguments 
    arguments = []
    for idx,pair in enumerate(column_pairs):
        temp = (tpch_columns[pair[0]],tpch_columns[pair[1]],trace_file,f"{output_dir}/transition_{pair[0]}_{pair[1]}.csv")
        arguments.append(temp)

    max_conccurent_processes = 4 if len(sys.argv) == 3 else int(sys.argv[3])

    print(arguments)

    results = []
    with multiprocessing.Manager() as manager:
        print(f"Launcing {max_conccurent_processes} workers")
        result_queue = manager.Queue()
        with multiprocessing.Pool(max_conccurent_processes) as pool:
            pool.starmap(find_all_transitions,[(a[0],a[1],a[2],a[3],result_queue) for a in arguments])


        # Once all transitions have been written out, aggregate statistics
        while not result_queue.empty():
            result = result_queue.get()
            results.append(result)

        print_list(results)        

    



    