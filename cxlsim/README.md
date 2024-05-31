# How to compile
* Run make as   
`make`  
Compile time options available are
* OPT: Use this to specify gcc compiler optimization. Default is -O0
* SKIP_CYCLE: Set this to true if you want to enable skipping cycles to save time. Useage `SKIP_CYCLE=true`
* TRACK_LATENCY: Set this to true to dump a latency.csv file with time stamps for every stage in the lifecycle of a CXL message. Usage `TRACK_LATENCY=true`
* COPTS: Use this to specify any other parameter you want to pass to the compiler. I use it to set HOST_VC_SIZE, HOST_BUF_SIZE, DEV_VC_SIZE and DEV_BUF_SIZE. Can be used for other things

# How to run
Use the following command  
`./cxlsim <trace_file> <max_time_for_simulation>`  
Example  
`./cxlsim dram.trace 5000000`