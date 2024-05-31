import sys
import csv

def load_col_details(filename):
    
    #Dictionary with tableid,columid as key and column name as value
    col_details = dict()
    
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            table_id = int(row[1])
            column_id = int(row[2])
            column_name = row[0]

            col_details.update({(table_id,column_id):column_name})

    return col_details

if __name__ == '__main__':

    trace_file = sys.argv[1]
    details_file = sys.argv[2]

    col_details = load_col_details(details_file)

    lines = open(trace_file).readlines()

    counts = dict()

    for line in lines:
        if '0xa' not in line and '0xb' not in line:
            continue
        addr = line.split(' ')[0]
        imp_string = addr[3:3+3]
        col_id = int(imp_string[-1],16)
        table_id = int(imp_string[:-1],16)
        column_name = col_details[(table_id,col_id)]
        # print(f"{addr},{imp_string},{table_id},{col_id}.{column_name}")
        if column_name in counts:
            counts[column_name] += 1
        else:
            counts.update({column_name:1})

    # print(counts)

    # with open()

    sorted_dict = dict(sorted(counts.items(), key=lambda item: item[1], reverse=True))

    for col,count in sorted_dict.items():
        print(f"{col},{count}")