#include "CXLBus.h"
#include <iostream>
#include "utils.h"
#include "CXLNode.h"

using namespace CXL;

extern int64_t curr_tick; /*!< Global tick variable*/
extern CXLParams params;
namespace CXL
{
    extern CXLLog log;
}

//! Constructor
CXLBus::CXLBus(int size, uint node_id, direction d)
{
    this->size = size;
    this->ticks_per_dequeue = params.cxl_bus_ticks_per_dequeue;
    this->bus_latency = params.cxl_bus_total_latency;
    this->from_buf = NULL;
    this->to_buf = NULL;
    this->node_id = node_id;
    this->time_of_last_dequeue = 0;
    set_dir(d);
}

// //! Constructor specifying direction
// CXLBus::CXLBus(int size, uint node_id, direction d)
// {
//     CXLBus(size, node_id);
//     set_dir(d);
// }

//! Check if from and to nodes are connected
void CXLBus::check_connections()
{
    CXL_ASSERT(to_buf != NULL && "TO buffer not connected");
    CXL_ASSERT(from_buf != NULL && "FROM buffer not connected");
}

//!  Return true if flit has been in the bus for longer than bus latency
bool CXLBus::is_flit_stagnant()
{
    // If bus is empty, return false
    if (is_empty())
        return false;
    // Compare the time of flit at head of queue to current tick
    if (curr_tick - flit_slots.front().time.time_of_transmission > bus_latency)
        return true;
    return false;
}

//! Set from and to CXL node ids
void CXLBus::set_from_to_node_id(uint from, uint to)
{
    from_node_id = from;
    to_node_id = to;
}

//!  Connect transmitting CXL endpoint
void CXLBus::connect_from(CXLBuf<flit> *from)
{
    from_buf = from;
}

//!  Connect receiving CXL endpoint
void CXLBus::connect_to(CXLBuf<flit> *to)
{
    to_buf = to;
}

//! Update function to be called for every bus from main
void CXLBus::update()
{
    // Check bus hasn't exceeded size
    CXL_ASSERT(flit_slots.size() <= size && "Bus size exceeded");
    // Check if flit is stagnant
    CXL_ASSERT(!is_flit_stagnant() && "Stagnant flit in bus");

    // Every once in params.cxl_bus_ticks_per_dequeue ticks try to dequeue from the bus
    // This will model the bandwidth
    if (curr_tick - time_of_last_dequeue >= params.cxl_bus_ticks_per_dequeue && !is_empty())
    {
        send_to_node();
    }
}

//! Returns true if bus is full, else false
bool CXLBus::is_full()
{
    if (flit_slots.size() == size)
        return true;
    return false;
}

//! Returns true if bus is empty, else false
bool CXLBus::is_empty()
{
    if (flit_slots.size() == 0)
        return true;
    return false;
}

//! Add a flit to the bus if bus is not full.
void CXLBus::add_to_bus(flit f)
{
    CXL_ASSERT(!is_full() && "Addding to bus when it is full");

    // If bus is empty, then reset the dequeue counter
    if (is_empty())
        time_of_last_dequeue = curr_tick;

    if (!is_full())
    {
        flit_slots.push_back(f);
        // Add to timing tracker
        bus_timings time;
        time.num_data = f.slot_count_in_flit(slot_type::data);
        time.num_empty = f.slot_count_in_flit(slot_type::empty);
        time.tick_added_to_bus = curr_tick;
        time.tick_removed_from_bus = 0;
#ifdef DUMP
        timing_tracker.insert({f.flit_id, time});
#endif
        // std::cout << "Pkt added to bus @" << curr_tick << "\n";
    }
}

//! Dequeue head of bus and add to rx buf of receivng end
void CXLBus::send_to_node()
{
    if (!is_empty() && curr_tick - flit_slots.front().time.time_of_transmission >= params.cxl_bus_total_latency)
    {
        // std::cout << "Pkt removed from bus @" << curr_tick << "\n";
        flit f;
        f.copy(flit_slots.front());
        f.time.time_of_receipt = curr_tick;
        // set the messages' ticks at which it is received at either switch or device or host based on the terminals of the switch
        switch (bus_type)
        {
        case bus_terminals::host_switch:
            f.set_time(&msg_timing::tick_switch_ds_rx, curr_tick);
            break;
        case bus_terminals::switch_device:
            f.set_time(&msg_timing::tick_received, curr_tick);
            break;
        case bus_terminals::device_switch:
            f.set_time(&msg_timing::tick_switch_us_rx, curr_tick);
            break;
        case bus_terminals::switch_host:
            f.set_time(&msg_timing::tick_resp_received, curr_tick);
            break;
        case bus_terminals::host_device:
            f.set_time(&msg_timing::tick_received, curr_tick);
            break;
        case bus_terminals::device_host:
            f.set_time(&msg_timing::tick_resp_received, curr_tick);
            break;
        default:
            CXL_ASSERT(false && "Illegal CXL Bus type value");
        }
        // // set the messages' tick_received or tick_resp_received depending on upstream or downstream direction
        // switch (dir)
        // {
        // case direction::UpStream:
        //     f.set_time(&msg_timing::tick_resp_received, curr_tick);
        //     break;
        // case direction::DwStream:
        //     f.set_time(&msg_timing::tick_received, curr_tick);
        //     break;
        // default:
        //     CXL_ASSERT(false && "Illegal CXL Direction value");
        // }
#ifdef DUMP
        // Set the bus timer
        timing_tracker[f.flit_id].tick_removed_from_bus = curr_tick;
#endif
        to_buf->enqueue(f);
        flit_slots.pop_front();
        time_of_last_dequeue = curr_tick;
#ifdef EVENTLOG
        log.CXLEventLog("Bus to node " + f.sprint() + "\n", this->node_id, to_node_id);
#endif
    }
}

//! Returns +ve int showing the number of flits in the bus
int CXLBus::bus_occupancy()
{
    return flit_slots.size();
}

//! Return max size
int CXLBus::max_size()
{
    return size;
}

//! For testing purposes. Fill bus with flits to check traffic flow
void CXLBus::fill_bus(std::vector<flit> &flits)
{
    CXL_ASSERT(flits.size() < max_size() - bus_occupancy() && "Bus overflow");
    for (flit f : flits)
    {
        add_to_bus(f);
    }
}

//! Set direction. Upstream means device->host direction, downstream means host->device
void CXLBus::set_dir(direction d)
{
    dir = d;
}

//! Write out the bus related data to a file
void CXLBus::dump_data()
{
    std::ofstream dump("bus_" + std::to_string(node_id) + ".dump");
    dump << "Flit ID,tick_added_to_bus,tick_removed_from_bus,num_data,num_empty\n";
    for (std::map<uint64_t, bus_timings>::iterator it = timing_tracker.begin(); it != timing_tracker.end(); ++it)
    {
        dump << it->first << ","
             << it->second.tick_added_to_bus << ","
             << it->second.tick_removed_from_bus << ","
             << it->second.num_data << ","
             << it->second.num_empty << "\n";
    }
    dump.close();
}