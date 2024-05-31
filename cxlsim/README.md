# How to compile
* Run make as   
`make`  
Compile time options available are
* OPT: Use this to specify gcc compiler optimization. Default is -O0
* SKIP_CYCLE: Set this to true if you want to enable skipping cycles to save time. Useage `SKIP_CYCLE=true`
* TRACK_LATENCY: Set this to true to dump a latency.csv file with time stamps for every stage in the lifecycle of a CXL message. Usage `TRACK_LATENCY=true`
* COPTS: Use this to specify any other parameter you want to pass to the compiler. I use it to set HOST_VC_SIZE, HOST_BUF_SIZE, DEV_VC_SIZE and DEV_BUF_SIZE. Can be used for other things

# How to run
* First split the trace into mulitple partitions to paralellize it  
`python generate_run.py <trace file> <number of workers> <name of the job> <base directory to store results>`  
* For example,  
`python generate_run.py dram.trace 1024 test_1 /home/user/simulations`
* This generates a directory `/home/user/simulations/q_test_1` and a bash script `/home/user/simulations/run_test_1.sh`
* Run the bash script to run simulation. Use `parallel` to launch multiple workers  
`parallel -j 32 < run_test_1.sh`
* To get the aggregated results  
`python merge_results.py /home/user/simulations/q_test_1`  

Use the following command  
`./cxlsim <trace_file> <max_time_for_simulation>`  
Example  
`./cxlsim dram.trace 5000000`