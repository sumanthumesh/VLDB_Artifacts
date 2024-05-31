import os
import sys

if __name__ == '__main__':

    if len(sys.argv) != 2:
        print(f"Expected one argument, got {sys.argv[1:]}")
        exit(1)
        
    trace_file = sys.argv[1]

    transitions = {}

    last_table_access = ''

    line_num = 0
    last_segment_line_num = 0

    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            line_num += 1
            if '0xa' not in line:
                continue

            current_segment = line.split(' ')[2][:5]

            #Check if it is a transition
            if current_segment != last_table_access:
                #Check if we saw this transition before
                if (last_table_access,current_segment) in transitions.keys():
                    transitions[(last_table_access,current_segment)] += 1
                else:
                    transitions[(last_table_access,current_segment)] = 1
                # if last_table_access == '0xa10' and current_segment == '0xa26':
                print(f"{last_segment_line_num},{line_num}")
            
            last_table_access = current_segment
            last_segment_line_num = line_num
    
    for key,value in transitions.items():
        print(f"{key}:{value}")