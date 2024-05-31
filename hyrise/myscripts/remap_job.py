import os
import sys
import subprocess

if __name__ == '__main__':
    
    trace_file = sys.argv[1]
    out_file = sys.argv[2]
    mapping_file = sys.argv[3]

    print("Replacing 0xa with 0xb")
    subprocess.run(f"sed -i 's/0xa/0xb/g' {trace_file}",shell=True)

    #Load columns and their table/column ids
    column_details = {(int(s.split(',')[1].strip()),int(s.split(',')[2].strip())):s.split(',')[0] for s in open("job_columns.dat").readlines()}

    for key,val in column_details.items():
        print(f"{key},{val}")

    #Load mapping
    mapping = {s.split(',')[0]:True if s.split(',')[1].strip() == '1' else False for s in open(mapping_file).readlines()}

    for key,val in mapping.items():
        print(f"{key},{val}")


    fout = open(out_file,"w")
    
    mapped_cols = []
    
    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if '0xa' not in line and '0xb' not in line:
                fout.write(line)
                continue
            addr = line.split(' ')[0]
            table_id = int(addr[3:5],16)
            column_id = int(addr[5],16)
            # print(f"{addr},{table_id},{column_id}")
            if mapping[column_details[(table_id,column_id)]] == False:
                if (table_id,column_id) not in mapped_cols:
                    mapped_cols.append((table_id,column_id))
                    print(f"{addr},{table_id},{column_id}")
                line=line.replace('0xb','0xa')
            fout.write(line)
            