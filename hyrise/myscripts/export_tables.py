import sys
import os

table_list = [
    "region",
    "customer",
    "nation",
    "lineitem",
    "supplier",
    "part",
    "orders",
    "partsupp"
]

if __name__ == '__main__':
    scaling_factor_raw = float(sys.argv[1])
    if scaling_factor_raw < 1:
        scaling_factor = scaling_factor_raw
    else:
        scaling_factor = int(scaling_factor_raw)

    if len(sys.argv) == 3:
        prefix = sys.argv[2]
    else:
        prefix = f"table_{scaling_factor}_"

    if not os.path.exists("./exported_tables"):
        os.mkdir("exported_tables")

    with open(f"export_table_{scaling_factor}.sql","w") as file:
        file.write(f"generate_tpch {scaling_factor}\n")
        for table in table_list:
            file.write(f"export {table} {os.path.abspath(os.path.join('exported_tables',prefix+table+'.bin'))}\n")
    #This python file will generate a script called