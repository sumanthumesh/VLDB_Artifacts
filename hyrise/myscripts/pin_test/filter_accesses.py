import sys

if __name__ == '__main__':

    start_addr = int(sys.argv[1],16)
    end_addr   = int(sys.argv[2],16)

    filename = sys.argv[3]

    miscount=0

    in_range = 0
    count=0
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if 'pc' in line:
                continue
            print(line)
            if len(line.split(",")) == 3:
                addr = int(line.split(",")[1],16)
            elif len(line.split(",")) == 4:
                addr = int(line.split(",")[2],16)
            elif len(line.split(",")) == 5:
                addr = int(line.split(",")[3],16)
            else:
                miscount+=1
                continue
            if addr >= start_addr and addr <= end_addr:
                in_range +=1
            count+=1
    print(f"In range: {in_range}")
    print(f"Missed  : {miscount}")
    print(f"Total   : {count}")