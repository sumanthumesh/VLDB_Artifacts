#include "RamDevice.h"
#include "CXLNode.h"
#include <functional>
#include <iostream>

namespace CXL
{
    extern CXLLog log;
    extern uint64_t num_reqs_completed;
    extern uint64_t num_dam_reqs;
}

using namespace ramulator;
using namespace CXL_IF;

void ramulator_req_complete(Request &r);
void direct_attached_req_complete(Request &r);

DirectAttached::DirectAttached(uint node_id)
{
    this->node_id = node_id;
    this->ramulator_init();
}

DirectAttached::~DirectAttached()
{
    delete this->configs;
    delete this->memory;
}

void DirectAttached::initialize_buffer()
{
    // inp_buf = new CXL_if_buf();
    inp_buf.buf_add(build_req(1, 0x12345680, Request::Type::READ, status_type::VALID));
    inp_buf.buf_add(build_req(2, 0x4cbd56c0, Request::Type::WRITE, status_type::VALID));
    inp_buf.buf_add(build_req(3, 0x35d46f00, Request::Type::READ, status_type::VALID));
    inp_buf.buf_add(build_req(4, 0x696fed40, Request::Type::WRITE, status_type::VALID));
    inp_buf.buf_add(build_req(5, 0x7876af80, Request::Type::READ, status_type::VALID));
    inp_buf.buf_add(build_req(6, 0x7876af80, Request::Type::READ, status_type::END));
}

void DirectAttached::initialize_state()
{
    state.stall = false;
    state.end = false;
    state.idle = false;
    state.reads = 0;
    state.writes = 0;
    state.clks = 0;
    state.status = status_type::VALID;
    state.is_sim_finished = false;

    addr = 0;
    type = Request::Type::READ;
    req_id = 0;
}

void DirectAttached::set_host(CXL::CXLHost *h)
{
    this->host = h;
}

void DirectAttached::ramulator_init()
{
    configs = new Config("ramulator/configs/DDR4-config.cfg");
    configs->add("mapping", "defaultmapping");
    configs->set_core_num(1);

    const std::string &standard = (*configs)["standard"];
    assert(standard != "" || "DRAM standard should be specified.");

    const char *trace_type = "dram";
    // trace_type++;
    if (strcmp(trace_type, "cpu") == 0)
    {
        configs->add("trace_type", "CPU");
    }
    else if (strcmp(trace_type, "dram") == 0)
    {
        configs->add("trace_type", "DRAM");
    }
    else
    {
        printf("invalid trace type: %s\n", trace_type);
        assert(false);
    }

    // string stats_out;

    // Stats::statlist.output(standard + ".stats");
    // stats_out = standard + string(".stats");

    typedef DDR4 T;
    T *spec = new DDR4((*configs)["org"], (*configs)["speed"]);
    // initiate controller and memory
    int C = configs->get_channels(), R = configs->get_ranks();
    // Check and Set channel, rank number
    spec->set_channel_number(C);
    spec->set_rank_number(R);

    std::vector<Controller<T> *> ctrls;
    for (int c = 0; c < C; c++)
    {
        DRAM<T> *channel = new DRAM<T>(spec, T::Level::Channel);
        channel->id = c;
        channel->regStats("");
        Controller<T> *ctrl = new Controller<T>(*configs, channel);
        ctrls.push_back(ctrl);
    }
    memory = new Memory<T, Controller>(*configs, ctrls);

    // Uncomment only for testing ramulator standalone
    // initialize_buffer();

    assert((*configs)["trace_type"] == "DRAM");

    // Initialize ramulator state
    initialize_state();

    // printf("Simulation done. Statistics written to %s\n", stats_out.c_str());
    // return 0;

    // Set the request related parameters
    addr = 0;
    type = Request::Type::READ;
    req_id;

    std::string filename = "trace_" + std::to_string(node_id) + ".out";
    gen_trace = ofstream(filename);
    filename = "ramulator_" + std::to_string(node_id) + ".csv";
    record_latency = ofstream(filename);
}

bool DirectAttached::update()
{
    // bool stall = false, end = false, idle = false;
    // int reads = 0, writes = 0, clks = 0;
    // long addr = 0;
    // Request::Type type = Request::Type::READ;
    // uint64_t req_id;
    // map<int, int> latencies;
    // auto read_complete = [&latencies](Request& r){latencies[r.depart - r.arrive]++;};
    // status_type status;

    // Wrapper for callback function
    //  std::function<void(Request &)> g = [this](Request &){CXL::CXLDevice::ramulator_req_notification();};

    if (state.status != status_type::END || memory->pending_requests())
    {
        if (state.status != status_type::END && !state.stall)
        {
            // end = !trace.get_dramtrace_request(addr, type);
            state.status = inp_buf.get_dramtrace_request(addr, type, req_id);
            // state.idle = state.status == status_type::IDLE ? true : false;
            // state.end = state.status == status_type::END ? true : false;
            // printf("Addr %x, Type %d\n", addr, type);
        }

        if (state.status != status_type::END) // Do not add anything to this if statement, I think the else potion of this, the set high watermark was getting triggered more often than in actual ramulator so write transactions were getting drained way too early
        {
            if (state.status != status_type::EMPTY) // Do not create a new request
            {
                Request req(addr, type, req_id, direct_attached_req_complete, host);
                req.addr = addr;
                req.type = type;
                state.stall = !memory->send(req);
                if (!state.stall)
                {
                    // Set the ramulator time for this message
                    if (reqs_in_ramulator.count(req.req_id) == 1)
                    {
                        reqs_in_ramulator[req.req_id].dam_time.start = CXL::curr_tick;
                        reqs_in_ramulator[req.req_id].dam_time.ram_start = state.clks;
                    }
#ifdef EVENTLOG
                    // printf("Rcvd Mem Req, Addr %x, Type %d, ID %lu\n", req.addr, req.type, req.req_id);
                    CXL::log.CXLEventLog("DAM rcvd memory request " + req.sprint() + "\n", node_id);
#endif
                    char temp[100];
                    sprintf(temp, "0x%08x", addr);
                    if (type == Request::Type::READ)
                        gen_trace << req_id << " " << temp << " "
                                  << "R"
                                  << " " << state.clks << "\n";
                    else
                        gen_trace << req_id << " " << temp << " "
                                  << "W"
                                  << " " << state.clks << "\n";
                    // Update timing tracker
                    timings t = {state.clks, 0};
#ifdef DUMP
                    timing_tracker.insert({req.req_id, t});
#endif
                    if (type == Request::Type::READ)
                        state.reads++;
                    else if (type == Request::Type::WRITE)
                        state.writes++;
                }
            }
        }
        else
        {
            memory->set_high_writeq_watermark(0.0f); // make sure that all write requests in the
                                                     // write queue are drained
        }

        memory->tick();
        state.clks++;
        // printf("Ramulator tick %d\n",state.clks);
        Stats::curTick++; // memory clock, global, for Statistics
        return false;
    }
    return true;
}

void DirectAttached::run_trace()
{
    while (!state.is_sim_finished)
    {
        // Request req(0x0000000, Request::Type::READ, req_id, ramulator_req_complete);
        state.is_sim_finished = this->update();
        // req_id++;
    }
    printf("CurTick %lu\n", Stats::curTick);
}

//! Function called when direct attached memory completes a request
void ramulator::direct_attached_req_complete(Request &r)
{
#ifdef EVENTLOG
    CXL::log.CXLEventLog("Memory access completed by DAM " + r.sprint() + "\n");
#endif
    // Increment the req completed counters to help stop simulation correctly
    CXL::num_reqs_completed++;
    CXL::num_dam_reqs++;
    if (CXL::num_reqs_completed % 100000 == 0)
        std::cout << CXL::num_reqs_completed << " reqs completed!\n";
    auto& entry = r.req_host->amat_per_table_dam[(r.addr >> 48) & 0xFFF];
    auto& count = entry.first;
    auto& avg = entry.second;
    ++count;
    avg = avg + (CXL::curr_tick - r.req_host->reqs_in_dam[r.req_id] - avg) / count;
#ifdef TRACK_LATENCY
    // Dump the latency onto file
    r.req_host->print_direct_attached_latency(r.req_id);
#endif
    r.req_host->reqs_in_dam.erase(r.req_id);
#ifdef DUMP
    // Set the tick of completion
    r.req_host->DAM->timing_tracker[r.req_id].end = r.req_host->DAM->state.clks;
#endif
}

//! Write out all the individual ramulator timings
void DirectAttached::dump_data()
{
    for (std::map<uint64_t, timings>::iterator it = timing_tracker.begin(); it != timing_tracker.end(); ++it)
    {
        record_latency << std::to_string(it->first) << ","
                       << std::to_string(it->second.start) << ","
                       << std::to_string(it->second.end) << "\n";
    }
}