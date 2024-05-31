import sys

if __name__ == '__main__':
    mapping_file = sys.argv[1]
    trace_file = sys.argv[2]
    non_table_file = sys.argv[3]

    mapping = dict()

    for line in open(mapping_file).readlines():
        column_name = line.split(',')[0]
        mapped_device = True if line.split(',')[1].strip() == '1' else False
        mapping.update({column_name:mapped_device})

    counts = dict()

    for line in open(trace_file).readlines():
        column_name = line.split(',')[0]
        trace_count = int(line.split(',')[1].strip())
        counts.update({column_name:trace_count})

    non_table_counts = int(open(non_table_file).readline().strip())

    print(len(counts))

    cxl_count = 0
    dam_count = 0
    total_count = 0

    for col,num in counts.items():
        if mapping[col] == True:
            cxl_count += num
        else:
            dam_count += num
        total_count += num

    print(f"{cxl_count},{dam_count+non_table_counts},{total_count+non_table_counts}")