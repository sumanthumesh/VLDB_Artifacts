import csv
import os
import sys
import math
import subprocess

def find_cols_and_segments(filename):
    num_segments = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            if '---' in ''.join(row):
                continue
            col = row[2]
            if col in num_segments.keys():
                num_segments[col] += 1
            else:
                num_segments.update({col:1})
    for key in num_segments.keys():
        num_segments[key] = int(num_segments[key]/2)            
    return num_segments

def find_accessed_cols(filename):
    accessed_cols = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col = row[0]
            count=int(row[-1])
            accessed_cols.update({col:count})
    return accessed_cols

def gather_deltas(filename):
    delta = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col = row[2]
            seg_id = int(row[1])
            count = sum([int(c) for c in row[3:7]])
            delta.update({(col,seg_id):count})
    return delta

def get_row_widths(filename):
    row_widths = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col = row[0]
            width = float(row[1])
            row_widths.update({col:width})
    return row_widths

def get_col_size(filename):
    sizes = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col = row[0]
            size = int(row[1])
            sizes.update({col:int(size)})
    return sizes

def get_col_details(filename):
    col_details = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col_name = row[0]
            table_id = int(row[1])
            col_id = int(row[2])
            col_details.update({col_name:(table_id,col_id)})
    return col_details

def get_reverse_col_details(filename):
    reverse_col_details = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            col_name = row[0]
            table_id = int(row[1])
            col_id = int(row[2])
            reverse_col_details.update({(table_id,col_id):col_name})
    return reverse_col_details

def get_segment_trace_counts(filename,reverse_col_details):
    trace_counts = dict()
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            table_id = int(line.split(',')[0].strip())
            column_id = int(line.split(',')[1].strip())
            chunk_id = int(line.split(',')[2].split(':')[0].strip())
            count = int(line.split(',')[2].split(':')[1].strip())
            col_name = reverse_col_details[(table_id,column_id)]
            trace_counts.update({(col_name,chunk_id):count})
    return trace_counts

if __name__ == '__main__':

    base_dir = sys.argv[1]
    row_width_file = sys.argv[2]
    number_partitions = int(sys.argv[3])
    size_file = sys.argv[4]
    column_details_file = sys.argv[5]

    all_files = os.listdir(base_dir)

    col_wise_data_file = os.path.join(base_dir,'column_wise_data.csv') #column_wise_data
    delta_file = os.path.join(base_dir,'delta.csv')
    access_counter_file = ''
    for file in all_files:
        if 'access_counter' in file:
            access_counter_file = os.path.join(base_dir,file) #column_wise_data
    if access_counter_file == '':
        print(f"couldnt file access counter file")
        exit(2)
    row_widths = get_row_widths(row_width_file)

    col_segments = find_cols_and_segments(access_counter_file)
    accessed_cols = find_accessed_cols(col_wise_data_file)
    delta = gather_deltas(delta_file)
    col_sizes = get_col_size(size_file)
    reversed_col_details = get_reverse_col_details(column_details_file)

    new_partition_counts = dict()
    for col in accessed_cols.keys():
        num_seg_per_partition = math.ceil(col_segments[col]/number_partitions)
        for i in range(number_partitions):
            new_partition_name = f"{col}_{i}"
            print(f"New Parition: {new_partition_name}")
            print(f"Looking in range {i*num_seg_per_partition} {(i+1)*num_seg_per_partition}")
            new_partition_counts[new_partition_name] = 0
            for k in range(i*num_seg_per_partition,(i+1)*num_seg_per_partition):
                if (col,k) in delta:
                    new_partition_counts[new_partition_name] += delta[(col,k)]

    for key,val in col_sizes.items():
        print(f"{key},{val}")

    #Now use optimizer and evaluate mapping
    #Create a hotness file
    with open('hotness.csv','w') as file:
        for key,val in new_partition_counts.items():
            file.write(f"{key},{val}\n")

    #Create a row_width file
    with open('row_width.csv','w') as file:
        for col in new_partition_counts.keys():
            original_col_name = '_'.join(col.split('_')[:-1])
            file.write(f"{col},{row_widths[original_col_name]}\n")

    #Create new size file
    with open('size.csv','w') as file:
        for col in new_partition_counts.keys():
            original_col_name = '_'.join(col.split('_')[:-1])
            num_actual_partitions = number_partitions if math.ceil(col_segments[original_col_name]/number_partitions) > 1 else 1
            file.write(f"{col},{int(col_sizes[original_col_name]/num_actual_partitions)}\n")

    result_to_print = []

    #Invoke Optimizer
    command = f"python optimizer.py hotness.csv size.csv 0.3 100 row_width.csv hyrise"
    result = subprocess.run(command,shell=True,capture_output=True,text=True)
    lines = result.stdout.splitlines()
    result_to_print.append(lines[-1])


    #Create a segment wise trace count file
    command = f"./isolate_partition {base_dir}/mem_blk_*.txt {base_dir}/roitrace_*.csv && mv trace_count.csv trace_count_segment.csv"
    print(command)
    subprocess.run(command,shell=True)

    #Convert into partition wise trace count file
    segment_trace_counts = get_segment_trace_counts('trace_count_segment.csv',reversed_col_details)

    #Convert into a trace_count file with partitions instead of segments
    partition_trace_counts = dict()
    for col in col_sizes.keys():
        num_seg_per_partition = math.ceil(col_segments[col]/number_partitions)
        for i in range(number_partitions):
            new_partition_name = f"{col}_{i}"
            print(f"New Parition: {new_partition_name}")
            print(f"Looking in range {i*num_seg_per_partition} {(i+1)*num_seg_per_partition}")
            partition_trace_counts[new_partition_name] = 0
            for k in range(i*num_seg_per_partition,(i+1)*num_seg_per_partition):
                if (col,k) in segment_trace_counts.keys():
                    partition_trace_counts[new_partition_name] += segment_trace_counts[(col,k)]

    with open('trace_count.csv','w') as file:
        for key,val in partition_trace_counts.items():
            if val > 0:
                file.write(f"{key},{val}\n")

    #Run evaluate mapping
    command = f"python evaluate_mapping.py trace_count.csv mapping.csv {base_dir}/non_table_accesses.txt size.csv"
    result = subprocess.run(command,shell=True,capture_output=True,text=True)

    lines = result.stdout.splitlines()
    
    result_to_print.append(base_dir)
    result_to_print.append(number_partitions)
    result_to_print.append(lines[-2])
    result_to_print.append(lines[-1])

    with open('partition_result.txt','a') as file:
        file.write(f"{result_to_print}\n")

    # for key,val in segment_trace_counts.items():
    #     print(f"{key},{val}")
    

    