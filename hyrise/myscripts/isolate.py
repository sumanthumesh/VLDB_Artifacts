import sys
import os
import subprocess
import math

if __name__ == '__main__':
    range_file = sys.argv[1]
    pin_file = sys.argv[2]

    num_threads = 1 if len(sys.argv) <= 3 else int(sys.argv[3])

    if not os.path.exists(range_file):
        print(f"Cannot access {range_file}")
        exit(1)
    elif not os.path.exists(pin_file):
        print(f"Cannot access {pin_file}")
        exit(1)

    # First remove the memory accessed or other lines that are before or after ROI
    # Just need to delete first 2 and last 2 lines
    result = subprocess.run(
        f"grep -n \"ROI\" {pin_file}", shell=True, capture_output=True, text=True)
    split_result = result.stdout.splitlines()
    print(split_result)
    if len(split_result) == 0:
        print("ROI already taken care of")
    elif len(split_result) == 2:
        roi_start = int(split_result[0].split(':')[0])
        roi_end = int(split_result[1].split(':')[0])
        print("Deleting region outside ROI")
        subprocess.run(
            f"sed -i \'1,{roi_start}d; {roi_end},$d\' {pin_file}", shell=True)
    else:
        print("Incorrect ROI")
        exit(1)

    # First get the number of lines in the pinfile
    result = subprocess.run(
        f"cat {pin_file} | wc -l", shell=True, capture_output=True)
    num_lines = int(result.stdout.splitlines()[0])

    lines_per_worker = math.ceil(num_lines/num_threads)

    # Before splitting the files, make sure all other files of similar names are removed
    output_prefix = "temp_out_"
    print(f"Removing {output_prefix}* files ")
    subprocess.run(f"rm -f {output_prefix}*", shell=True)

    # Split the pin file into multiple files
    subprocess.run(
        f"split -l {lines_per_worker} {pin_file} {output_prefix}", shell=True)

    # Scan the directory for all files with matching name
    all_files = os.listdir(".")

    temp_src_files = [
        file for file in all_files if file.startswith(f"{output_prefix}")]

    print(temp_src_files)

    # Before having the c++ code generate new output files, make sure to delete the others that were there before it
    cpp_prefix = "iso_part_"
    print(f"Removing {cpp_prefix}* files ")
    subprocess.run(f"rm -f {cpp_prefix}*", shell=True)

    # We need to send all of these to the c++ program
    command = f"./a.out {range_file} "
    for t in sorted(temp_src_files):
        command += f"{t} "
    print(f"Final C++ call\n{command}")
    result = subprocess.run(command, shell=True)

    # Once processed we need to recombine all these files into one single isolate_<pid>.trace file
    # Extract pid from the ranges file
    pid = range_file.split('_')[-1].split('.')[0]
    out_file = f"isolate_{num_threads}_{pid}.trace"

    # Combining the processed files
    print("Combining the processed files")
    # all_files = sorted(os.listdir("."))
    # cpp_out_files = [file for file in all_files if file.startswith(cpp_prefix)]
    command = "cat "
    # print(cpp_out_files)
    for t in range(num_threads):
        command += f"{cpp_prefix}{t} "
    command += f"> {out_file}"
    print("Recombining files")
    print(command)
    subprocess.run(command, shell=True)

    # Once all the files are combined remove all the temporary files we generated
    print("Removing all the intermediate files")
    subprocess.run(f"rm -f {cpp_prefix}* {output_prefix}*", shell=True)
