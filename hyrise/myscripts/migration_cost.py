import csv
import os
import sys

def load_times(filename):
    
    times = dict()
    with open(filename) as file:
        reader=csv.reader(file)
        for row in reader:
            times.update({row[0]:(float(row[1]),0)})

    return times

def load_sizes(filename):
    sizes = dict()

    with open(filename) as file:
        reader=csv.reader(file)
        for row in reader:
            sizes.update({row[0]:float(row[1])})

    return sizes    

if __name__ == '__main__':

    # hotness_file = sys.argv[1]
    mapping_file = sys.argv[1]

    active_columns = []

    with open(mapping_file) as file:
        reader = csv.reader(file)
        for row in reader:
            if row[1] == '0':
                active_columns.append(row[0])

    migration_times = load_times('column_migration_time_pcie_5.csv')

    sizes = load_sizes('size_100SF.dat')

    total_time = 0
    total_size = 0

    db_size = sum(list(sizes.values()))

    for col in active_columns:
        print(f"{col} : {migration_times[col]}")
        total_time += migration_times[col][0]
        total_size += sizes[col]

    for idx,col in enumerate(active_columns):
        print(f"{idx}:{col}")

    print(f"Time: {total_time}ms")
    print(f"Transferred GB: {total_size/(2**30)}GB")
    print(f"Transferred %: {total_size/db_size*100}%")