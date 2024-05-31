import os
import sys
import re
import csv

if __name__ == '__main__':
    path = sys.argv[1]

    #List all the files
    all_files = os.listdir(path)

    #Filter them to find the files you need
    pattern = r'latency_.*.csv'
    filtered_files = []
    for file in all_files:
        if re.match(pattern,file):
            filtered_files.append(file)
    
    print(filtered_files)

    total_accesses = 0
    final_avg = 0

    cxl_accesses = 0
    dam_accesses = 0

    for file in filtered_files:
        print(file)
        with open(os.path.join(path,file)) as file:
            reader = csv.reader(file)
            for row in reader:
                num_accesses = int(float(row[2]))
                if row[0] == "1":
                    cxl_accesses += num_accesses
                else:
                    dam_accesses += num_accesses
                amat = float(row[3])
                final_avg = (final_avg * total_accesses + num_accesses * amat)/(total_accesses + num_accesses)
                total_accesses += num_accesses

    print(f"CXL Accesses : {cxl_accesses}")
    print(f"DAM Accesses : {dam_accesses}")
    print(f"Num Accesses : {total_accesses}")
    print(f"Final AMAT : {final_avg}")
