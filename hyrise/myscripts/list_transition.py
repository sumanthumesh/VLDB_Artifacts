import os
import sys
import csv

def find_all_transitions(start,end,trace_file):
    
    line_num = 0
    transitions = []
    time = 0
    start_line_num = 0

    last_access_is_start = False

    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            line_num += 1
            ins_gap = int(line.split(' ')[0])
            time += ins_gap

            # if '0xa' not in line:
            #     continue

            if start in line:
                start_line_num = line_num
                time = 0
                last_access_is_start = True
            elif end in line:
                if last_access_is_start:
                    transitions.append((start_line_num,line_num,time))
                last_access_is_start = False

    return transitions

if __name__ == '__main__':

    if len(sys.argv) != 4:
        print(f"Expected one argument, got {sys.argv[1:]}")
        exit(1)
        

    start = sys.argv[1]
    end = sys.argv[2]
    trace_file = sys.argv[3]

    transitions = find_all_transitions(start,end,trace_file)

    with open("transitions.csv","w") as file:
        writer = csv.writer(file)
        for t in transitions:
            writer.writerow(t)




    