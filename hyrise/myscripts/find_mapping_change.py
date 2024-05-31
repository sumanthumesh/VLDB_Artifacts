import os
import sys
import csv
import subprocess

def get_mapping(hotness_file,dam_size):

    command = f"python optimizer.py {hotness_file} size_job.dat {dam_size} 10 row_width_job.dat hyrise"
    subprocess.run(command,shell=True)
    mapping = dict()
    with open("mapping.csv") as file:
        reader = csv.reader(file)
        for row in reader:
            mapping.update({row[0]:row[1]})

    if 'lqp' in hotness_file:
        new_name = "mapping_lqp.csv"
    elif 'pqp' in hotness_file:
        new_name = "mapping_pqp.csv"
    else:
        new_name = "mapping_counter.csv"

    # new_name = "mapping_lqp.csv" if "lqp" in hotness_file else "mapping_counter.csv"

    command = f"mv mapping.csv {os.path.join(os.path.abspath(os.path.dirname(hotness_file)),new_name)}"
    
    subprocess.run(command,shell=True)

    return mapping

def diff_in_mapping(map1,map2):

    if len(map1.keys()) != len(map2.keys()):
        print(f"Mismatched mapping length")
        exit(2)

    for key in map1.keys():
        if map1[key] != map2[key]:
            return True

    return False

def find_amat(mapping_file):
    
    base_dir = os.path.abspath(os.path.dirname(mapping_file))
    command = f"python evaluate_mapping.py {base_dir}/trace_count.csv {mapping_file} {base_dir}/non_table_accesses.txt size_job.dat"

    result = subprocess.run(command,shell=True,capture_output=True,text=True)

    lines = result.stdout.splitlines()
    amat_before = float(lines[-1].split(',')[0])
    amat_after = float(lines[-1].split(',')[1])

    return (amat_before,amat_after)

if __name__ == '__main__':

    base_dir = sys.argv[1]

    hotness_from_counters = os.path.join(base_dir,"hotness_counter.csv")
    hotness_from_lqp = os.path.join(base_dir,"hotness_lqp.csv")
    hotness_from_pqp = os.path.join(base_dir,"hotness_pqp.csv")

    dam_sizes_diff = []
    map_diff = dict()
    start = 0.01
    increment = 0.01
    s = start
    while s < 2:

        counter_mapping = get_mapping(os.path.join(base_dir,"hotness_counter.csv"),s)
        counter_amat = find_amat(os.path.join(base_dir,"mapping_counter.csv"))
        lqp_mapping = get_mapping(os.path.join(base_dir,"hotness_lqp.csv"),s)
        lqp_amat = find_amat(os.path.join(base_dir,"mapping_lqp.csv"))
        # pqp_mapping = get_mapping(os.path.join(base_dir,"hotness_pqp.csv"),s)
        # pqp_amat = find_amat(os.path.join(base_dir,"mapping_pqp.csv"))

        if diff_in_mapping(counter_mapping,lqp_mapping):
            map_diff.update({s:(counter_amat[0],counter_amat[1],lqp_amat[1])})
        
        # if diff_in_mapping(pqp_mapping,lqp_mapping):
        #     map_diff.update({s:(pqp_amat[0],pqp_amat[1],lqp_amat[1])})

        s += increment

    print('Differences:')
    for key,val in map_diff.items():
        print(f"{key},{val}")
    