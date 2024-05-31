import os
import sys
import subprocess

if __name__ == '__main__':
    base_dir = sys.argv[1]
    size_dam = sys.argv[2]

    hotness_files = {
        'lqp' : "hotness_lqp.csv",
        'counter' : "hotness_counter.csv",
        'ideal' : "trace_count.csv"
    }

    amat = dict()

    for hotness_type,filename in hotness_files.items():
        # Generate the mapping file
        if hotness_type == "ideal":
            mode = "pin"
        else:
            mode = "hyrise"

        # Commands to execute
        commands = []
        #Generate mapping
        command = f"python optimizer.py {base_dir}/{filename} size_job.dat {size_dam} 10 row_width_job.dat {mode}"
        subprocess.run(command,shell=True)
        #Get the AMAT
        command = f"python evaluate_mapping.py {base_dir}/trace_count.csv mapping.csv {base_dir}/non_table_accesses.txt size_job.dat"
        result=subprocess.run(command,shell=True,capture_output=True,text=True)
        lines = result.stdout.splitlines()

        print(result.stderr)

        amat_before = float(lines[-1].split(',')[0])
        amat_after = float(lines[-1].split(',')[1].strip())

        amat.update({hotness_type:float(amat_after)})

    print(f"{amat_before},{amat['counter']},{amat['lqp']},{amat['ideal']}")