#!/bin/bash

# Directory containing the traces
directory="/data1/nliang/split/q_11"
# Remote destination in the format user@host:path
remote_destination1="nliang@mbit3.eecs.umich.edu:/z/nliang/split/q_11"
remote_destination2="nliang@mbit6.eecs.umich.edu:/data1/nliang/split/q_11"
remote_destination3="nliang@mbit5.eecs.umich.edu:/data1/nliang/split/q_11"
# Change to the directory with the traces
cd "$directory"

# Get the list of files and move one third of them
total_files=$(ls | wc -l)
files_to_move=$((total_files / 3))

# For second third, skip the first third of files
second_third_start=$((files_to_move + 1))
second_third_end=$((2 * files_to_move))

# For third third, start after the second third
third_third_start=$((2 * files_to_move + 1))
third_third_end=$total_files

for file in $(ls | head -n $files_to_move); do
    scp -i /data1/nliang/sshkey "$file" "$remote_destination1"
done
# Choose which third to move, e.g., for the second third
for file in $(ls | sed -n "${second_third_start},${second_third_end}p"); do
    scp -i /data1/nliang/sshkey $file $remote_destination2
    # Optionally, move the file to a 'moved' directory or delete it after successful copy
done

# Uncomment and modify the loop to move the third third
for file in $(ls | sed -n "${third_third_start},${third_third_end}p"); do
   scp -i /data1/nliang/sshkey $file $remote_destination3
   # Optionally, move the file to a 'moved' directory or delete it after successful copy
done