import subprocess
import os

if __name__ == '__main__':
   
    #Check for the hyrise process
    result = subprocess.run("pgrep -n hyrise",shell=True,capture_output=True)
    lines = result.stdout.splitlines()
    lines.sort()

    pid = int(lines[0])

    pin_dir = f"/data1/sumanthu/pin"
    pin = f"{pin_dir}/pin"
    pintool = f"{pin_dir}/source/tools/Memory/obj-intel64/dcache_hyrise.so"

    oneapi_dir = "/opt/intel/oneapi"
    vtune_dir = f"{oneapi_dir}/vtune/latest"
    vtune_bin = "vtune"

    #Set vtune and oneapi vars
    subprocess.run(f"bash -c \'source {oneapi_dir}/setvars.sh\'",shell=True)

    #Launch vtune with the hyrise pid
    dest_dir = f"vtune_{pid}"
    if os.path.exists(dest_dir):
        subprocess.run(f"rm -r {dest_dir}",shell=True)
    print(f"Attaching to {pid}")
    vtune_command = f"{vtune_bin} -collect memory-access -r {dest_dir} -target-pid={pid} &"
    subprocess.run(vtune_command,shell=True)

