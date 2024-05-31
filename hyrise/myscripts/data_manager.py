import pulp
import numpy as np
import sys

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
        lines = file.readlines()

    for line in lines:
        column_name = line.split(',')[0]
        counts = int(line.split(',')[1])
        hotness.update({column_name: counts})

    return hotness


def read_column_sizes_file(filename):
    sizes = dict()

    with open(filename) as file:
        lines = file.readlines()

    for line in lines:
        column_name = line.split(',')[0]
        size = int(line.split(',')[1])
        sizes.update({column_name: size})

    return sizes


def get_amat(x, accesses_np, lat_dam, lat_cxl):
    return sum([accesses_np[i]*lat_dam if x[i] == False else accesses_np[i]*lat_cxl for i in range(len(accesses_np))])


if __name__ == '__main__':

    hotness_file = sys.argv[1]
    size_file = sys.argv[2]
    dam_gb = float(sys.argv[3])
    cxl_gb = float(sys.argv[4])

    # Get a hold of the hotness data
    hotness = read_hotness_data(hotness_file)

    # Get a hold of avg width of each row

    # Get a hold of the column sizes
    column_sizes = read_column_sizes_file(size_file)

    # Get a list of column names
    column_names = column_sizes.keys()
    num_columns = len(column_sizes)

    # Define the CXL and DAM sizes in bytes
    size_cxl = int(cxl_gb * 2**30)
    size_dam = int(dam_gb * 2**30)

    # Latencies
    lat_cxl = 270
    lat_dam = 45

    # Cost per page
    cost_per_page = 700
    cache_lize_size = 64
    page_size = 2**12

    # Get numpy arrays of column size and hotness
    # Multiply number of accesses by the avg size of each row and divide by 64 to get number of cache lines accessed
    # Its an imprecise thing, but we want to see if it works
    # At the end of the day, accesses_np gives us the number of cache lines accessed i.e., each access in accesses_np is cache_line bytes wide
    accesses_np = np.array([hotness[col] * tpch_row_widths[col] /
                           cache_lize_size if col in hotness.keys() else 0 for col in column_names])
    sizes_np = np.array([val for val in column_sizes.values()])

    print(accesses_np)
    print(np.sum(sizes_np))

    # Initial Mapping
    # For now map everything to CXL
    init_x_i = [True for i in range(len(accesses_np))]

    # Define the problem
    prob = pulp.LpProblem("Data_Manager", pulp.LpMinimize)

    # Create binary vector, x[i] is 1 if ith column is in CXL, 0 if it is in DAM
    x_i = pulp.LpVariable.dicts("x_i", range(num_columns), cat=pulp.LpBinary)
    # Create complement of x_i
    complement_x_i = pulp.LpVariable.dicts(
        "complement_x_i", range(len(x_i)), cat=pulp.LpBinary)
    # Constraint to mode x_i and complement_x_i as binary complements
    for i in range(len(x_i)):
        prob += complement_x_i[i] + x_i[i] == 1
  
    # # Auxillary variable y_i to model absolute diff between x_i and init_x_i
    # y_i = pulp.LpVariable.dicts("y_i", range(num_columns), cat=pulp.LpBinary)

    # # Constraint to model y_i as the abs difference between x_i and init_x_i
    # for i in range(len(x_i)):
    #     prob += y_i >= init_x_i[i] - x_i[i]
    #     prob += y_i >= x_i[i] - init_x_i[i]
    
    # Objective function
    # Consists of two terms
    # 1. Cost of the transfer - Total size of the stuff we need to move times the latency for this move
    #We cant use abs method as an affine transform, so intriduce auxillary varible y
    # transfer_cost = pulp.lpSum(
    #     [y_i[i] * sizes_np[i] for i in range(len(accesses_np))])/(page_size/cache_lize_size) * cost_per_page
    # 2. The potential benefit we might see = AMAT * size of accesses
    amat = pulp.lpSum([accesses_np[i] * lat_cxl * x_i[i] + accesses_np[i]
                      * lat_dam * complement_x_i[i] for i in range(len(accesses_np))])/np.sum(accesses_np)
    # Therefore the objective function to mimize is
    # prob += transfer_cost - amat * np.sum(accesses_np)
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
    status = prob.solve()

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
        for idx,col_name in enumerate(column_names):
            file.write(f"{col_name},{int(temp[idx][1])}\n")

    cxl_cols_size = 0
    dam_cols_size = 0
    for idx, col_name in enumerate(column_names):
        if temp[idx][1] == 1:
            cxl_cols.append(col_name)
            cxl_cols_size += sizes_np[idx]
        else:
            dam_cols.append(col_name)
            dam_cols_size += sizes_np[idx]

    # for idx,col_name in enumerate(column_names):
        # print(f"{col_name}:{temp[idx][1]}")

    print(f"CXL: {cxl_cols}")
    print(f"DAM: {dam_cols}")

    print(f"CXL: {cxl_cols_size/2**30}")
    print(f"DAM: {dam_cols_size/2**30}")

    print(pulp.LpStatus[status])