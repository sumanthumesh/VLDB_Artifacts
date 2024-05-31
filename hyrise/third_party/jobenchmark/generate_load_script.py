import os
import sys

if __name__ == '__main__':
    csv_dir = sys.argv[1]

    #get a list of all 21 tables
    all_files = os.listdir(csv_dir)

    #Fileter out to get the .csv.json files
    json_files = []
    for file in all_files:
        if '.csv.json' in file:
            json_files.append(file)

    commands = []

    for table_file in json_files:
        table_name = table_file.replace('.csv.json','')
        command = f"load {os.path.join(os.path.abspath(csv_dir),table_name+'.csv')} {table_name} Dictionary\n"
        commands.append(command)

    with open('load_job.sql','w') as file:
        file.writelines(commands)