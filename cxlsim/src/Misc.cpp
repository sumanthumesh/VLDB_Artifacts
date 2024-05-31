#include "flit.h"
#include "CXLParams.h"
#include <iostream>
#include "utils.h"
// File includes all sorts of miscellaneous constructors and functions for which we dont want to have a separate file

using namespace CXL;

uint64_t CXL::message::msg_count = 0;
uint64_t CXL::flit::flit_counter = 0;

namespace CXL
{
    extern int64_t curr_tick;
}

flit_header::flit_header()
{
    slots.resize(4);
}

flit::flit()
{
    size = B68;
    slots = std::vector<slot>(4);
    for (slot &s : slots)
        s.type = empty;
    header = flit_header();
    valid = false; // make this true
    flit_id = 0;
}

void flit::copy(const flit &f)
{
    valid = f.valid;
    time = f.time;
    size = f.size;
    header = f.header;
    slots = f.slots;
    flit_id = f.flit_id;
}

void message::init_time()
{
    // Set timing parameters
    time.read_write = false; //Read by default
    time.tick_created = curr_tick;
    time.tick_unpacked = 0;
    time.tick_created = 0;
    time.tick_packed = 0;
    time.tick_transmitted = 0;
    time.tick_received = 0;
    time.tick_unpacked = 0;
    time.tick_at_ramulator = 0;
    time.tick_ramulator_complete = 0;
    time.tick_repacked = 0;
    time.tick_retransmitted = 0;
    time.tick_resp_received = 0;
    time.tick_resp_unpacked = 0;
    time.tick_req_complete = 0;
    time.ramulator_clk_start = 0;
    time.ramulator_clk_end = 0;
}

message::message(opcode oc, uint64_t addr) : opCode(oc), address(addr)
{
    init_time();
    msg_id = message::msg_count;
    msg_count++;
}

message::message(opcode oc, uint64_t addr, int dp_id, int sp_id) : opCode(oc), address(addr), dp_id(dp_id), sp_id(sp_id)
{
    init_time();
    msg_id = message::msg_count;
    msg_count++;
    time.tick_created = curr_tick;
}

// message::message(opcode oc, uint64_t addr, int dp_id, int sp_id, int64_t time) : opCode(oc), address(addr), dp_id(dp_id), sp_id(sp_id), time.tick_created(time)
// {
//     init_time();
//     msg_id = message::msg_count;
//     msg_count++;
//     // tick_created = curr_tick;
// }

void message::copy(const message &m)
{
    valid = m.valid;
    opCode = m.opCode;
    meta_field = m.meta_field;
    meta_value = m.meta_value;
    snoop_type = m.snoop_type;
    tag = m.tag;
    address = m.address;
    ld_id = m.ld_id;
    sp_id = m.sp_id;
    dp_id = m.dp_id;
    traffic_class = m.traffic_class;
    time = m.time;
    // tick_created = m.tick_created;
    // tick_unpacked = m.tick_unpacked;

    poison = m.poison;
    byte_enable_present = m.byte_enable_present;

    dev_load = m.dev_load;
    msg_id = m.msg_id;
}

slot::slot(message &msg)
{
}
slot::slot(slot_type type, message msg)
{
    this->type = type;
    this->msg = msg;
}
slot::slot()
{
}

void flit::print()
{
    for (int i = 0; i < 4; ++i)
    {
        std::cout << this->slots[i].type << "\n";
    }
}

// Return string with flit parameters
std::string flit::sprint()
{
    std::string s;
    s += "#" + std::to_string(flit_id) + " ";
    for (int i = 0; i < 4; i++)
    {
        switch (this->slots[i].type)
        {
        case slot_type::m2s_req:
            s = s + "(req - " + std::to_string(this->slots[i].msg.msg_id) + ") ";
            break;
        case slot_type::m2s_rwd_hdr:
            s = s + "(rwd - " + std::to_string(this->slots[i].msg.msg_id) + ") ";
            break;
        case slot_type::s2m_drs_hdr:
            s = s + "(drs - " + std::to_string(this->slots[i].msg.msg_id) + ") ";
            break;
        case slot_type::s2m_ndr:
            s = s + "(ndr - " + std::to_string(this->slots[i].msg.msg_id) + ") ";
            break;
        case slot_type::data:
            s = s + "(data - " + std::to_string(this->slots[i].msg.msg_id) + ") ";
            break;
        case slot_type::empty:
            s = s + "(empty) ";
            break;
        default:
            CXL_ASSERT(false && "Illegal slot type");
        }
    }
    // Add credit info
    s += " {" + std::to_string(header.credit.req_credit) + "," + std::to_string(header.credit.rsp_credit) + "," + std::to_string(header.credit.data_credit) + "} ";
    return s;
}

//! Very hacky way of getting address of a flit that has only data elements
uint64_t flit::get_first_address() const
{
    for(const slot &s: slots)
    {
        if(s.type == slot_type::empty)
            continue;
        return s.msg.address;
    }
    return 0;
}

// message::message(message& m2){
//     this->valid = m2.valid;
//     this->opCode = m2.opCode;
//     this->meta_field = m2.meta_field;
//     this->meta_value = m2.meta_value;
//     this->sncxl_bus_ticks_per_dequeueoop_type = m2.snoop_type;
//     this->tag = m2.tag;
//     this->address = m2.address;
//     this->ld_id = m2.ld_id;
//     this->sp_id = m2.sp_id;
//     this->dp_id = m2.dp_id;
//     this->traffic_class = m2.traffic_class;
//     this->poison = 	m2.poison;
//     this->byte_enable_present = m2.byte_enable_present;
//     this->dev_load = m2.dev_load;
//     this->msg_id = m2.msg_id;
// }

//! Set all params
void CXLParams::recalculate()
{
    link_bandwidth_s = spec_bandwidth * link_width;                /*!< Bytes/s*/
    link_bandwidth = link_bandwidth_s * ticks_per_ns / 1000000000; /*!< Bytes per tick*/
    bus_size = (float)link_bandwidth_s / bytes_per_flit * 1e-9 * cxl_bus_total_latency_ns;
    cxl_bus_ticks_per_dequeue = 1 / ((float)link_bandwidth_s / bytes_per_flit * 1e-9 / ticks_per_ns);
    delay_tx_buf_to_bus = delay_tx_buf_to_bus_ns * ticks_per_ns;
    delay_rx_buf_to_unpack = delay_rx_buf_to_unpack_ns * ticks_per_ns;
    delay_vc_to_pack = delay_vc_to_pack_ns * ticks_per_ns;
    delay_vc_to_ramulator = delay_vc_to_ramulator_ns * ticks_per_ns;
    cxl_bus_total_latency = cxl_bus_total_latency_ns * ticks_per_ns;
    delay_ramulator_update = ramulator_update_delay_ns * ticks_per_ns;
    delay_vc_to_retire = delay_vc_to_retire_ns * ticks_per_ns;
    delay_cxl_noc_switch = delay_cxl_noc_switch_ns * ticks_per_ns;
    delay_cxl_port_switch = delay_cxl_port_switch_ns * ticks_per_ns;
}

//! Print all CLX params;
void CXLParams::print()
{
    std::cout << "link_bandwidth: " << link_bandwidth << "\n";
    std::cout << "bus_size: " << bus_size << "\n";
    std::cout << "cxl_bus_ticks_per_dequeue: " << cxl_bus_ticks_per_dequeue << "\n";
    std::cout << "delay_tx_buf_to_bus: " << delay_tx_buf_to_bus << "\n";
    std::cout << "delay_rx_buf_to_unpack: " << delay_rx_buf_to_unpack << "\n";
    std::cout << "delay_vc_to_pack: " << delay_vc_to_pack << "\n";
    std::cout << "cxl_bus_total_latency: " << cxl_bus_total_latency << "\n";
    std::cout << "delay_ramulator_update: " << delay_ramulator_update << "\n";
}

// Print important message parameters
void message::print()
{
    std::cout << "----------------------\n";
    std::cout << "Msg id: " << msg_id << "\n";
    printf("Addr  : %08x\n", address);
    std::cout << "Type  : " << opCode << "\n";
    std::cout << "----------------------\n";
}

// get a string with important message parameters
std::string message::sprint() const
{
    char s[200];
    sprintf(s, "(%d,0x%08x,%d)", msg_id, address, opCode);
    return std::string(s);
}

//! Return true if all slots in the flit are empty
bool flit::is_flit_empty()
{
    for (slot &s : slots)
    {
        if (s.type != slot_type::empty)
            return false;
    }
    return true;
}

//! Return true if all slots in the flit are not empty
bool flit::is_flit_full()
{
    for (slot &s : slots)
    {
        if (s.type == slot_type::empty)
            return false;
    }
    return true;
}

//! Returns the count of messages with this particular opcode
int flit::slot_count_in_flit(slot_type stype)
{
    int count = 0;
    for (slot &s : slots)
    {
        if (s.type == stype)
            count++;
    }
    return count;
}

//! If input pos is 1, returns true if slots 2 and 3 are empty, false otherwise
bool flit::are_all_slots_after_this_empty(int pos)
{
    for (int i = pos + 1; i < 3; i++)
    {
        if (slots[i].type != slot_type::empty)
            return false;
    }
    return true;
}

//! Set particular timing parameter for all non empty flit slots
void flit::set_time(int64_t msg_timing::*member, int64_t value)
{
    for (int i = 0; i < 4; i++)
    {
        if (slots[i].type == slot_type::empty)
            continue;
        slots[i].msg.time.*member = value;
    }
}
