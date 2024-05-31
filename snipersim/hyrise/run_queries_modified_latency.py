import os
import sys

unmapped_latencies = {1:441.6874777,
                      2:880.5303428,
                      3:606.5438698,
                      4:549.1874498,
                      5:680.2337038,
                      6:1219.937506,
                      7:958.9543505,
                      8:824.3227395,
                      10:694.6756673,
                      11:1064.10564,
                      12:949.9864241,
                      13:763.0786767,
                      14:1006.515596,
                      16:777.9122981,
                      17:1723.230611,
                      18:391.5143002,
                      19:1642.562008,
                      20:1070.118156,
                      22:490.8221776}

mapped_latencies = {1:343.6774803,
                    2:325.2775075,
                    3:358.23563,
                    4:351.2180788,
                    5:434.1345404,
                    6:292.6184723,
                    7:481.4280067,
                    8:430.0453016,
                    9:552.2036476,
                    10:419.9615053,
                    11:327.9140332,
                    12:372.457507,
                    13:462.871461,
                    14:460.2098527,
                    16:414.348345,
                    17:363.8648473,
                    18:230.744035,
                    19:357.381596,
                    20:494.9819577,
                    21:442.5155731,
                    22:331.5778455}

def create_sql_file(query_path,scaling_factor,num_cores_hyrise,filename="tracing.sql"):
    with open(f"{result_dir}/{filename}","w") as file:
        file.write(f"generate_tpch {scaling_factor}\n")
        file.write(f"cores {num_cores_hyrise}\n")
        file.write(f"setting scheduler on\n")
        file.write(f"script {query_path}\n")
        file.write(f"quit\n")

if __name__ == '__main__':
    scaling_factor = sys.argv[1]
    
    num_cores_hyrise = 28
    num_cores_sniper = 32
    sniper_dir = "/data2/sumanthu/snipersim/"
    sniper_bin = f"{sniper_dir}/run-sniper"
    result_dir = f"{sniper_dir}/hyrise"
    hyrise_bin = "/data2/sumanthu/hyrise/build_debug/hyriseConsole"
    query_dir = "/data1/sumanthu/tpch_queries/"
    
    # for query_id,latency in unmapped_latencies.items():
    for query_id,latency in mapped_latencies.items():
        # dest_dir = f"unmapped_{query_id}_{scaling_factor}"
        dest_dir = f"mapped_{query_id}_{scaling_factor}"
        sql_filename = f'tracing_{query_id}.sql'
        create_sql_file(f"{os.path.join(query_dir,f'{query_id}.sql')}",scaling_factor,num_cores_hyrise,sql_filename)
        
        sniper_command = f"{sniper_bin} -d {dest_dir} --roi --no-cache-warming -n {num_cores_sniper} -v -c cascadelake -g scheduler/type=static -g perf_model/dram/latency={int(latency/10)} -- {hyrise_bin} {result_dir}/{sql_filename} &> log_mapped_{query_id}.txt"
        print(sniper_command)
