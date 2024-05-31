import os
import sys
import csv
import matplotlib.pyplot as plt

def parse_access_counters(filename):
    
    list_counters = []
    
    counter = dict()
    with open(filename) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if '-----' in line:
                if len(counter) != 0:
                    list_counters.append(counter)
                    counter = dict()
                continue
            split_line = line.split(',')
            segment_identifier = (split_line[0],split_line[1],split_line[2])
            access_counter = [int(x) for x in split_line[3:]]
            counter.update({segment_identifier:access_counter})

    list_counters.append(counter)

    if len(list_counters) != 2:
        print(f"Something went wrong, should have only see 2 timestamps")
        exit(1)

    if len(list_counters[0]) != len(list_counters[1]):
        print(f"Counters of unequal shapes")
        exit(1)

    return list_counters

def has_counter_changed(end,start):
    temp = []
    for i in range(5):
        temp.append(end[i]-start[i])

    return temp
 
if __name__ == '__main__':

    access_counter_path = sys.argv[1]

    if len(sys.argv) == 2:
        is_plot = False
    elif sys.argv[2] == 'plot':
        is_plot = True
    else:
        is_plot = False 

    [start,end] = parse_access_counters(access_counter_path)

    # count = 0
    # for key,val in end.items():
    #     if key[0] == 'lineitem' and key[2] == 'l_extendedprice':
    #         count += 1
    # print(f"Start Count {count}")

    delta = dict()

    for key in start.keys():
        for i in range(5):
            diff = has_counter_changed(end[key],start[key])
            if sum(diff) == 0:
                if key[0] == 'lineitem' and key[2] == 'l_extendedprice':
                    print(f"Removed {key}")
                continue
            delta.update({key:diff})

    # delta.sort()

    count = 0
    for key,val in delta.items():
        if key[0] == 'lineitem' and key[2] == 'l_extendedprice':
            count += 1
    print(f"Delta Count {count}")

    # print(sorted(delta.items()))

    dest_dir = os.path.dirname(access_counter_path)
    
    #Write the output to a file
    with open(os.path.join(dest_dir,"delta.csv"),"w") as file:
        for key,val in delta.items():
            file.write(f"{key[0]},{key[1]},{key[2]},{val[0]},{val[1]},{val[2]},{val[3]},{val[4]}\n")

    #Plot figures for every single column you can find
    #Plot each of the 4 main counters against chunk index
    column_wise_pont = dict()
    column_wise_sequ = dict()
    column_wise_mono = dict()
    column_wise_rand = dict()
    column_wise_dict = dict()
    column_wise_cidx = dict()

    for key,val in delta.items():
        if (key[0],key[1],key[2]) not in column_wise_pont.keys():
            # column_wise_cidx.update({(key[0],key[1],key[2]):[int(key[1])]})
            column_wise_pont.update({(key[0],key[1],key[2]):[int(val[0])]})
            column_wise_sequ.update({(key[0],key[1],key[2]):[int(val[1])]})
            column_wise_mono.update({(key[0],key[1],key[2]):[int(val[2])]})
            column_wise_rand.update({(key[0],key[1],key[2]):[int(val[3])]})
            column_wise_dict.update({(key[0],key[1],key[2]):[int(val[4])]})
        else:
            # column_wise_cidx[(key[0],key[1],key[2])].append(int(key[1]))
            column_wise_pont[(key[0],key[1],key[2])].append(int(val[0]))
            column_wise_sequ[(key[0],key[1],key[2])].append(int(val[1]))
            column_wise_mono[(key[0],key[1],key[2])].append(int(val[2]))
            column_wise_rand[(key[0],key[1],key[2])].append(int(val[3]))
            column_wise_dict[(key[0],key[1],key[2])].append(int(val[4]))
            # print("Came here")

    # print(f"CIDX {len(column_wise_cidx['lineitem',915,'l_extendedprice'])}")

    # print(column_wise_sequential)
    # print(len(column_wise_cidx))
    colmap = {'point':column_wise_pont,
              'sequential':column_wise_sequ,
              'monotonic':column_wise_mono,
              'random':column_wise_rand,
              'dictionary':column_wise_dict}

    if is_plot:  
        for key in column_wise_cidx.keys():

            for indices,access_counts in colmap.items():

                plt.figure()
                x_axis = column_wise_cidx[key]
                y_axis = access_counts[key]

                if(sum(y_axis)) == 0:
                    print(f"Skipping {key[1]} {indices}")
                    continue
                
                if len(x_axis) != len(y_axis):
                    print(f"mismatch length")
                    exit(1)

                plt.scatter(x_axis,y_axis,s=1)

                plt.savefig(f"{dest_dir}/plt_{key[1]}_{indices}.png")
                plt.close()

    column_wise_alll = dict()

    for key,val in delta.items():
        column_wise_alll.update({key:val[0]+val[1]+val[2]+val[3]})

    for key,chunk_idx in column_wise_cidx.items():

        column_wise_alll.update({key[1]:[]})
        for idx in chunk_idx:
            print(f"Col: {key}")
            print(f"Idx: {idx}")
            print(f"{len(column_wise_pont[key])}")
            print(f"{len(column_wise_sequ[key])}")
            print(f"{len(column_wise_mono[key])}")
            print(f"{len(column_wise_rand[key])}")
            column_wise_alll[key[1]].append(column_wise_pont[key][idx]+column_wise_sequ[key][idx]+column_wise_mono[key][idx]+column_wise_rand[key][idx])

        if is_plot:
            plt.figure()
            x_axis = chunk_idx
            y_axis = column_wise_alll[key[1]]
            if(sum(y_axis)) == 0:
                print(f"Skipping {key[1]} {indices}")
                continue
            plt.scatter(x_axis,y_axis,s=1)
            plt.savefig(f"{dest_dir}/plt_{key[1]}_alll.png")
            plt.close()

    #Generate columnwise statistics
    column_wise_counters = dict()
    for key,val in delta.items():
        if key[2] not in column_wise_counters:
            column_wise_counters.update({key[2]:[0,0,0,0,0]})
            column_wise_counters[key[2]] = [x+y for x,y in zip(column_wise_counters[key[2]],val)]
        else:
            column_wise_counters[key[2]] = [x+y for x,y in zip(column_wise_counters[key[2]],val)]
    
    with open(f"{dest_dir}/column_wise_data.csv","w") as file:
        for key,val in column_wise_counters.items():
            file.write(f"{key},{val[0]},{val[1]},{val[2]},{val[3]},{val[4]},{sum(val[:4])}\n")    

