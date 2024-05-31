import sys
import subprocess
import math
import os

simulator_binary_dir = "/data1/sumanthu/cxlsim" 

if __name__ == '__main__':
    trace_file = sys.argv[1] # trace file dir
    num_workers = int(sys.argv[2]) # num of partitions
    query = sys.argv[3]
    trace_dir = sys.argv[4] # trace target dir

    # First get the number of lines in the pinfile
    result = subprocess.run(f"cat {trace_file} | wc -l", shell=True, capture_output=True)
    num_lines = int(result.stdout.splitlines()[0])
    lines_per_worker = math.ceil(num_lines/num_workers)

    print(f"{num_lines} total, {num_workers} workers, {lines_per_worker} lines per worker")

    if not os.path.exists(f"{trace_dir}/q_{query}"):
        os.makedirs(f"{trace_dir}/q_{query}")

    split_prefix = f"{trace_dir}/q_{query}/trace_part_"

    print("Splitting")
    subprocess.run(f"split -l {lines_per_worker} {trace_file} {split_prefix}", shell=True)
    
    # Scan the directory for all files with matching name
    all_files = os.listdir(f"{trace_dir}/q_{query}")
    temp_src_files = [file for file in all_files if file.startswith("trace_part_")]
    # Generate the run script
    print("generating")
    with open(f"{trace_dir}/run_{query}.sh", 'w') as file:
        for part_input in temp_src_files:
            command = f"time cd {simulator_binary_dir} && time ./cxlsim {trace_dir}/q_{query}/{part_input} q_{query} {trace_dir} > {trace_dir}/q_{query}/{part_input}.log && rm -f {trace_dir}/q_{query}/{part_input}\n"
            file.write(command)