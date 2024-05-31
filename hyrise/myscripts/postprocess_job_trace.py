import os
import sys

if __name__ == '__main__':
    trace_file = sys.argv[1]
    
    dest_name = os.path.join(os.path.abspath(os.path.dirname(trace_file)),'cleaned_' + os.path.basename(trace_file))
    
    fout = open(dest_name,'w')
    
    with open(trace_file) as file:
        while True:
            line = file.readline()
            if not line:
                break
            if '0xb' not in line and '0xa' not in line:
                fout.write(line)
                continue
            addr = line.split(' ')[0]
            if len(addr) >= 17:
                addr = addr[:17]
                # print(f"New addr {addr}")
            new_line = addr + ' ' + ' '.join(line.split(' ')[1:])
            fout.write(new_line)
    
    