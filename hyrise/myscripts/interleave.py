import sys
import os

def extract_pids(filename):
    with open(filename) as file:
        lines = file.readlines()
    
    pids = []

    for line in lines:
        pids.append(int(line))

    return pids

def are_all_accesses_empty(accesses):
    for access in accesses:
        if access:
            return False
    return True

def get_ins_gaps(lines):
    ins_gaps = []
    for line in lines:
        if not line:
            ins_gaps.append(None)
        ins_gaps.append(int(line.split(",")[0]))
    return ins_gaps

def get_access_details(line):
    
    if not line:
        return None

    addr = int(line.split(" ")[0],16)
    access_type = "R" if "R" in line.split(" ")[1] else "W"
    ins_gap = int(line.split(" ")[2][:-1])

    return [ins_gap,addr,access_type]

def get_all_accesses(lines):
    dets = []
    for line in lines:
        if not line:
            dets.append(None)
        dets.append(get_access_details(line))
    return dets

def get_lowest_gap_idx(accesses):

    lowest = -1
    for idx,access in enumerate(accesses):
        if not access:
            continue
        if lowest < 0:
            lowest = idx
        if access[0] < accesses[lowest][0]:
            lowest = idx
    if lowest < 0:
        print("Invalid lowest idx")
        exit(2)
    return lowest

def update_accesses(accesses,lowest_idx):
    # print("========================================")
    # print("Before")
    # print(accesses)
    # print(lowest_idx)
    for idx,access in enumerate(accesses):
        # print(f"{access} v/s {accesses[lowest_idx]}")
        if idx == lowest_idx:
            # print("skip")
            continue
        if accesses[idx] == None:
            continue
        accesses[idx][0] -= accesses[lowest_idx][0]
    # print("After")
    # print(accesses)
    # print("========================================")
    return accesses

def interleave_mem_accesses(file_list,dst_file):
    #This function takes a list of roitrace.csv files, each file representing the trace of a single process and processes them to give an interleaved access trace
    file_ptrs = [open(x) for x in file_list]
    #Read first line from all the files
    lines = [file_ptr.readline() for file_ptr in file_ptrs]
    dst = open(dst_file,"w")
    #Read the accesses from the lines, each access is a tuple (ins gap, addr,R/W)
    accesses = get_all_accesses(lines)
    while True:
        #Break if all the files are empty
        if are_all_accesses_empty(accesses):
            break
        #Process all the non empty lines and get a current state
        #The state is made up of a total instruction counter and the instruction gap given by the head of each file
        #Algorithm
        #If we have N files, F0, F1, F2, ..... FN-1, and we only consider the first line of each file
        #In every cycle, we do the following
        #Choose the file with the least instruction gap F_least
        #Subtract the ins gap of least from all the other files and update their values
        #Increment the instruction counter by instructon gap of F_least
        #Fetch net line for F_least

        #Find the file with the lowest instruction gap, F_least
        lowest_gap_idx = get_lowest_gap_idx(accesses)
        #Write out this access to file
        dst.writelines(f"{hex(accesses[lowest_gap_idx][1])} {accesses[lowest_gap_idx][2]} {accesses[lowest_gap_idx][0]}\n")
        #Update each ins_gap in all other file heads by subtracting the ins_gap of the lowest
        accesses = update_accesses(accesses,lowest_gap_idx)
        #Fecth a new line to fill for the old one for F_least
        new_line = file_ptrs[lowest_gap_idx].readline()
        acc_details = get_access_details(new_line)
        accesses[lowest_gap_idx] = acc_details
    dst.close()


if __name__=='__main__':
    dir = sys.argv[1]
    pids = extract_pids(f"{dir}/pids.txt")
    trace_list = [f"{dir}/cxlsim_{pid}.trace" for pid in pids]
    interleave_mem_accesses(trace_list,f"{dir}/interleaved.trace")



