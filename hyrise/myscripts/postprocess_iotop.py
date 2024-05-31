import sys
import csv

if __name__ == '__main__':
    filename = sys.argv[1]
   
    lines = open(filename).readlines()

    sum = 0

    for line in lines:
        io_percentage = float(line.split(':')[1].split(',')[0])
        sum += io_percentage

    # print(f"{sum}/{len(lines)} = {sum/len(lines)}")
    print(f"{sum/len(lines)}")