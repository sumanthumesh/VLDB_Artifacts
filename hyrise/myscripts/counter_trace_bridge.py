import sys
import csv
import subprocess
import os

def count_column_from_trace(trace_file,table_info_file):
    #Load table info    
    table_info = dict()
    with open(table_info_file) as file:
        reader = csv.reader(file)
        for row in reader:
            table_info.update({row[0]:hex(int(row[1]))[2:]+hex(int(row[2]))[2:]})
    
    for key in table_info.keys():
        print(f"{key}",end=',')
    print("")
    
    #Now grep for each of the tableid columnid pairs in the trace file
    trace_counts = dict()
    for key in table_info.keys():
        print(f"Searching for {key}")
        grep_command = f"grep 0xa{table_info[key]} {trace_file} | wc -l"
        result = subprocess.run(grep_command,shell=True,capture_output=True,text=True)
        trace_counts.update({key:int(result.stdout.strip())})

    print(trace_counts)

    with open(f"accesses_{query_name}.csv","w") as file:
        # counts = [x for x in trace_counts.values()]
        writer = csv.writer(file)
        for key,val in trace_counts.items():
            writer.writerow([key,val])

    print(f"Output written to accesses_{query_name}.csv")

if __name__ == '__main__':
    
    trace_file = sys.argv[1]

    query_name = trace_file.split('/')[-2].split('_')[1]

    print(query_name)

    if len(sys.argv) > 2:
        table_info_file = sys.argv[2]
    else:
        table_info_file = "details.dat"

    count_column_from_trace(trace_file,table_info_file)

    # #Load table info    
    # table_info = dict()
    # with open(table_info_file) as file:
    #     reader = csv.reader(file)
    #     for row in reader:
    #         table_info.update({row[0]:hex(int(row[1]))[2:]+hex(int(row[2]))[2:]})
    
    # for key in table_info.keys():
    #     print(f"{key}",end=',')
    # print("")
    # #Load the access counters
    # access_counts = dict()
    # with open(access_counter_file) as file:
    #     reader = csv.reader(file)
    #     for row in reader:
    #         access_counts.update({row[0]:int(row[6])})
    
    # #Now grep for each of the tableid columnid pairs in the trace file
    # trace_counts = dict()
    # for key in table_info.keys():
    #     print(f"Searching for {key}")
    #     grep_command = f"grep 0xa{table_info[key]} {trace_file} | wc -l"
    #     result = subprocess.run(grep_command,shell=True,capture_output=True,text=True)
    #     trace_counts.update({key:int(result.stdout.strip())})

    # print(trace_counts)

    # with open(f"accesses_{query_name}.csv","w") as file:
    #     # counts = [x for x in trace_counts.values()]
    #     writer = csv.writer(file)
    #     writer.writerows([trace_counts.values()])