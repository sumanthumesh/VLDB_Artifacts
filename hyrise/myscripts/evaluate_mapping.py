import os
import sys
import csv

cxl_latency = 210
dam_latency = 30

all_tpch_cols = [line.strip() for line in open("all_tpch_columns.dat").readlines()]

def load_mapping(filename):
    with open(filename) as file:
        lines = file.readlines()

    mapping = {line.split(',')[0]:True if line.split(',')[1].strip() == '1' else False for line in lines}
    
    return mapping

def load_trace_counts(filename):
    with open(filename) as file:
        lines = file.readlines()

    trace_counts = {line.split(',')[0]:int(line.split(',')[1].strip()) for line in lines}    

    return trace_counts

def get_amat(trace_counts,mapping,non_table_count):
    
    total_time = 0
    for col in trace_counts.keys():
        if mapping[col]:
            total_time += trace_counts[col] * cxl_latency
        else:
            total_time += trace_counts[col] * dam_latency
    total_time += non_table_count * dam_latency

    # print(f"total time {total_time}")
    # print(f"table accesses {sum(trace_counts.values())}")
    # print(f"non table {non_table_count}")

    amat = total_time / (sum(trace_counts.values()) + non_table_count)
            
    return amat

def load_sizes(filename):
    with open(filename) as file:
        reader = csv.reader(file)
        sizes = dict()
        for row in reader:
            sizes.update({row[0]:int(row[1])})
        return sizes


if __name__ == '__main__':

    trace_file = sys.argv[1]
    mapping_file = sys.argv[2]
    non_table_file = sys.argv[3]
    size_file = sys.argv[4]

    non_table_count = int(open(non_table_file).readlines()[0].strip())
    # print(non_table_count)

    trace_counts = load_trace_counts(trace_file)
    
    #Size of DAM cols
    col_sizes = load_sizes(size_file)

    mapping = {col:True for col in col_sizes.keys()}

    amat_before = get_amat(trace_counts,mapping,non_table_count)

    mapping = load_mapping(mapping_file)

    amat_after = get_amat(trace_counts,mapping,non_table_count)

    dam_cols = []
    for key,val in mapping.items():
        if val == False:
            dam_cols.append(key)
    print(dam_cols)

    dam_size = 0
    for col in dam_cols:
        dam_size += col_sizes[col]

    all_in_dam = True

    for col,count in trace_counts.items():
        if count > 0 and col not in dam_cols:
            all_in_dam = False
    
    if all_in_dam:
        print(f"All columns fit in DAM")

    print(f"DAM OCCUPANCY: {dam_size/2**30}")
    print(f"{amat_before},{amat_after}")
