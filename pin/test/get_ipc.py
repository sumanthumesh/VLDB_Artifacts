import sys
import os
from multiprocessing import Process
import subprocess
import time

def get_pid_from_line(line):
    return int(line.split()[1])

def run_perf(pid):
    result = subprocess.run(f"perf stat -e cycles,instructions -p {pid}",shell=True,stdout=subprocess.PIPE,text=True)
    lines = result.stdout.splitlines()
    return lines


def poll_and_perf(bin_name):
    #run poller for 10s max
    end = time.time() + 10
    while time.time() < end:
        #Search for the binary name in ps
        result = subprocess.run(f"ps -xjf | grep \"{bin_name}\"",shell=True,stdout=subprocess.PIPE,text=True)
        lines = result.stdout.splitlines()
        if len(lines)==0:
            continue
        for line in lines:
            command = line.split("\_")[1].replace(" ","")
            if bin_name == command:
                pid = get_pid_from_line(line)
                print(f"Attaching to pid: {pid}")
                end = 0
                run_perf(pid)
                break

def run_program(invoke_command):
    result = subprocess.run(invoke_command,shell=True,text=True)

if __name__ == '__main__':

    invoke_command = sys.argv[1:]
    bin_name = sys.argv[1]

    executor = Process(target=run_program,args=(invoke_command,))
    poller = Process(target=poll_and_perf,args=(bin_name,))

    #start poller first
    poller.start()
    executor.start()

    executor.join()
    poller.join()