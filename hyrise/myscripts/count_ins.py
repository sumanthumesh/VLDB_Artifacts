import sys

if __name__ == '__main__':
    filepath = sys.argv[1]
    with open(filepath) as file:
        sum = 0
        count = 0
        while True:
            line = file.readline()
            if not line:
                break
            if 'rtn' in line or 'Evict' in line:
                continue
            ins_gap = int(line.split(',')[-1])
            count += 1
            sum += ins_gap
    print(f"Sum: {sum}")
    print(f"Cnt: {count}")
    print(f"Avg: {sum/count}")