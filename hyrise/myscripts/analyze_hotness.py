import sys
import csv

def load_hotness(filename):

    hotness = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            hotness.update({row[0]:int(row[1])})
    
    return hotness

def load_row_width(filename):

    row_widths = dict()
    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            row_widths.update({row[0]:float(row[1])})
    
    return row_widths

if __name__ == '__main__':

    hotness_file = sys.argv[1]
    row_width_file = sys.argv[2]
    size_file = sys.argv[3]
    hotness = load_hotness(hotness_file)
    row_widths = load_row_width(row_width_file)
    sizes = load_hotness(size_file)
    access_size = {col:hotness[col]*row_widths[col] for col in hotness.keys()}

    sorted_dict = dict(sorted(access_size.items(), key=lambda item: item[1], reverse=True))

    for key,val in sorted_dict.items():
        print(f"{key},{val},{sizes[key]/2**30}")
