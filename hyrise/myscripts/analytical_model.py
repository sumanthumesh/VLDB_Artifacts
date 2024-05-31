import sys
import os
import numpy as np
import csv
import random
import subprocess
import matplotlib.pyplot as plt


cxl_lat = 270
dam_lat = 45

def parse_csv_file(filename):
    #Assume first row a nd first column are labels
    with open(filename) as file:
        reader = csv.reader(file)
        header = next(reader)
        col_indices = {header[i+1]:i for i in range(len(header)-1)}
        row_indices = dict()
        list_row = []
        for idx,row in enumerate(reader):
            row_indices.update({row[0]:idx})
            list_row.append([int(x) for x in row[1:]])
    # normalized_list = []
    # for row in list_row:
    #     normalized_list.append([x/sum(row) for x in row])
    # print(row_indices)
    # print(col_indices)
    # return np.array(normalized_list)
    return (row_indices,col_indices,np.array(list_row))

class DataTable:
    def __init__(self,args):
        self.row_indices = args[0]
        self.col_indices = args[1]
        self.data = args[2]

    def access_row(self,row_id):
        return self.data[self.row_indices[row_id]]
    
    def access_col(self,col_id):
        return self.data[:,self.col_indices[col_id]]
    
    def access_cell(self,row_id,col_id):
        return self.data[self.row_indices[row_id],self.col_indices[col_id]]

def load_non_table_accesses(filename):
    non_table_accesses = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            non_table_accesses.update({row[0]:int(row[2])})
    return non_table_accesses

def load_mapping(filename):
    mapping = dict()

    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            col_name = line.split(',')[0]
            val = True if line.split(',')[1].strip() == "1" else False
            mapping.update({col_name:val}) 
    return mapping

def apply_mapping_get_amat(query_list,mapping,trace_counts,non_table_accesses):
    if len(mapping)!=trace_counts.data.shape[1]:
        print(f"Mismatch {len(mapping)} vs {len(trace_counts.data.shape[1])}")
        exit(1)

    total_lat = 0
    total_accesses = 0

    for query in query_list:
        print(f"query {query}")
        for col_name in trace_counts.col_indices.keys():
            if mapping[col_name] == True:
                total_lat += cxl_lat*trace_counts.access_cell(query,col_name)
            else:
                total_lat += dam_lat*trace_counts.access_cell(query,col_name)
        total_lat+=dam_lat*non_table_accesses[query]
        
        total_accesses += sum(trace_counts.access_row(query)) + non_table_accesses[query]
    
    print(f"total_accesses {total_accesses}")

    return total_lat/total_accesses
    
def get_random_query_seq(query_list,length):
    query_seq = []
    for i in range(length):
        query_seq.append(random.choice(query_list))
    return query_seq

def segregate(mapping):
    cxl_cols = []
    dam_cols = []
    for key,val in mapping.items():
        if val == True:
            cxl_cols.append(key)
        else:
            dam_cols.append(key)
    return cxl_cols,dam_cols

if __name__ == '__main__':
    
    hotness_source = sys.argv[1]

    access_counts_file = "tpch_10_access_counts.csv"
    trace_counts_file = "tpch_10_trace_counts.dat"
    table_vs_non_table_file = "table_vs_non_table_10.dat"
    mapping_file = "mapping.csv"

    # seq_length = int(sys.argv[1])

    query_list = ["1","2","3","4","5","6","7","8","9","10","11","12","13","14","16","17","18","19","20","21","22"]
    # query_list = ["6","7","8","11","12","14","17","19","20"]

    #First load all the access counts i.e., what we get from the query plan prediction or hyrise access counters
    access_counts = DataTable(parse_csv_file(access_counts_file))
    #Next, load all the actual llc misses i.e., what we get from the pintool tracing
    trace_counts = DataTable(parse_csv_file(trace_counts_file))
    #Load in the number of non-table accesses for each query
    non_table_accesses = load_non_table_accesses(table_vs_non_table_file)
    
    sizes = {x.split(':')[0] : int(x.split(':')[1]) for x in open("size_10G.dat").readlines()}

    #Initial Mapping
    mapping = {col:True for col in trace_counts.col_indices.keys()}
    # mapping = {col:random.choice([True,False]) for col in trace_counts.col_indices.keys()}
    # print(mapping)

    #For plotting
    a_values = []
    b_values = []

    num_iterations = 1

    #Here we do the iterations
    for i in range(num_iterations):
        #Get a query sequence
        # query_seq = get_random_query_seq(query_list,seq_length)
        # query_seq = ["8","11","13"]
        # query_seq = [sys.argv[1]]
        query_seq = [sys.argv[2]]

        # print(f"Query Seq {query_seq}")

        #Check hotness of this query sequence
        cmd = f"python analyze_hotness.py {hotness_source} {' '.join(query_seq)} > hotness.dat"
        subprocess.run(cmd,shell=True)

        #Pass on the hotness to the LP solver
        # subprocess.run(f"python data_manager.py hotness.dat size_10G.dat 0.6 10",shell=True,capture_output=True,text=True)

        #Calculate AMAT before mapping
        amat_before = apply_mapping_get_amat(query_seq,mapping,trace_counts,non_table_accesses)
        
        old_mapping = mapping.copy()
        
        #Update mapping for next iteration
        mapping = load_mapping("mapping.csv")
    
        cxl_cols,dam_cols = segregate(mapping)
        print(f"CXL:{cxl_cols}")
        print(f"DAM:{dam_cols}")
        cxl_size = 0
        for col in cxl_cols:
            cxl_size += sizes[col]
        dam_size = 0
        for col in dam_cols:
            dam_size += sizes[col]
        # print(f"CXL:{cxl_size/2**30}")
        # print(f"DAM:{dam_size/2**30}")


        #Calculate AMAT after mapping
        amat_after = apply_mapping_get_amat(query_seq,mapping,trace_counts,non_table_accesses)
    
        # print("delta:",end="")
        # for key in mapping.keys():
        #     if mapping[key] != old_mapping[key]:
        #         print(f"{key}",end=',')
        # print("")

        # print(f"Iteration {i}")
        # print(f"Mapping {mapping}")
        # print(f"AMAT: {amat_before}")
        print(f"AMAT: {amat_before} -> {amat_after}")
        # print(f"{query_seq[0]},{amat_before},{amat_after},{dam_size/2**30},{cxl_size/2**30}")
        # print(f"{amat_after}")
        # print(f"{query_seq[0]},{dam_size/2**30}")
        # print(f"{query_seq}")

        a_values.append(amat_before)
        b_values.append(amat_after)

    # x_values = list(range(num_iterations))
    # # x_values = [0,1]

    # avg_improvement = sum([b-a for a,b in zip(a_values,b_values)])/num_iterations

    # plt.quiver(x_values, a_values, [0]*len(x_values) , [b-a for a,b in zip(a_values,b_values)], angles='xy', scale_units='xy', scale=1)

    # plt.xlabel("Iteration")
    # plt.ylabel("AMAT")

    # plt.title(f"SEQ: {seq_length}, IT: {num_iterations}, DELTA: {avg_improvement}")

    # plt.savefig(f"Transitions_all_{seq_length}.png")

    # #Get the mapping from the mapping file
    # mapping = load_mapping(mapping_file,list(access_counts.col_indices.keys()))


    # print(sum(trace_counts.access_row("1")))
    # print(non_table_accesses["1"])

    # print(f"{query_list}")
    # print(f"AMAT: {amat}")