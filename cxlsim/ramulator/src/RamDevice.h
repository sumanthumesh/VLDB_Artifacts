#ifndef __RAMDEVICE_H
#define __RAMDEVICE_H

#include "Processor.h"
#include "Config.h"
#include "Controller.h"
#include "SpeedyController.h"
#include "Memory.h"
#include "DRAM.h"
#include "Statistics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include <functional>
#include <map>
#include <utility>
#include "CXLInterface.h"
#include <fstream>

/* Standards */
#include "Gem5Wrapper.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DSARP.h"
#include "GDDR5.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "SALP.h"
#include "ALDRAM.h"
#include "TLDRAM.h"
#include "STTMRAM.h"
#include "PCM.h"
// #include "CXLNode.h"

namespace CXL
{
    class CXLDevice;
    class CXLHost;
    class message;
}

namespace ramulator
{

    void ramulator_req_complete(Request &r);
    void direct_attached_req_complete(Request &r);

    typedef struct
    {
        bool stall, end, idle;
        int reads, writes, clks;
        CXL_IF::status_type status;
        bool is_sim_finished;
    } dramtrace_state;

    typedef struct
    {
        uint64_t start;
        uint64_t end;
    } timings;

    class RamDevice
    {
    private:
        Config *configs;
        Memory<ramulator::DDR4, Controller> *memory;
        

        // State variables for the update function
        long addr;
        Request::Type type;
        uint64_t req_id;

    public:
        CXL::CXLDevice *parent_device;
        RamDevice() = default;
        RamDevice(RamDevice&&) = default;
        //RamDevice& operator= ( const RamDevice & ) = default; 	
        RamDevice(const RamDevice&) = default;
        ~RamDevice();
        dramtrace_state state;
        CXL_IF::CXL_if_buf inp_buf;
        std::ofstream gen_trace;                                /*!< FIle to write out all the incoming requests to ramulator in order along with their time. This log will be used by a modified version of ramulator to check if timings are correct*/
        std::ofstream record_latency;                           /*!File to write down the time a request entered ramulator and the time at which it exited*/
        std::map<uint64_t, timings> timing_tracker;             /*!< Map keeping track of ramulator in and out times*/
        std::map<uint64_t, CXL::message> messages_in_ramulator; /*!< Map keeping track of messages that were sent from the host but havent been completed yet*/
        void ramulator_init();
        void initialize_buffer();
        void initialize_state();
        bool update();
        void run_trace();
        void set_parent(CXL::CXLDevice *parent_dev);
        void dump_data();
    };

    class DirectAttached
    {
    private:
        Config *configs;
        Memory<ramulator::DDR4, Controller> *memory;
        CXL::CXLHost *host;

        // State variables for the update function
        long addr;
        Request::Type type;
        uint64_t req_id;

    public:
        uint node_id;
        DirectAttached() = default;
        DirectAttached(uint node_id);
        ~DirectAttached();
        dramtrace_state state;
        CXL_IF::CXL_if_buf inp_buf;
        std::ofstream gen_trace;                    /*!< FIle to write out all the incoming requests to ramulator in order along with their time. This log will be used by a modified version of ramulator to check if timings are correct*/
        std::ofstream record_latency;               /*!<File to write down the time a request entered ramulator and the time at which it exited*/
        std::map<uint64_t, timings> timing_tracker; /*!< Map keeping track of ramulator in and out times*/
        std::map<uint64_t, Request> reqs_in_ramulator;
        void ramulator_init();
        void initialize_buffer();
        void initialize_state();
        bool update();
        void run_trace();
        void dump_data();
        void set_host(CXL::CXLHost *);
    };
}

#endif