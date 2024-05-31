import sys
import os
import matplotlib.pyplot as plt
import numpy as np

if __name__ == '__main__':
    
    
    if len(sys.argv) != 2:
        print(f"Expected one argument, got {sys.argv[1:]}")
        exit(1)

    cols_to_track = [0xa20,
                     0xa24,
                     0xa26,
                     0xa10,
                     0xa13]
    
    col_names = ['l_orderkey',
                 'l_quantity',
                 'l_discount',
                 'o_orderkey',
                 'o_totalprice']


    trace_file = sys.argv[1]

    time = 0

    print("Processing file")

    epoch_size = 1000

    counter = 0

    verif = [0] * len(cols_to_track)

    timestamps = []

    for i in range(len(cols_to_track)):
        timestamps.append([])

    print(timestamps)

    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if counter % 1000 == 0:
                print(counter)
            ins_gap = int(line.split(' ')[0])
            addr = int(line.split(' ')[2],16)
            
            # print(f"Addr: {hex(addr)}")

            time += ins_gap
            for idx,col in enumerate(cols_to_track):
                if (addr & 0xfff000000000000) >> 48 == col:
                    # print(f"Matched with col {hex(col)}")
                    # if int(time/epoch_size) not in timestamps[idx]:
                    timestamps[idx].append(int(time/epoch_size))
                    verif[idx] += 1
                    # print(timestamps)
            counter += 1

    # print(timestamps)
    print("File processed")

    colors = ['blue','green','red','cyan','magenta','yellow','black']



    print(f"Time: {time}")
    x_axis = range(0,int(time/epoch_size))
    # print(x_axis)
    print("Created x axis")
    # y_axis = np.where(np.isin(x_axis, timestamps[idx]), 1, 0)
    #     plt.scatter(x_axis,y_axis,label=f"{col}",s=1,color=colors[idx])
    for idx,col in enumerate(cols_to_track):
        print(f"y={idx}")
        y_axis = np.where(np.isin(x_axis, timestamps[idx]), idx+1, 0)
        # print(f"SUM: {np.sum(y_axis)}")
        print(f"Column: {col_names[idx]}")
        print(f"Min: {np.min(timestamps[idx])}")
        print(f"Max: {np.max(timestamps[idx])}")
        print(f"NUM: {len(timestamps[idx])}")
        # print(timestamps[idx])
        # y_axis = [1 if x in timestamps[idx] else 0 for x in x_axis]
        # print(len(y_axis))
        # plt.scatter(x_axis,y_axis,label=f"{col_names[idx]}",s=1,color=colors[idx])
        
    # # print("Created y axis")
    print("Plot done")
    # plt.xlabel("Time")
    # plt.ylabel("Accessed or not")
    # plt.legend()

    # plt.savefig('plot.png')


    
    print(verif)

    # with open("verif.csv","w") as file:

    # for idx,col in enumerate(cols_to_track):
    #     y_axis = y_axis = np.where(np.isin(x_axis, timestamps[idx]), 1, 0)
        # plt.scatter()       