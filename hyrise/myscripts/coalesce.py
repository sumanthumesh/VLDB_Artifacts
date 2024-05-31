import sys
import os

blk_size = 2**20
line_size = 0x20

blks = []

def print_blks():
    for idx,ele in enumerate(blks):
        print(f"{idx}: {hex(ele[0])},{hex(ele[1])}")

def print_tuple(ele):
    print(f"{hex(ele[0])},{hex(ele[1])}")

def parse_line(line):
    #Sample of line 
    #String 0(18): 0x7fc3ac3ff060
    addr = int(line.split(":")[1].replace(" ",""),16)
    size = int(line.split(":")[0].split("(")[1].replace(")",""))
    return (size,addr)

def round_down(addr):
    return addr if addr % line_size == 0 else addr - addr % line_size

def round_up(addr):
    return addr if addr % line_size == 0 else addr + (line_size - addr % line_size)

def check_if_in_blks(addr_tuple):
    if len(blks) == 0:
        return False
    for ele in blks:
        if round_down(addr[0]) >= ele[0] and round_down(addr[0]) <= ele[1]:
            return True
        if round_up(addr[1]) >= ele[0] and round_up(addr[1]) <= ele[1]:
            return True
    return False

def give_matching_blk_id(addr_tuple):
    if len(blks) == 0:
        return -1
    #Round the address tuple
    addr = []
    addr.append(round_down(addr_tuple[0]))
    addr.append(round_up(addr_tuple[1]))
    for idx,ele in enumerate(blks):
        #Return idx if there is any overlap or if the difference is less than blk_size
        #case 1, disjoint, addr is greater than blk
        if addr[1] - ele[0] >= 0 and addr[1] - ele[0] <= blk_size:
            return idx
        #case 2, disjoint, addr is lesser than blk
        elif ele[1] - addr[0] >= 0 and ele[1] - addr[0] <= blk_size:
            return idx
        #case 3, overlap, addr is greater than blk
        elif addr[0] >= ele[0] and ele[0] >= addr[1] and addr[1] >= ele[1]:
            return idx
        #case 4, overlap, addr is lesser than blk
        elif ele[0] >= addr[0] and addr[0] >= ele[1] and ele[1] >= addr[1]:
            return idx 
    return -1

def update_blks(addr_tuple):
    # if len(blks) == 0:
    #     blks.append(addr_tuple)
    #     return
    # print("ADDR ",end="")
    # print_tuple(addr_tuple)
    blk_id = give_matching_blk_id(addr_tuple)
    #If no matching block exists, create a new one
    if blk_id == -1:
        blks.append([round_down(addr_tuple[0]),round_up(addr_tuple[1])])
        # print(f"New BLK ",end="")
        # print_tuple([round_down(addr_tuple[0]),round_up(addr_tuple[1])])
        return
    #Change the ending of the blk to end of the addr tuple
    if round_up(addr_tuple[1]) > blks[blk_id][1]:
        blks[blk_id][1] = round_up(addr_tuple[1]) 
        # print(f"Update BLK ",end="")
        # print_tuple(blks[blk_id])
    if round_down(addr_tuple[0]) < blks[blk_id][0]:
        blks[blk_id][0] = round_down(addr_tuple[0])
        # print(f"Update BLK ",end="")
        # print_tuple(blks[blk_id])

if __name__ == '__main__':
    filename = sys.argv[1]
    ctr = 0
    with open(filename) as file:
        while True:
            if ctr % 1000000 == 0:
                print(ctr)
            line = file.readline()
            if not line:
                break
            if '-' in line:
                continue
            if 'ROI' in line:
                continue
            (size,addr) = parse_line(line)
            update_blks([addr,addr+size])
            ctr += 1
    print_blks()

    # print(parse_line("String 0(18): 0x7fc3ac3ff060"))