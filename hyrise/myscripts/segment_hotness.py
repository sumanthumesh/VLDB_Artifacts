import csv
import sys
import os

def parse_access_counters(filename):
    
    list_counters = []
    
    counter = dict()
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if '-----' in line:
                if len(counter) != 0:
                    list_counters.append(counter)
                    counter = dict()
                continue
            split_line = line.split(',')
            segment_identifier = (split_line[0],split_line[1],split_line[2])
            access_counter = sum([int(x) for x in split_line[3:-1]])
            counter.update({segment_identifier:access_counter})

    list_counters.append(counter)

    if len(list_counters) != 2:
        print(f"Something went wrong, should have only see 2 timestamps")
        exit(1)

    if len(list_counters[0]) != len(list_counters[1]):
        print(f"Counters of unequal shapes")
        exit(1)

    return list_counters

def find_diff(before_ctr,after_ctr):
    if len(before_ctr) != len(after_ctr):
        print(f"Length mismatch {before_ctr} v/s {after_ctr}")
        exit(1)

    delta = dict()

    for seg_id in before_ctr.keys():
        delta.update({seg_id:after_ctr[seg_id]-before_ctr[seg_id]})

    return delta

if __name__ == '__main__':

    access_ctr_file = sys.argv[1]

    before_ctr, after_ctr = parse_access_counters(access_ctr_file)

    delta = find_diff(before_ctr,after_ctr)

    out_file = f"{os.path.join(os.path.abspath(os.path.dirname(access_ctr_file)),'segment_hotness.csv')}"
    # out_file = f"segment_hotness.csv"

    with open(out_file, "w") as file:
        writer = csv.writer(file)
        for key,val in delta.items():
            if val == 0:
                continue
            row = [key[0],key[1],key[2],val]
            writer.writerow(row)
        