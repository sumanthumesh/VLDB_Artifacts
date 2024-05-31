import sys
import os

data_dir = f"/data1/sumanthu/hyrise/build_reldebug"

def load_addr(filename):
    addr_range = []
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            addr_start = int(line.split(",")[0],16)
            addr_end   = int(line.split(",")[1],16)
            addr_range.append((addr_start,addr_end))
    return addr_range

def process_line(line):
    # print(line)
    ins_gap = int(line.split(",")[0])
    addr    = int(line.split(",")[2],16)
    return (ins_gap,addr)

def is_table_access(addr,addr_range):
    for pair in addr_range:
        if addr >= pair[0] and addr <= pair[1]:
            return True
    return False

def classify(filename,isolate_file,addr_range):
    dst = open(isolate_file,"w")
    mis_counts = 0
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if len(line.split(",")) != 4:
                mis_counts+=1
                continue
            (ins_gap,addr) = process_line(line)
            if is_table_access(addr,addr_range):
                dst.writelines(f"{ins_gap},{hex(addr)},T\n")
            dst.writelines(f"{ins_gap},{hex(addr)}\n")
    print(f"Miss counts: {mis_counts}")
    dst.close()

if __name__ == '__main__':
    pid = int(sys.argv[1])

    roi_file = f"{data_dir}/roitrace_{pid}.csv"
    addr_file = f"{data_dir}/addr_{pid}.csv"
    iso_file = f"{data_dir}/isolate_{pid}.csv"

    #Read the address ranges from file
    addr_range = load_addr(addr_file)

    #Now create a new file where you dump in the accesses
    classify(roi_file,iso_file,addr_range)