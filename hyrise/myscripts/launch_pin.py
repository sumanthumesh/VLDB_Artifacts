import subprocess

if __name__ == '__main__':
    print(f"Hello World")

    with open(f"launch_.txt","w") as file:
        print(f"Hello World")

    #Check for the hyrise process
    result = subprocess.run("pgrep -n hyrise",shell=True,capture_output=True)
    lines = result.stdout.splitlines()
    lines.sort()

    pid = int(lines[0])

    pin_dir = f"/data1/sumanthu/pin"
    pin = f"{pin_dir}/pin"
    pintool = f"{pin_dir}/source/tools/Memory/obj-intel64/dcache_hyrise.so"

    #Launch the pintool with this pid
    print(f"Attaching to {pid}")
    pin_command = f"{pin} -pid {pid} -t {pintool} -tl -ts -a 16 -b 64 -c 65536"
    subprocess.run(pin_command,shell=True)

