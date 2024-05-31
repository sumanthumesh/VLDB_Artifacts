import sys
import subprocess

def remove_roi(filename):
    
    print(f"Processing {filename}")

    result = subprocess.run(f"grep -n ROI {filename}",shell=True,capture_output=True,text=True)
    if len(result.stdout.splitlines()) == 0:
        print(f"ROI already removed")
        return
    elif len(result.stdout.splitlines()) == 2:
        print(f"Will remove outside ROI")
    else:
        print(f"Incorrect ROI {result.stdout.splitlines()}")
    
    result = subprocess.run(f"grep -n startROI {filename}",shell=True,capture_output=True,text=True)
    startROI = [x.split(':')[0].strip() for x in result.stdout.splitlines()]
    if len(startROI) != 1:
        print(f"Expected one startROI, found {startROI}")
        # exit(1)
    if len(startROI) > 0:
        command = f'sed -i \'1,{startROI[0]}d\' {filename}'
        print(command)
        subprocess.run(command,shell=True)

    result = subprocess.run(f"grep -n endROI {filename}",shell=True,capture_output=True,text=True)
    endROI = [x.split(':')[0].strip() for x in result.stdout.splitlines()]
    if len(endROI) != 1:
        print(f"Expected one endROI, found {endROI}")
        # exit(1)
    if len(endROI):
        command = f"sed -i \'{endROI[0]},$d\' {filename}"
        print(command)
        subprocess.run(command,shell=True)

    #Remove lines before startROI
    # command = "sed -i \'1,"+startROI[0]+"d\' "+filename
    #Remove lines after endROI


if __name__ == '__main__':
    '''
    Remove the accesses before start ROI and after endROI
    '''
    trace_file = sys.argv[1]
    remove_roi(trace_file)