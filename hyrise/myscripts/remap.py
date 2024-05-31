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
    column_details = {s.split(',')[0]:(s.split(',')[1].strip(),hex(int(s.split(',')[2].strip()))[2:]) for s in open("details.dat").readlines()}

    #Load mapping
    mapping = {s.split(',')[0]:True if s.split(',')[1].strip() == '1' else False for s in open(mapping_file).readlines()}

    # print(column_details)
    # print(mapping)

    args = []

    for col in mapping.keys():
        if mapping[col] == False:
            args.append(f"0xb{column_details[col][0]}{column_details[col][1]}")
            args.append(f"0xa{column_details[col][0]}{column_details[col][1]}")

    print(args)

    arg = ' '.join(args)

    #Call the c++ program to deal with the replacements
    command = f"./string_replace {trace_file} {out_file} {arg}"
    print(command)
    subprocess.run(command,shell=True)

    # # The rest of the arguments need to be pairwise
    # if not len(sys.argv[3:]) % 2 == 0:
    #     print(f"Expected even number of args, received {sys.argv[3:]}")
    #     exit(1)

    # temp = sys.argv[3:]

    # pairs = [temp[i:i+2] for i in range(0,len(temp),2)]

    # #Create a copy of the trace file
    # # subprocess.run(f"cp {trace_file} {out_file}",shell=True)

    # #Call the c++ function to deal with this
    # args = ' '.join(sys.argv[3:])
    # command = f"./string_replace {trace_file} {out_file} {args}"
    # print(command)
    # subprocess.run(command,shell=True)

    # # for pair in pairs:
    # #     print(f"Replacing {pair[0]} with {pair[1]}")
    # #     subprocess.run(f"sed -i 's/{pair[0]}/{pair[1]}/g' {out_file}",shell=True)    

    # print(f"Output written to {out_file}")        

