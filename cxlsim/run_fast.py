import sys
import subprocess
import math
import os

if __name__ == '__main__':
    dir = sys.argv[1]
    num_workers = int(sys.argv[2])
    query = sys.argv[3]

    # # Scan the directory for all files with matching name
    # all_files = os.listdir(dir)

    # temp_src_files = [file for file in all_files if file.startswith("trace_part_")]

    # print(temp_src_files)

    # output_prefix = "latency_"
    # # Before having the c++ code generate new output files, make sure to delete the others that were there before it
    # print(f"Removing {output_prefix}* files ")
    # subprocess.run(f"rm -f /data1/nliang/split/q_{query}/{output_prefix}*", shell=True)

    # #List of intermediate output files
    # list_output_prefixes = []

    # #Launch multiple workers
    # processes = []
    # for part_input in temp_src_files:
    #     command = f"./cxlsim /data1/nliang/split/q_{query}/{part_input} {query}"
    #     print(command)
    #     p = subprocess.Popen(command,shell=True)
    #     processes.append(p)

    # for p in processes:
    #     p.wait()

    # #Get a list of all intermediate latency csv files
    all_files = os.listdir(dir)
    temp_result_files = sorted([file for file in all_files if file.startswith(f"latency_")])

    filelist = ' '.join(temp_result_files)
    command = f"cat {filelist} > consolidated_latency.csv"
    print(command)
    subprocess.run(command,shell=True)


    # Once all the files are combined remove all the temporary files we generated
    print("Removing all the intermediate files")
    # subprocess.run(f"rm -f latency_* {output_prefix}*", shell=True)