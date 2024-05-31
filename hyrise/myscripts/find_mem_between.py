import os
import sys

def get_addr_from_line(line):
    return int(line.split(",")[2],16)

if __name__ == '__main__':
    filename = sys.argv[1]
    start_addr = int(sys.argv[2],16)
    end_addr   = int(sys.argv[3],16)

    #optional argument number 4 that if it says lsit, then list out all the line numbers at which you see matches
    list_match_line_numers = False if len(sys.argv) <= 4 or sys.argv[4] != "list" else True

    line_numbers = []

    total_ctr = 0
    range_ctr = 0
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            total_ctr += 1
            if "RTN" in line:
                continue
            addr = get_addr_from_line(line)
            if addr >= start_addr and addr <= end_addr:
                range_ctr += 1
                if list_match_line_numers:
                    line_numbers.append(total_ctr+1)
    print(f"({hex(start_addr)},{hex(end_addr)})")
    print(f"Total: {total_ctr}")
    print(f"Range: {range_ctr}")
    if list_match_line_numers:
        for idx,ele in enumerate(line_numbers):
            print(f"{idx}: {ele}")