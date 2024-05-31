import sys
import subprocess
import multiprocessing

if __name__ == '__main__':
    """
    Pass in the path to a query and give the scaling factor
    This will run the query, generate the access counter file and move it into a new folder
    """

    if len(sys.argv) != 4:
        print(f"Expected three arguments")
        exit(1)

    
    query_path = sys.argv[1]
    scaling_factor = sys.argv[2]
    hyrise_dir = sys.argv[3]

    #Create the sql file to run on hyrise
    with open("tracing.sql","w") as file:
        file.write(f"wait 3\n")
        file.write(f"generate_tpch {scaling_factor}\n")
        file.write(f"setting scheduler on\n")
        file.write(f"acctr\n")
        file.write(f"script {query_path}\n")
        file.write(f"acctr\n")
        file.write(f"visualize\n")
        file.write(f"visualize lqp\n")
        file.write(f"quit\n")

    #Check if there is any other hyrise process running currently, if so print error
    #TODO: remove this limitation
    # result = subprocess.run("pgrep -n hyriseConsole",shell=True,capture_output=True,text=True)
    # split_line=result.stdout.splitlines()

    # if len(split_line) != 0:
    #     print(f"Detected other hyrise instance {split_line}")
    #     exit(1)

    #Run hyrise
    p = subprocess.Popen(f"{hyrise_dir}/hyriseConsole tracing.sql",shell=True)

    #To track the current hyrise process
    result = subprocess.run("pgrep -n hyriseConsole",shell=True,capture_output=True,text=True)
    split_line=result.stdout.splitlines()
    
    # if len(split_line) != 1:
    #     print(f"Expected single hyrise instance {split_line}")
    #     exit(1)

    pid = int(split_line[0])

    print(f"Captured PID: {pid}")

    p.wait()

    query_name = query_path.split('/')[-1].split('.')[0]

    #Move the generated .png and .json files into the newly created dir
    final_dir = f"access_query_{query_name}_{scaling_factor}_{pid}"
    subprocess.run(f"mkdir -p {final_dir}",shell=True)
    subprocess.run(f"mv access_counters_{pid}.txt pqp.png lqp.png pqp_{pid}.json lqp_{pid}.json -t {final_dir}",shell=True)

    #Calculate the access counter changes
    subprocess.run(f"python access_counter_change.py {final_dir}/access_counters_{pid}.txt",shell=True)
'''
    #Generate the combined(lqp+pqp) plan json 
    subprocess.run(f"python combine_lqp_pqp.py {final_dir}",shell=True)
    #Run prediction on combined plan information
    subprocess.run(f"python process_query_plan.py {final_dir}/pp_{pid}.json",shell=True)


    #Read the column wise access counts and compare with prediction
    subprocess.run(f"python verify_prediction.py {final_dir}",shell=True)
    # with open(f"{final_dir}/column_wise_data.csv") as file:
    #     lines = file.readlines()
    

    # column_wise_counts = dict()
    # for line in lines:
    #     key = line.split(",")[0]
    #     val = int(line.split(",")[6])
    #     column_wise_counts.update({key:val})

    # with open(f"{final_dir}/prediction.csv") as file:
    #     lines = file.readlines()

    # prediction_counts = dict()
    # for line in lines:
    #     key = line.split(",")[0]
    #     val = int(line.split(",")[1])
    #     prediction_counts.update({key:val})

    # for key,val in column_wise_counts.items():
    #     if key not in prediction_counts.keys():
    #         print(f"Missing column {key} in prediction counts")
    #     if val == prediction_counts[key]:
    #         print(f"Column {key} matches")
    #     else:
    #         print(f"Column {key} mismatch, {val} v/s {prediction_counts[key]}")
'''
