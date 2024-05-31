import sys


if __name__ == '__main__':
    filepath = sys.argv[1]

    size_counter = dict()

    current_col = ""

    with open(filepath) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if 'Table' in line:
                current_col = line.split(":")[3].strip()
                if current_col not in size_counter.keys():
                    size_counter.update({current_col:0})
                continue
            if '---' in line:
                continue
            if 'VEC' in line:
                continue
            size = int(line.split('(')[1].split(')')[0])
            size_counter[current_col] += size

    # for key in sorted(size_counter,key=size_counter.get,reverse=True):
    #     print(f"{key}:{size_counter[key]}")
    for key,val in size_counter.items():
        # print(f"{key}:{val}")
        print(f"{key},{val}")
            