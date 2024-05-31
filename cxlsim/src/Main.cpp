#include "CXLNode.h"
#include "CXLSwitch.h"
#include "CXLSys.h"
#include "CXLParams.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <unistd.h>

using namespace CXL;

namespace CXL
{
    int64_t curr_tick = 0;
    CXLParams params;
    CXLLog log("Event.log");
    uint64_t num_reqs_completed = 0; /*!< Variable to store how many requests sent out by the host have been completed till now. Includes both DAM and CXLDevice requests*/
    uint64_t num_dam_reqs = 0;
    #ifdef TRACK_LATENCY
        std::string latency_file_prefix;
    #endif
}

int main(int argc, char *argv[])
{
    // Declare variables to hold the command line arguments
    std::string trace_file;
    int64_t run_for_n_ticks = 0;
    int64_t num_reqs;
    std::string query_id;
    std::string base_dir;

    std::cout<<"Number of arguments "<<argc<<"\n";

    // Handle the command line args
    switch (argc)
    {
    case 3: // Specifying trace file and number of ticks
        trace_file = argv[1];
        query_id = argv[2];
        base_dir = ".";
        #ifdef TRACK_LATENCY
            latency_file_prefix = "latency";
        #endif
        // num_reqs = 65536;
        break;
    case 4: //Specifying trace file, number of ticks and output csv file prefix
    #ifdef TRACK_LATENCY
        trace_file = argv[1];
        run_for_n_ticks = (int64_t)stol(argv[2]);
        latency_file_prefix = argv[3];
        break;
    #else
        trace_file = argv[1];
        query_id = argv[2];
        base_dir = argv[3];
        break;
    #endif
    default:
        CXLAssert(false, "Invalid number of arguments");
    }

    std::ifstream fin(trace_file);
    num_reqs = std::count(std::istream_iterator<char>(fin >> std::noskipws), {}, '\n');

    params.spec_bandwidth = (int64_t)4 << 30;
    params.bytes_per_slot = 16;
    params.bytes_per_flit = 68;
    params.slots_per_flit = 4;
    params.ticks_per_ns = 10;
    params.link_width = 16;
    params.cxl_bus_total_latency_ns = 15;
    params.delay_tx_buf_to_bus_ns = 11;
    params.delay_rx_buf_to_unpack_ns = 11;
    params.delay_vc_to_pack_ns = 2;
    params.ramulator_update_delay_ns = 0.625; // 3200 MT/s or frequency of 1600 MT/s
    params.delay_vc_to_ramulator_ns = 2;
    params.delay_vc_to_retire_ns = 2;
    params.delay_cxl_noc_switch_ns = 10;
    params.delay_cxl_port_switch_ns = 13;
    params.ticks_per_ins = 1 * params.ticks_per_ns; //Defaulted it to 1 instruction per 1ns on a 1GHz machine
    // params.ticks_per_ins = 1; // Defaulted it to 1 instruction per 1ns on a 1GHz machine

    params.recalculate();
    params.print();

    // Main simulation engine

    std::vector<std::pair<uint64_t, uint64_t>> address_interval;
    std::vector<std::pair<uint64_t, uint64_t>> address_interval_DAM;
    // address_interval.push_back({0x00000000, 0xffffffff});
    address_interval_DAM.push_back({0x000000000000000, 0xaffffffffffffff});
    address_interval.push_back({0xb00000000000000, 0xcffffffffffffff});
    //address_interval.push_back({0xb00000000000000, 0xbffffffffffffff});
    // address_interval.push_back({0x20000000,0x30000000});
    // address_interval.push_back({0x30000000,0xffffffff});
    std::vector<uint64_t> device_host;
    device_host.push_back(0); // device 0 is a subordinate of host 0
    //device_host.push_back(0);
    // device_host.push_back(0);
    // device_host.push_back(0);
    CXLSystem cxl(1, 1, 1, address_interval_DAM, address_interval, device_host);
    CXLHost &host0 = cxl.hosts[0];
    host0.set_trace_file(trace_file);

    printf("Credits: %d,%d,%d\n", cxl.hosts[0].int_cred.data_credit, cxl.hosts[0].int_cred.req_credit, cxl.hosts[0].int_cred.rsp_credit);

    // std::string base_dir = "/data1/sumanthu/simulations/";

    int64_t inactive_cycle_count = 0;
    std::string output_latency_file = base_dir + "/" + query_id + "/latency_" + std::to_string(getpid()) + ".csv";
    std::cout << output_latency_file << "\n";
    std::ofstream outputFile(output_latency_file);
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open file " << output_latency_file << " for writing." << std::endl;
        return 1;
    }
    while (true)
    {
        cxl.update();
        curr_tick++;
        if(cxl.hosts[0].no_active_transaction())
            inactive_cycle_count++;
        if (cxl.hosts[0].messages_sent_to_device.size() == 0 && cxl.hosts[0].reqs_in_dam.size() == 0 && num_reqs_completed >= num_reqs)
        {
            printf("%d, %d\n", cxl.hosts[0].messages_sent_to_device.size(), cxl.hosts[0].reqs_in_dam.size());
            for(const auto& pair : cxl.hosts[0].amat_per_table_dam) {
                outputFile << std::fixed << 0 << ", " << pair.first << ", " << pair.second.first << ", " << pair.second.second << std::endl;
            }
            for(const auto& pair : cxl.hosts[0].amat_per_table_cxl) {
                outputFile << std::fixed << 1 << ", " << std::hex << pair.first << ", " << pair.second[0] << ", " << pair.second[1] << ", " << pair.second[2] << std::endl;
            }
            // Closing the file
            outputFile.close();
            break;
        }
    }

    // Print the skipped cycles for host
    cxl.hosts[0].print_skipped_cycles();
    std::cout << "DAM completed " << CXL::num_dam_reqs << "\n";
    for (int i = 0; i < cxl.devices.size(); i++)
    {
        cxl.devices[i].print_skipped_cycles();
    }

    std::cout << "Break 9\n";
    // Close log file
    CXL::log.eventlog.close();

    std::cout << "Break 10\n";
#ifdef DUMP
    // Dump data
    cxl.hosts[0].dump_data();
    // bus0.dump_data();
    // bus1.dump_data();
    // device0.access_dram()->dump_data();
    // device1.access_dram()->dump_data();
    // DAM.dump_data();
    std::cout << "Break 11\n";
#endif

#ifdef TRACK_LATENCY
    // cxl.hosts[0].dump_latency();
    // cxl.switch_.dump_latency();
#endif
    
    std::cout << "Simulation Finished at " << curr_tick << "\n";

    return 0;
}
