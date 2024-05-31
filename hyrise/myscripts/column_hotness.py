import csv
import sys

if __name__ == '__main__':

    column_wise_counters = sys.argv[1]

    counters = dict()

    with open(column_wise_counters) as file:
        reader = csv.reader(file)
        for row in reader:
            column = row[0]
            count = sum([int(x) for x in row[1:6]])
            counters.update({column:count})
    
    # all_cols = [x.strip() for x in open("all_tpch_columns.dat").readlines()]

    # for col in all_cols:
    #     if col not in counters.keys():
    #         counters.update({col:0})

    print(counters)

    with open('hotness.csv',"w") as file:
        writer = csv.writer(file)
        writer.writerows([[key,val] for key,val in counters.items()])