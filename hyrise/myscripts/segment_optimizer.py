import pulp
import numpy as np
import sys
import time
import csv

tpch_row_widths = {
    "c_custkey": 4,
    "c_name": 18.0,
    "c_address": 24.97017001545595,
    "c_nationkey": 4,
    "c_phone": 15.0,
    "c_acctbal": 4,
    "c_mktsegment": 9.0,
    "c_comment": 72.27142513653695,
    "o_orderkey": 4,
    "o_custkey": 4,
    "o_orderstatus": 1.0,
    "o_totalprice": 4,
    "o_orderdate": 10.0,
    "o_orderpriority": 8.4,
    "o_clerk": 15.0,
    "o_shippriority": 4,
    "o_comment": 48.634055909083074,
    "l_orderkey": 4,
    "l_partkey": 4,
    "l_suppkey": 4,
    "l_linenumber": 4,
    "l_quantity": 4,
    "l_extendedprice": 4,
    "l_discount": 4,
    "l_tax": 4,
    "l_returnflag": 1.0,
    "l_linestatus": 1.0,
    "l_shipdate": 10.0,
    "l_commitdate": 10.0,
    "l_receiptdate": 10.0,
    "l_shipinstruct": 12.0,
    "l_shipmode": 4.285714285714286,
    "l_comment": 26.634523977598946,
    "p_partkey": 4,
    "p_name": 32.706686303387336,
    "p_mfgr": 14.0,
    "p_brand": 8.0,
    "p_type": 20.6,
    "p_size": 4,
    "p_container": 7.575,
    "p_retailprice": 4,
    "p_comment": 14.471827500883704,
    "ps_partkey": 4,
    "ps_suppkey": 4,
    "ps_availqty": 4,
    "ps_supplycost": 4,
    "ps_comment": 122.73784355179704,
    "s_suppkey": 4,
    "s_name": 18.0,
    "s_address": 24.94301465254606,
    "s_nationkey": 4,
    "s_phone": 15.0,
    "s_acctbal": 4,
    "s_comment": 62.29474233983287,
    "n_nationkey": 4,
    "n_name": 7.08,
    "n_regionkey": 4,
    "n_comment": 74.28,
    "r_regionkey": 4,
    "r_name": 6.8,
    "r_comment": 66.0
}


def read_hotness_data(filename):
    hotness = dict()

    with open(filename) as file:
        reader = csv.reader(file)
        for row in reader:
            key = tuple(row[:3])
            val = int(row[3])
            hotness.update({key:val})

    return hotness

def read_seg_sizes_file(filename):
    sizes = dict()

    with open(filename) as file:
        lines = file.readlines()

    for line in lines:
        column_name = line.split(',')[0]
        size = int(line.split(',')[1])
        sizes.update({column_name: size})

    return sizes

def get_row_width(seg_id):
    return tpch_row_widths[seg_id[2]]

def get_seg_size(seg_id,column_sizes):
    return column_sizes[seg_id[2]]

if __name__ == '__main__':

    hotness_file = sys.argv[1]
    size_file = sys.argv[2]
    dam_gb = float(sys.argv[3])
    cxl_gb = float(sys.argv[4])

    mode = sys.argv[5]

    if mode not in ["hyrise", "pin"]:
        print(f"Please select one option for data source. \"hyrise\" for lqp, pqp or tree similarity based data that needs to be multiplied with avg width. \"pin\" for data extracted from pintool tracing")
        exit(1)

    # Get a hold of the hotness data
    hotness = read_hotness_data(hotness_file)

    # Get a hold of avg width of each row

    # Get a hold of the column sizes
    seg_sizes = read_seg_sizes_file(size_file)

    # Get a list of column names
    # column_names = column_sizes.keys()
    seg_ids = hotness.keys()
    # num_columns = len(column_sizes)
    num_segs = len(seg_ids)

    # Define the CXL and DAM sizes in bytes
    size_cxl = int(cxl_gb * 2**30)
    size_dam = int(dam_gb * 2**30)

    # Latencies
    lat_cxl = 210
    lat_dam = 30

    # Cost per page
    cache_lize_size = 64

    # Get numpy arrays of column size and hotness
    # Multiply number of accesses by the avg size of each row and divide by 64 to get number of cache lines accessed
    # Its an imprecise thing, but we want to see if it works
    # At the end of the day, accesses_np gives us the number of cache lines accessed i.e., each access in accesses_np is cache_line bytes wide
    
    #Accesses need to be multipled by row width if we're going for hotness taken from hyrise counters or any hyrise sources
    #If they are from pintrace then we don't need to multiply with row widths
    if mode == "hyrise":
        accesses_np = np.array([hotness[seg_id] * get_row_width(seg_id) /
                           cache_lize_size if seg_id in hotness.keys() else 0 for seg_id in seg_ids])
    elif mode == "pin":
        accesses_np = np.array([hotness[seg_id] * get_row_width(seg_id) * cache_lize_size for seg_id in seg_ids])
    else:
        print(f"Invalid mode {mode}")
        exit(1)

    sizes_np = np.array([get_seg_size(seg_id,seg_sizes) for seg_id in seg_ids])

    # print(accesses_np)
    print(np.sum(sizes_np))

    # Initial Mapping
    # For now map everything to CXL
    init_x_i = [True for i in range(len(accesses_np))]

    # Define the problem
    prob = pulp.LpProblem("Data_Manager", pulp.LpMinimize)

    # Create binary vector, x[i] is 1 if ith column is in CXL, 0 if it is in DAM
    x_i = pulp.LpVariable.dicts("x_i", range(len(seg_ids)), cat=pulp.LpBinary)
    # Create complement of x_i
    complement_x_i = pulp.LpVariable.dicts(
        "complement_x_i", range(len(x_i)), cat=pulp.LpBinary)
    # Constraint to mode x_i and complement_x_i as binary complements
    for i in range(len(x_i)):
        prob += complement_x_i[i] + x_i[i] == 1
  
    amat = pulp.lpSum([accesses_np[i] * lat_cxl * x_i[i] + accesses_np[i]
                      * lat_dam * complement_x_i[i] for i in range(len(accesses_np))])/np.sum(accesses_np)
    
    prob += amat
    # prob += pulp.lpSum([accesses_np[i] * lat_cxl * x_i[i] + accesses_np[i] * lat_dam *
    #    complement_x_i[i] for i in range(len(accesses_np))])/np.sum(accesses_np)

    # Constraints
    # CXL size
    prob += pulp.lpSum([sizes_np[i] * x_i[i]
                       for i in range(len(sizes_np))]) <= size_cxl
    # DAM size
    prob += pulp.lpSum([sizes_np[i] * complement_x_i[i]
                       for i in range(len(sizes_np))]) <= size_dam


    # Solve the problem
    clk_id = time.CLOCK_PROCESS_CPUTIME_ID
    start = time.clock_gettime_ns(clk_id)
    status = prob.solve()
    end = time.clock_gettime_ns(clk_id)

    print(f"OBJ: ", pulp.value(pulp.lpSum([accesses_np[i] * lat_cxl if x_i[i] ==
          True else accesses_np[i] * lat_dam for i in range(len(accesses_np))])/np.sum(accesses_np)))
    # print(f"X: ",pulp.value(sizes_np[2]))

    # Print the solution
    temp = dict()
    for var in prob.variables():
        # print(type(var.get_value()))
        print(f"{var.name} = {pulp.value(var)}")  # Adjust format if needed
        if 'complement' in var.name:
            continue
        else:
            var_idx = int(var.name.split('_')[2])

        temp[var_idx] = (var.name, pulp.value(var))

    cxl_cols = []
    dam_cols = []

    # for i in range(len(sizes_np)):
    #     print(f"{i}:{temp[i]}")

    # for i in range(len(temp)):
    #     print(f"{int(temp[i][1])}", end=",")
    # print("")

    with open("mapping.csv","w") as file:
        for idx,seg_id in enumerate(seg_ids):
            file.write(f"{seg_id},{int(temp[idx][1])}\n")

    cxl_cols_size = 0
    dam_cols_size = 0
    for idx, seg_id in enumerate(seg_ids):
        if temp[idx][1] == 1:
            cxl_cols.append(seg_id)
            cxl_cols_size += sizes_np[idx]
        else:
            dam_cols.append(seg_id)
            dam_cols_size += sizes_np[idx]

    # for idx,col_name in enumerate(column_names):
        # print(f"{col_name}:{temp[idx][1]}")

    # print(f"CXL: {cxl_cols}")
    # print(f"DAM: {dam_cols}")

    with open('DAM_segments.txt',"w") as file:
        for seg in dam_cols:
            file.write(f"{seg}\n")

    print(f"CXL: {cxl_cols_size/2**30}")
    print(f"DAM: {dam_cols_size/2**30}")

    print(pulp.LpStatus[status])

    print(f"{end-start}@@@")