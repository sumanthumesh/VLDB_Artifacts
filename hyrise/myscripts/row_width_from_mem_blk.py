import sys

if __name__ == '__main__':

    mem_blk_file = sys.argv[1]

    widths = dict()

    with open(mem_blk_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if "Table" in line:
                table_name = line.split(',')[0].split(':')[1].strip()
                column_name = line.split(',')[2].split(':')[1].strip()
                string_counter = 0
                string_width = 0
                continue
            if "VEC" in line or "-----" in line:
                continue
            datatype = line.split(' ')[0]
            if datatype == "Int":
                widths.update({(table_name, column_name): 4})
            elif datatype == "Float":
                widths.update({(table_name, column_name): 4})
            elif datatype == "Long":
                widths.update({(table_name, column_name): 4})
            elif datatype == "Double":
                widths.update({(table_name, column_name): 8})
            elif datatype == "String":
                string_counter += 1
                string_width += int(line.split(')')[0].split('(')[1])
                widths.update({(table_name, column_name): string_width/string_counter})

    for key, val in widths.items():
        print(f"{key[1]}:{val}")
