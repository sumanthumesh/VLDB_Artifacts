import subprocess
import csv
import os
import sys
import time

if __name__ == '__main__':
    # We take the path to query and create a .sql file which we will pass onto hyrise to trace
    # Then we will move these files into a new folder for safekeeping

    # We take 2 arguments, first is the query, second is the scaling factor
    if len(sys.argv) != 4:
        print(
            f"Expect 3 args, query path, sclaing factor and path to dir with hyriseConsole binary, received only {sys.argv[1:]}")
        exit(1)


    scaling_factor = float(sys.argv[2]) if float(sys.argv[2]) < 1 else int(sys.argv[2])
    query_path = sys.argv[1]

    # Figure out the query numer from the path
    query_name = query_path.split('/')[-1].split('.')[0]

    if not os.path.exists(query_path):
        print(f"Cannot access {query_path}")
        exit(1)

    # Here we create the query itself and write it to a file
    query_file = "tracing.sql"
    with open(query_file, "w") as file:
        #Generate tables
        file.write(f"script ../third_party/jobenchmark/load_job.sql\n")
        # Write out the mem_blks file for reference
        file.write(f"acctr\n")
        file.write("seg\n")
        # #Write out the ranges file
        # file.write("coalesce\n")
        #Access counters
        #Set the multi threading scheduler
        file.write("setting scheduler on\n")
        #Set pin tracing on
        file.write("pintool on\n")
        #Run the query
        file.write(f"script {query_path}\n")
        #Print PID for cross referencing
        #Access counters
        file.write(f"visualize\n")
        file.write(f"visualize lqp\n")
        file.write(f"acctr\n")
        file.write(f"pid\n")
        #Quit
        file.write("quit\n")

    # Fork a process to run the tracing
    # #Before launcing hyrise, check if any other instance is running right now
    # result = subprocess.run("pgrep -n hyriseConsole",shell=True,capture_output=True,text=True)
    # split_str = result.stdout.splitlines()
    # if len(split_str) != 0:
    #     print(f"Another hyrise instance running {split_str}")
    #     exit(1)

    hyrise_dir = sys.argv[3]
    hyrise_path = os.path.join(hyrise_dir,"hyriseConsole")
    
    p = subprocess.Popen(f"{hyrise_path} {query_file}",shell=True)

    time.sleep(3)

    #Capture the pid of hyrise
    result = subprocess.run("pgrep -n hyriseConsole",shell=True,capture_output=True,text=True)
    split_str = result.stdout.splitlines()
    if len(split_str) != 1:
        print(f"Expected only one hyriseConsole, but found {split_str}")
        exit(1)
    
    pid = int(split_str[0])

    # Wait for the hyrise process to end
    p.wait()

    # Once hyrise has finished, move the resulting mem_blk, ranges and roitrace files to a new created folder
    print(f"Recorded hyrise PID: {pid}")

    # Create new folder
    new_folder = f"q_{query_name}_{scaling_factor}_{pid}"
    print(f"Moving files to new folder")
    subprocess.run(f"mkdir -p {new_folder}",shell=True)
    subprocess.run(f"mv mem_blk_{pid}.txt roitrace_{pid}.csv access_counters_{pid}.txt pqp_{pid}.json lqp_{pid}.json pqp.png lqp.png -t {new_folder}",shell=True)

    #Print stuff for reference
    print(f"Finished tracing for {pid}")

    #Generate the lqp and pqp _template.json files
    commands = []
    commands.append(f"python preprocess_lqp_node.py {new_folder}/lqp_{pid}.json")
    commands.append(f"python preprocess_pqp_node.py {new_folder}/pqp_{pid}.json")

    #Generate the hotness from access counters
    commands.append(f"python access_counter_change.py {new_folder}/access_counters_{pid}.txt")
    commands.append(f"python column_hotness.py {new_folder}/column_wise_data.csv && mv hotness.csv {new_folder}/hotness_counter.csv")

    #Generate isolate trace
    commands.append(f"python remove_roi.py {new_folder}/roitrace_{pid}.csv")
    commands.append(f"./isolate_job {new_folder}/mem_blk_{pid}.txt {new_folder}/roitrace_{pid}.csv {new_folder}/isolate_{query_name}.trace")

    #Hotness from pintrace
    commands.append(f"python column_count_from_job_trace.py {new_folder}/isolate_{query_name}.trace job_columns.dat > {new_folder}/trace_count.csv")

    #Table and non table counts
    commands.append(f"cat {new_folder}/isolate_{query_name}.trace | wc -l > {new_folder}/total_accesses.txt")
    commands.append(f"grep 0xa {new_folder}/isolate_{query_name}.trace | wc -l> {new_folder}/table_accesses.txt")

    for command in commands:
        print(command)
        subprocess.run(command,shell=True)

    table_accesses = int(open(f"{new_folder}/table_accesses.txt").readline().strip())
    total_accesses = int(open(f"{new_folder}/total_accesses.txt").readline().strip())
    non_table_accesses = total_accesses - table_accesses

    with open(f"{new_folder}/non_table_accesses.txt","w") as file:
        file.write(f"{non_table_accesses}\n")