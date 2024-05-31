#include "CXLNode.h"
#include "message.h"
#include "flit.h"
#include <vector>
#include <cstdint>
#include "utils.h"
#include <iostream>
#include <string>

using namespace CXL;

namespace CXL
{
    extern CXLLog log;
}

// CXLDevice::CXLDevice() : CXLNode(), dram(RamDevice())
// {
//     // this->node_id = uint(0);
//     // device_init();
//     // unpacker_rollover = 0;
//     // packer_rollover = 0;
//     // NDR_packed = 0;
//     // last_ramulator_update = params.delay_ramulator_update;
//     // ramulator_inp_state = round_robin_state::R;
//     // credits crd = {0, 0, 0};
//     // ext_creds.insert({0, crd});
//     // last_transmitted_at = 0;
//     // packer_stall = false;
//     // ndr_ctr = 0;
//     // drs_ctr = 0;
//     // is_packer_waiting = false;
//     // // Initialize your own credits counter to the max size - 1 of your available NDR and DRS VCs. It is max size - 1 because by default we have given 1 credit to the host already
//     // int_cred = {M2S_Req.size() - M2S_Req.buf_occupancy() - 1, 0, M2S_RWD.size() - M2S_RWD.buf_occupancy() - 1};
//     // log.CXLEventLog("Internal Credit initialization [" + print_cred(int_cred) + "]\n", this->node_id);
//     // num_read_ports = 2;
//     // packer_wait_time = 10;
//     // started_packing_at = 0;
//     // pkr2_state = round_robin_state::R;
//     // connected_hosts.push_back(0);
//     // packer_seq_length = 0;
//     // seq_threshold = 5; // If all responses are DRS, 4 DRS and their data can be put into 5 flits without leaving any slot empty
// }

CXLDevice::CXLDevice(int vc_size, int buf_size, uint node_id) : CXLNode(vc_size, buf_size), M2S_RWD(vc_size), M2S_Req(vc_size)
{
    this->node_id = node_id;
    for (int i = 0; i < NUM_VC; i++)
    {
        S2M_NDR[i] = CXLBuf<message>(vc_size / NUM_VC);
        S2M_DRS[i] = CXLBuf<message>(vc_size / NUM_VC);
    }
    device_init();
    unpacker_rollover = 0;
    packer_rollover = 0;
    NDR_packed = 0;
    cur_DRS_vc = 0;
    cur_NDR_vc = 0;
    last_ramulator_update = params.delay_ramulator_update;
    ramulator_inp_state = round_robin_state::R;
    credits crd = {0, 0, 0};
    ext_creds.insert({0, crd});
    last_transmitted_at = 0;
    packer_stall = false;
    ndr_ctr = 0;
    drs_ctr = 0;
    is_packer_waiting = false;
    // Initialize your own credits counter to the max size - 1 of your available NDR and DRS VCs. It is max size - 1 because by default we have given 1 credit to the host already
    int_cred = {M2S_Req.size() - M2S_Req.buf_occupancy() - 1, 0, M2S_RWD.size() - M2S_RWD.buf_occupancy() - 1};
#ifdef EVENTLOG
    log.CXLEventLog("Internal Credit initialization [" + print_cred(int_cred) + "]\n", this->node_id);
#endif
    packer_wait_time = (int64_t)(0.1 * (float)params.ticks_per_ns); // 0.1ns
    started_packing_at = 0;
    pkr2_state = round_robin_state::R;
    connected_hosts.push_back(0);
    packer_seq_length = 0;
    seq_threshold = 5; // If all responses are DRS, 4 DRS and their data can be put into 5 flits without leaving any slot empty
    skippable_cycle = 0;
    empty_cycle_packer = 0;
    empty_cycle_sent_to_ram = 0;
    empty_cycle_transmit = 0;
    empty_cycle_unpacker = 0;
    num_reqs = 0;
    printf("Device, %d,%d,%d,%d\n", S2M_DRS[0].size() * S2M_DRS.size(), tx_buffer.size(), M2S_Req.size(), rx_buffer.size());
}

//! This is the main function for device. It is called once per tick
void CXLDevice::update()
{
    bool packer_flag = false,
         unpacker_flag = false,
         send_to_ram_flag = false,
         transmit_flag = false,
         ramulator_update_flag = false;
    // Do Packing
    bool packer_flag2 = !skip_packer_check();
#ifdef SKIP_CYCLE
    if (packer_flag2)
        packer_flag = FLIT_packer();
    empty_cycle_packer = packer_flag2 ? empty_cycle_packer : empty_cycle_packer + 1;
#else
    packer_flag = FLIT_packer();
#endif
    //  Do unpack
    bool unpacker_flag2 = !skip_unpacker_check();
#ifdef SKIP_CYCLE
    if (unpacker_flag2)
        unpacker_flag = FLIT_unpacker();
    empty_cycle_unpacker = unpacker_flag2 ? empty_cycle_unpacker : empty_cycle_unpacker + 1;
#else
    unpacker_flag = FLIT_unpacker();
#endif
    // Send memory transaction to ramulator
    bool send_to_ram_flag2 = !skip_send_to_ram_check();
#ifdef SKIP_CYCLE
    if (send_to_ram_flag2)
        send_to_ram_flag = send_to_ramulator();
    empty_cycle_sent_to_ram = send_to_ram_flag2 ? empty_cycle_sent_to_ram : empty_cycle_sent_to_ram + 1;
#else
    send_to_ram_flag = send_to_ramulator();
#endif

    // Update counter whenever we send to ramulator
    if (send_to_ram_flag)
        num_reqs++;

    // Transmit
    bool transmit_flag2 = !skip_transmit_check();
#ifdef SKIP_CYCLE
    if (transmit_flag2)
        transmit_flag = transmit();
    empty_cycle_transmit = transmit_flag2 ? empty_cycle_transmit : empty_cycle_transmit + 1;
#else
    transmit_flag = transmit();
#endif

    // Call ramulator update according to its speed
    // Ramulator update already has cycle skipping, no need for anything else
    // if (last_ramulator_update == 0)
    // {
    //     ramulator_update_flag = true;
    //     last_ramulator_update = params.delay_ramulator_update;
    //     access_dram()->update();
    // }

    if(curr_tick % params.delay_ramulator_update == 0)
    {
        access_dram()->update();
        // std::cout<<"CXLRAM update "<<curr_tick<<"\n";
    }

    last_ramulator_update--;

    if (!packer_flag2 && !unpacker_flag2 && !send_to_ram_flag2 && !transmit_flag2)
        skippable_cycle++;

#ifdef SKIP_CYCLE
    // It should not be such that the skip check function says cycle is skippable (i.e, false) and the actual component function itself returns true
    // Flasely assuming a cycle to be skippable is not acceptable
    CXL_ASSERT(!((!packer_flag2 && packer_flag)) && "Packer Flag Error");
    CXL_ASSERT(!((!unpacker_flag2 && unpacker_flag)) && "Unpacker Flag Error");
    CXL_ASSERT(!((!send_to_ram_flag2 && send_to_ram_flag)) && "Send to ramualtor Flag Error");
    CXL_ASSERT(!((!transmit_flag2 && transmit_flag)) && "Transmit Flag Error");
#endif
}

bool CXLDevice::check_ram2dev()
{
    return 0;
}

bool CXLDevice::check_dev2ram()
{
    return 0;
}

int CXLDevice::search_reqs_in_ramulator(Request &r)
{
    int pos = -1;
    // Make last 6 bits 0
    uint64_t new_addr = (uint64_t)r.addr & 0xffffffffffffffc0;
    for (int i = 0; i < reqs_in_ramulator.size(); i++)
    {
        if (reqs_in_ramulator[i].msg.address == new_addr)
        {
            pos = i;
        }
    }
    return pos;
}

/*!< This function will be called when ramulator finishes processing a read or write request*/
void CXLDevice::ramulator_req_notification(Request &r, CXLDevice *dev)
{
    // TODO add source code here to trigger when req is completed
    // std::cout << "Req " << r.req_id << " finished execution @" << curr_tick << " | " << access_dram()->state.clks << "\n";

    // Check if we completed a valid message, i.e., if its req id is stored in messages in ramulator
    CXL_ASSERT(messages_in_ramulator.count(r.req_id) == 1 && "Ramulator callback is for an unknown request");

    // Create copy of the Req or Rwd message by looking up the req_id
    message m = messages_in_ramulator[r.req_id];
    // Set the time at which ramulator serviced the request
    m.time.ramulator_clk_end = access_dram()->state.clks;
    m.time.tick_ramulator_complete = curr_tick;
    // Create an NDR or DRS message by modifying the copied message and add to the respective queues
    switch (m.opCode)
    {
    case opcode::Req:
        m.opCode = opcode::DRS;
        // m.time.tick_created = curr_tick;
#ifdef EVENTLOG
        log.CXLEventLog("Created message from ramulator response " + m.sprint() + "\n", this->node_id);
#endif
        S2M_DRS[cur_DRS_vc].enqueue(m);
        cur_DRS_vc = cur_DRS_vc == NUM_VC - 1 ? 0 : cur_DRS_vc + 1;
        break;
    case opcode::RwD:
        m.opCode = opcode::NDR;
        // m.time.tick_created = curr_tick;
#ifdef EVENTLOG
        log.CXLEventLog("Created message from ramulator response " + m.sprint() + "\n", this->node_id);
#endif
        S2M_NDR[cur_NDR_vc].enqueue(m);
        cur_NDR_vc = cur_NDR_vc == NUM_VC - 1 ? 0 : cur_NDR_vc + 1;
        break;
    default:
        CXL_ASSERT(false && "Unknown input message");
    }

    // Based on req_id as key, delete from messages in ramulator
    messages_in_ramulator.erase(r.req_id);
}

ramulator::RamDevice *CXLDevice::access_dram()
{
    return &dram;
}

/*!< Function unpacks flits and populates the rx side VCs and updates credit counters*/
bool CXLDevice::FLIT_unpacker()
{
    // rx buffer, Req queue and RwD queue should not be full at same time
    CXL_ASSERT(!(rx_buffer.is_buf_full() && M2S_Req.is_buf_full() && M2S_RWD.is_buf_full()) && "Rx side stall!");

    // Check if rx buffer is not empty
    if (rx_buffer.is_buf_empty())
        return false;

    // Do a latency check on the head of the rx buffer
    flit f = rx_buffer.get_head();
    if (curr_tick - f.time.time_of_receipt < params.delay_rx_buf_to_unpack)
        return false;

    flit copy_f;
    copy_f.copy(f);

// Device started to unpack flit
#ifdef EVENTLOG
    log.CXLEventLog("Device unpacking flit " + f.sprint() + "\n", this->node_id);
#endif
    // Counters for keeping count of different message types that were received
    int m2s_req_ctr = 0;
    int m2s_rwd_ctr = 0;
    int m2s_data_ctr = 0;

    // Packing rule checks
    // For 68B flit, data rollover cannot be more than 4 TODO: take care of packing rules for other flit types
    CXL_ASSERT((unpacker_rollover <= 4 && unpacker_rollover >= 0) && "Rollover data out of range");
    bool flag = true; // False means violation, will be used in an assert
    switch (unpacker_rollover)
    {
    case 0: // No check
        break;
    case 1: // Slot 1 must be data chunk
        flag = f.slots[1].type == data ? flag : false;
        break;
    case 2: // Slot 1 and 2 must be data chunks
        flag = f.slots[1].type == data && f.slots[2].type == data ? flag : false;
        break;
    case 3: // Slot 1,2 and 3 must be data chunks
        flag = f.slots[1].type == data && f.slots[2].type == data && f.slots[3].type == data ? flag : false;
        break;
    case 4: // All slots should be data chunks
        flag = f.slots[0].type == data && f.slots[1].type == data && f.slots[2].type == data && f.slots[3].type == data ? flag : false;
        break;
    default:
        CXL_ASSERT(false && "Data rollover out of range");
    }
    CXL_ASSERT(flag && "Data rollover rule violated");

    // TODO Add more packing rules

    // If there is a rollover, deal with that first since there can be a rwd header and data belonging to a diff rwd header in the same flit
    if (unpacker_rollover != 0)
    {
        // Start looking at the slots and deduct rollover
        for (int i = 0; i < 4; i++)
        {
            // It it is a data slot, then it belongs to previous rwd header. Decrement rollover counter and set the slot type to empty so that we dont process it again
            if (f.slots[i].type != slot_type::data)
                continue;
            unpacker_rollover--;
            f.slots[i].type = slot_type::empty;
            // If rollover reaches 0, enqueue the associated rwd header (the last rwd hdr) and quit this loop
            if (unpacker_rollover == 0)
            {
#ifdef EVENTLOG
                log.CXLEventLog("Unpacked message on device " + last_rwd_hdr.sprint() + "\n", this->node_id);
#endif
                // Set unpacking time for message
                last_rwd_hdr.time.tick_unpacked = curr_tick;
                M2S_RWD.enqueue(last_rwd_hdr); // Assume that m2s_rwd_hdr is accompanied by its data in the M2S_RWD queue
                break;
            }
        }
    }

    // Process the slots TODO: adapt to support multiple messages in a single slot
    // Remember that the rollover data chunks have been dealt with already
    for (slot s : f.slots)
    {
        switch (s.type)
        {
        case m2s_req:
#ifdef EVENTLOG
            log.CXLEventLog("Unpacked message on device " + s.msg.sprint() + "\n", this->node_id);
#endif
            // Set the tick unpacked
            s.msg.time.tick_unpacked = curr_tick;
            M2S_Req.enqueue(s.msg);
            m2s_req_ctr++;
            // std::cout << "Unpacked Req with id " << s.msg.msg_id << "\n";
            break;
        case m2s_rwd_hdr:
            CXL_ASSERT(unpacker_rollover == 0 && "Received RwD header without getting previous data first!");
            unpacker_rollover = 4; // TODO: May need to paramterize the 4 later
            m2s_rwd_ctr++;
            last_rwd_hdr.copy(s.msg);
            break;
        case data:
            CXL_ASSERT(unpacker_rollover > 0 && "Data received when not expected!");
            unpacker_rollover--;
            if (unpacker_rollover == 0)
            {
#ifdef EVENTLOG
                log.CXLEventLog("Unpacked message on device " + last_rwd_hdr.sprint() + "\n", this->node_id);
#endif
                // Set the tick unpacked
                last_rwd_hdr.time.tick_unpacked = curr_tick;
                M2S_RWD.enqueue(last_rwd_hdr); // Assume that m2s_rwd_hdr is accompanied by its data in the M2S_RWD queue
                // std::cout << "Unpacked RWD with id " << last_rwd_hdr.msg_id << "\n";
            }
            m2s_data_ctr++;
            break;
        case empty: // Do nothing
            break;
        default:
            CXL_ASSERT(false && "Illegal message type on rx buffer");
        }
    }

    // Packing rule check
    // Check max number of messages of a type in the flit
    flag = true; // False means rule violated
    // Max number of m2s_req is 2
    flag = m2s_req_ctr <= 2 ? flag : false;
    // Max number of m2s_rdw_hdr is 1
    flag = m2s_rwd_ctr <= 1 ? flag : false;
    // Max number of m2s_data is 4
    flag = m2s_data_ctr <= 4 ? flag : false;
    CXL_ASSERT(flag && "Packing rule violated");

    // Dequeue the flit
    rx_buffer.dequeue();

    // Update credit counters
    // int source_id = f.slots[0].msg.sp_id;
    ext_creds[0].rsp_credit += f.header.credit.rsp_credit;
    ext_creds[0].data_credit += f.header.credit.data_credit;
#ifdef EVENTLOG
    log.CXLEventLog("After Unpacking Flit " + copy_f.sprint() + " [" + print_cred(int_cred) + "] " + "[" + print_cred(ext_creds[0]) + "]\n", this->node_id);
#endif
    // credits_counter[0].req_credit = f.header.req_credit;
    // credits_counter[0].rsp_credit = f.header.rsp_credit;
    return true;
}

/*!< Convert from CXL message to ramulator reqs*/
// CXL_IF::ramulator_req message2ramulator_req(message m)
// {
//     std::vector<CXL_IF::ramulator_req> reqs;
//     // Make sure only reads or writes are there
//     CXL_ASSERT(m.opCode == opcode::Req || m.opCode == opcode::RwD, "Illegal message type");
//     // Making the assumption that ramulator operate at 64B granularity, same as CXL

//     CXL_IF::ramulator_req temp;
//     temp.req_addr = m.address; // TODO check if we need to align this to 64B boundary
//     temp.req_id = m.msg_id;
//     temp.req_type = m.opCode == opcode::Req ? ramulator::Request::Type::READ : ramulator::Request::Type::WRITE;
//     temp.type = CXL_IF::status_type::VALID;
//     return temp;
// }

/*!< Function checks if particular queue is not empty and pushes message onto ramulator queue*/
bool CXLDevice::check_rx_vc(CXLBuf<message> &vc)
{
    if (vc.is_buf_empty())
        return false;
    // Do a latency check on the head
    if (curr_tick - vc.get_head().time.tick_unpacked < params.delay_vc_to_ramulator)
        return false;
    // If the device to ramulator buffer is full, dont add anything
    if (access_dram()->inp_buf.isFull())
    {
        // log.CXLEventLog("Ramulator buffer full\n", this->node_id);
        return false;
    }
    // Convert from message to ramulator requests
    CXL_IF::ramulator_req req = message2ramulator_req(vc.get_head());
    // Add requests to ramulator input buffer
    access_dram()->inp_buf.buf_add(req);
    // Record the message sent to ramulator in a map with message id as the key
    messages_in_ramulator.insert({vc.get_head().msg_id, vc.get_head()});
// std::cout << "Sent req " << vc.get_head().msg_id << " to ramulator @" << curr_tick << " | " << access_dram()->state.clks << "\n";
#ifdef EVENTLOG
    log.CXLEventLog("Sent req to ramulator buffer" + vc.get_head().sprint() + " [" + print_cred(int_cred) + "] " + "[" + print_cred(ext_creds[0]) + "]" + "\n", this->node_id);
#endif
    // Free internal credits that were allotted to the host
    switch (vc.get_head().opCode)
    {
    case opcode::Req:
        int_cred.req_credit++;
        break;
    case opcode::RwD:
        // Do not free data credits when message is sent to ramulator. Only free these credits when the message is packed by the device
        // int_cred.data_credit++;
        break;
    default:
        CXL_ASSERT(false && "Illegal message type");
    }
    // Dequeue from VC
    vc.dequeue();
    return true;
}

/*!< Send requests from the Req and RwD queues to ramulator input buffer*/
bool CXLDevice::send_to_ramulator()
{
    bool active_flag = true; /*!< This flag is true if this cycle is active. If it is false, we can skip this cycle*/
    // Both VCs and the ramulator input buffer should not be full at the same time
    CXL_ASSERT(!(M2S_Req.is_buf_full() && M2S_RWD.is_buf_full() && access_dram()->inp_buf.isEmpty()) && "Ramulator side stall");

    // 3 state round robin {R,r,w}
    switch (ramulator_inp_state)
    {
    case round_robin_state::R:
        // First check RwD and then Req
        if (check_rx_vc(M2S_RWD)) // This functions adds reqs to ramulator inp buffer
            ramulator_inp_state = round_robin_state::w;
        else if (check_rx_vc(M2S_Req))
            ramulator_inp_state = round_robin_state::r;
        else
            active_flag = false;
        break;
    case round_robin_state::r:
        // First check Req and then RwD
        if (check_rx_vc(M2S_Req)) // This functions adds reqs to ramulator inp buffer
            ramulator_inp_state = round_robin_state::R;
        else if (check_rx_vc(M2S_RWD))
            ramulator_inp_state = round_robin_state::w;
        else
            active_flag = false;
        break;
    case round_robin_state::w:
        // First check Req and then RwD
        if (check_rx_vc(M2S_Req)) // This functions adds reqs to ramulator inp buffer
            ramulator_inp_state = round_robin_state::R;
        else if (check_rx_vc(M2S_RWD))
            ramulator_inp_state = round_robin_state::w;
        else
            active_flag = false;
        break;
    default:
        CXL_ASSERT(false && "Unknown state in round robin");
    }

    return active_flag;
}

void CXLDevice::device_init()
{
    // // Check if rx and tx buses are connected
    // CXL_ASSERT(check_connection(), "Incorrect bus connections");
    // dram = ramulator::RamDevice();
    // dram = ramulator::RamDevice();
    // Set parent device

    dram.set_parent(this);
    // cout << "This is done with addr " << this << "\n";

    // Run ramulator init
    dram.ramulator_init();
}

bool CXLDevice::check_connection()
{
    return CXLNode::check_connection();
}

//! Check tx buffer and put flits on the bus
bool CXLDevice::transmit()
{
    if (tx_buffer.is_buf_empty())
        return false;
    if (tx_bus->is_full())
        return false;
    if (curr_tick - tx_buffer.get_head().time.time_of_creation < params.delay_tx_buf_to_bus)
        return false;
    // Tx buffer has to maintain same output bandwidth as the bus
    if (curr_tick - last_transmitted_at < params.cxl_bus_ticks_per_dequeue)
        return false;
    // Set the last transmitted time to current tick
    last_transmitted_at = curr_tick;
    flit f;
    f.copy(tx_buffer.get_head());
    f.time.time_of_transmission = curr_tick;
    // Set time
    f.set_time(&msg_timing::tick_retransmitted, curr_tick);
// Add to the bus
#ifdef EVENTLOG
    log.CXLEventLog("Device to bus " + f.sprint() + "\n", this->node_id);
#endif
    tx_bus->add_to_bus(f);
    // Dequeue from tx buffer
    tx_buffer.dequeue();
    return true; // True means successfully dequeued
}

bool CXLDevice::check_vcs_for_packing(std::array<CXLBuf<message>, NUM_VC> &vcs, CXLBuf<message> *&vc)
{
    for (int i = packer_cur_vc; i < NUM_VC + packer_cur_vc; i++)
    {
        if (!vcs[i % NUM_VC].is_buf_empty() && curr_tick - vcs[i % NUM_VC].get_head().time.tick_ramulator_complete >= params.delay_vc_to_pack)
        {
            switch (vcs[i % NUM_VC].get_head().opCode)
            {
            case opcode::NDR:
                if (ext_creds[0].rsp_credit != 0)
                {
                    vc = &vcs[i % NUM_VC]; // assign
                    packer_cur_vc = packer_cur_vc == NUM_VC - 1 ? 0 : i % NUM_VC + 1;
                    return true;
                }
                break;
            case opcode::DRS:
                if (ext_creds[0].data_credit != 0 && packer_seq_length < seq_threshold)
                {
                    vc = &vcs[i % NUM_VC]; // assign vc
                    packer_cur_vc = packer_cur_vc == NUM_VC - 1 ? 0 : i % NUM_VC + 1;
                    return true;
                }
                break;
            default:
                CXL_ASSERT(false && "Illegal message type");
            }
        }
    }
    return false;
}

void CXLDevice::get_msg_from_vcs(CXLBuf<message> *&vc, bool &valid)
{
    switch (pkr2_state)
    {
    case round_robin_state::r:
        if (check_vcs_for_packing(S2M_NDR, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::R;
        }
        else if (check_vcs_for_packing(S2M_DRS, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::w;
        }
        else
            valid = false;
        break;
    case round_robin_state::R:
        if (check_vcs_for_packing(S2M_DRS, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::w;
        }
        else if (check_vcs_for_packing(S2M_NDR, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::R;
        }
        else
            valid = false;
        break;
    case round_robin_state::w:
        if (check_vcs_for_packing(S2M_NDR, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::r;
        }
        else if (check_vcs_for_packing(S2M_DRS, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::w;
        }
        else
            valid = false;
        break;
    default:
        CXL_ASSERT(false && "Illegal state in round robin");
    }
}

//! Just a function to reduce repeated lines of code in sending a packed flit to tx buffer
void CXLDevice::add_flit_to_buf(flit &f)
{
    // Set time of creation
    f.time.time_of_creation = curr_tick;
    // Set the credits
    f.header.credit = get_device_credits();
    // Set time of packing
    f.set_time(&msg_timing::tick_repacked, curr_tick);
    // Assign flit ID and increment flit counter
    f.flit_id = CXL::flit::flit_counter;
    CXL::flit::flit_counter++;
    //  Send to tx buf
    tx_buffer.enqueue(f);
    // If this flit can cause rollover, increment sequence length otherwise reset the sequence length
    if (packer_rollover > 0)
        packer_seq_length++;
    else
        packer_seq_length = 0;
    // If we are at the sequence threshold, raise alert if rollover is non zero
    CXL_ASSERT(!(packer_seq_length == seq_threshold && packer_rollover > 0) && "Device pakcer sequence threshold violated");
// Log it
#ifdef EVENTLOG
    log.CXLEventLog("Device packed flit " + f.sprint() + " [" + print_cred(int_cred) + "] " + "[" + print_cred(ext_creds[0]) + "]" + "\n", this->node_id);
#endif
}

//! Do a sanity check on the host's internal credit counters
void CXLDevice::credit_sanity_check()
{
    CXL_ASSERT(int_cred.rsp_credit == 0 && "Rsp cred of device should be 0");
    CXL_ASSERT((int_cred.req_credit >= 0 && int_cred.req_credit <= M2S_Req.size()) && "Req cred of host out of range");
    CXL_ASSERT((int_cred.data_credit >= 0 && int_cred.data_credit <= M2S_RWD.size()) && "Data cred of host out of range");
}

//! Once flit is packed, we need to put the credits device is willing to allot into the header. Also need to decrement our internal credit counter
credits CXLDevice::get_device_credits()
{
    int req_credit = 0, data_credit = 0; /*!< These are the ones which will be sent out this cycle*/
    // Sanity checks
    credit_sanity_check();
    // Rsp credits
    if (int_cred.req_credit == 1)
        req_credit = 1;
    if (int_cred.req_credit > 1)
        req_credit = int_cred.req_credit / connected_hosts.size();
    // data credits
    if (int_cred.data_credit == 1)
        data_credit = 1;
    if (int_cred.data_credit > 1)
        data_credit = int_cred.data_credit / connected_hosts.size();
    // Decrement internal credits
    int_cred.req_credit -= req_credit;
    int_cred.data_credit -= data_credit;
    credits crds = {req_credit, 0, data_credit};
    return crds;
}

//! Function to check if we can skip calling the packer for this cycle. Returns true if we can skip the cycle
bool CXLDevice::skip_packer_check()
{
    // Packer can be skipped based on following conditions
    // If the tx VCs are both empty and
    // If there is no rollover
    // If there are messages in tc VCs then we need to make sure that the first #read port number of messages do not satisfy latency constraints
    if (packer_rollover != 0 || is_packer_waiting)
        return false;
    // If all tx virtual channels are empty we can skip cycle
    bool flag = true;
    for (int i = 0; i < NUM_VC; i++)
    {
        if (!S2M_DRS[i].is_buf_empty() || !S2M_NDR[i].is_buf_empty())
            flag = false;
    }
    if (flag)
        return true;
    // Now check the head of each tx VC
    // If there are any latency satisfying messages then this cycle cannot be guaranteed to be inactive so return false
    for (int i = 0; i < NUM_VC; i++)
    {
        // Check DRS channel
        if (!S2M_DRS[i].is_buf_empty() && curr_tick - S2M_DRS[i].get_head().time.tick_created >= params.delay_vc_to_pack)
            return false;
        // Check RWD channel
        if (!S2M_NDR[i].is_buf_empty() && curr_tick - S2M_NDR[i].get_head().time.tick_created >= params.delay_vc_to_pack)
            return false;
    }
    // We have checked all conditions now, can return true at this point
    return true;
}

//! Function to check if we can skip calling the unpacker for this cycle. Returns true if we can skip the cycle
bool CXLDevice::skip_unpacker_check()
{
    // To skip cycle for unpacker need to check the following
    // 1. Check if rx buffer is empty? If it is empty then it is definitely a skippable cycle
    // 2. In case rx buffer is not empty, check the head element and see if the head element satisfies latency requirements. If it doesn't then it is a skippable cycle
    return (rx_buffer.is_buf_empty() ||
            (!rx_buffer.is_buf_empty() && (curr_tick - rx_buffer.get_head().time.time_of_receipt < params.delay_rx_buf_to_unpack)));
}

//! Function to check if we can skip calling the send to ramulator for this cycle. Returns true if we can skip the cycle
bool CXLDevice::skip_send_to_ram_check()
{
    // We can skip the cycle if
    // 1. Both rx VCs are empty
    // 2. The CXL interface inp_buf is full
    // 3. The VCs are not empty, but the message at the head does not satisfy latency constraint
    if (access_dram()->inp_buf.isFull())
        return false;
    if (M2S_Req.is_buf_empty() && M2S_RWD.is_buf_empty())
        return true;
    if (!M2S_Req.is_buf_empty() && curr_tick - M2S_Req.get_head().time.tick_unpacked >= params.delay_vc_to_ramulator)
        return false;
    if (!M2S_RWD.is_buf_empty() && curr_tick - M2S_RWD.get_head().time.tick_unpacked >= params.delay_vc_to_ramulator)
        return false;
    return true;
}

//! Function to check if we can skip calling the transmit function for this cycle. Returns true if we can skip the cycle
bool CXLDevice::skip_transmit_check()
{
    // Transmit can be skipped if
    // 1. tx buffer is empty
    // 2. If tx bus is full
    // 3. If tx buffer is not empty then check if head of the buffer has satisfied latency constraints or buffer is not exceeding bus bandwidth
    return ((tx_buffer.is_buf_empty() || tx_bus->is_full()) ||
            (!tx_buffer.is_buf_empty() &&
             (curr_tick - tx_buffer.get_head().time.time_of_creation < params.delay_tx_buf_to_bus ||
              curr_tick - last_transmitted_at < params.cxl_bus_ticks_per_dequeue)));
}

//! Packs messages from VCs into flits
bool CXLDevice::FLIT_packer()
{
    bool active_flag = true; /*!< This flag indicates if during this cycle the packer is active or not*/
    // If we dont have a half packed flit, create a new filt
    if (!is_packer_waiting)
    {
        flit_to_pack = flit();
        started_packing_at = curr_tick;
    }
    // If we have a fully formed flit ready, just send it
    if (is_packer_waiting && flit_to_pack.is_flit_full())
    {
        // Check if tx buffer is full, if so apply backpressure and stop packing
        if (tx_buffer.is_buf_full())
        {
            // log.CXLEventLog("Host tx buffer full\n", this->node_id);
            return true;
        }
        // Send flit to tx buffer
        add_flit_to_buf(flit_to_pack);
        // Set packer waiting to false
        is_packer_waiting = false;
        return true;
    }
    // If we have exceeded the packing wait time while having a partially packed flit, send the flit as is, by leaving slots empty if necessary (not all the slots)
    // Remember to not simply send out an empty flit unnecessarily
    else if (is_packer_waiting && curr_tick - started_packing_at > packer_wait_time && !flit_to_pack.is_flit_empty())
    {
        // Check if tx buffer is full, if so apply backpressure and stop packing
        if (tx_buffer.is_buf_full())
        {
            // log.CXLEventLog("Host tx buffer full\n", this->node_id);
            return true;
        }
        // Send flit to tx buffer
        add_flit_to_buf(flit_to_pack);
        // Set packer waiting to false
        is_packer_waiting = false;
        return true;
    }

    CXL_ASSERT((packer_rollover >= 0 && packer_rollover <= 4) && "Packer rollover illegal value");

    // If there is rollover from previous cycle and this is a brancd new flit, do that first
    if (packer_rollover != 0 && !is_packer_waiting)
    {
        switch (packer_rollover)
        {
        case 1:
            flit_to_pack.header.slots[1] = slot_type::data;
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_drs_hdr.address;
            packer_rollover--;
            break;
        case 2:
            flit_to_pack.header.slots[1] = slot_type::data;
            flit_to_pack.header.slots[2] = slot_type::data;
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[2].msg.address = last_drs_hdr.address;
            packer_rollover -= 2;
            break;
        case 3:
            flit_to_pack.header.slots[1] = slot_type::data;
            flit_to_pack.header.slots[2] = slot_type::data;
            flit_to_pack.header.slots[3] = slot_type::data;
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[3].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[3].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[2].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[3].msg.address = last_drs_hdr.address;
            packer_rollover -= 3;
            break;
        case 4:
            flit_to_pack.header.slots[0] = slot_type::data;
            flit_to_pack.header.slots[1] = slot_type::data;
            flit_to_pack.header.slots[2] = slot_type::data;
            flit_to_pack.header.slots[3] = slot_type::data;
            flit_to_pack.slots[0].type = slot_type::data;
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[3].type = slot_type::data;
            flit_to_pack.slots[0].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[1].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[3].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[0].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[1].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[2].msg.address = last_drs_hdr.address;
            flit_to_pack.slots[3].msg.address = last_drs_hdr.address;
            packer_rollover -= 4;
            // Check if tx buffer is full, if so apply backpressure and stop packing
            if (tx_buffer.is_buf_full())
            {
                // log.CXLEventLog("Host tx buffer full\n", this->node_id);
                // Set is packer waiting so that we dont overwrite the all data flit in the next cycle
                is_packer_waiting = true;
                return true;
            }
            // If its an all data flit, might as well add it to tx buffer and clear state
            // Send flit to tx buffer
            add_flit_to_buf(flit_to_pack);
            is_packer_waiting = false;
            return true;
            break;
        default:
            CXL_ASSERT(false && "Illegal rollover count");
        }
    }

    // temporary variables
    CXLBuf<message> *vc;
    bool valid = false;
    bool no_more_messages_in_VC = false;
    for (int i = 0; i < 4; i++)
    {
        // Don't try to pack a slot if its not empty
        if (flit_to_pack.slots[i].type != slot_type::empty)
            continue;
        // If we have rollover and the slot under consideration is not the header (slot 0)
        if (packer_rollover && i != 0)
        {
            flit_to_pack.header.slots[i] = slot_type::data;
            flit_to_pack.slots[i].type = slot_type::data;
            flit_to_pack.slots[i].msg.msg_id = last_drs_hdr.msg_id;
            flit_to_pack.slots[i].msg.address = last_drs_hdr.address;
            packer_rollover--;
            continue;
        }
        // Try to get a message from the VCs
        get_msg_from_vcs(vc, valid);
        if (!valid)
            break;
        if (valid)
        {
            switch (vc->get_head().opCode)
            {
            case opcode::NDR:
                // Can't have more than two NDR in a single flit
                if (flit_to_pack.slot_count_in_flit(slot_type::s2m_ndr) <= 1)
                {
                    flit_to_pack.header.slots[i] = slot_type::s2m_ndr;
                    flit_to_pack.slots[i].type = slot_type::s2m_ndr;
                    flit_to_pack.slots[i].msg.copy(vc->get_head());
                    // Free data credit when you pack an NDR. This means that we can accept one more write
                    int_cred.data_credit++;
                    // Decrement rsp credits
                    ext_creds[0].rsp_credit--;
                    // Erase from VC
                    vc->dequeue();
                    continue;
                }
                break;
            case opcode::DRS:
                // We can add an rwd header to a flit's slot 0 as long as there is no other rwd header in the same flit
                if ((flit_to_pack.slot_count_in_flit(slot_type::s2m_drs_hdr) == 0 && i == 0) ||
                    (flit_to_pack.slot_count_in_flit(slot_type::s2m_drs_hdr) == 0 && flit_to_pack.are_all_slots_after_this_empty(i)))
                {
                    flit_to_pack.header.slots[i] = slot_type::s2m_drs_hdr;
                    flit_to_pack.slots[i].type = slot_type::s2m_drs_hdr;
                    last_drs_hdr.copy(vc->get_head());
                    flit_to_pack.slots[i].msg.copy(vc->get_head());
                    packer_rollover = 4;
                    // Decrement data credits
                    ext_creds[0].data_credit--;
                    vc->dequeue();
                    continue;
                }
                break;
            default:
                CXL_ASSERT(false && "Illegal message type");
            }
        }
    }

    // Packer is waiting only if the current flit is partially packed
    if (!flit_to_pack.is_flit_empty())
        is_packer_waiting = true;
    else
        // If the currrent flit is empty that means the packer did nothing in this cycle and this is therefore not an active cycle
        active_flag = false;

    // Temp vars
    int drs_count = flit_to_pack.slot_count_in_flit(slot_type::s2m_drs_hdr);
    int ndr_count = flit_to_pack.slot_count_in_flit(slot_type::s2m_ndr);
    int data_count = flit_to_pack.slot_count_in_flit(slot_type::data);

    // Packing rule check
    bool flag = false; // True means some error has occured;
    if (ndr_count > 2 || ndr_count < 0)
        flag = true;
    if (data_count > 4 || data_count < 0)
        flag = true;
    if (drs_count > 1 || drs_count < 0)
        flag = true;
    if (drs_count + data_count + ndr_count > 4)
        flag = true;
    CXL_ASSERT(!flag && "Packing rule violated on packer side");
    return active_flag;
}

// //! Returns node id of the destination CXL Device by looking at the address
// uint CXLDevice::destination_device(uint64_t addr)
// {
//     uint64_t dest;
//     bool found = false;

//     for (auto &iter : this->address_intervals)
//     {
//         // check if this is correct, do we need to consider the offset, in this case, 8 bytes?
//         if (addr < iter.second.second && addr >= iter.second.first)
//         {
//             found = true;
//             dest = iter.first;
//             break;
//         }
//     }
//     CXL_ASSERT(found == true, "Invalid address");
//     return dest;
// }

//! Print all the skipped cycles for each of the component functions within device update
void CXLDevice::print_skipped_cycles()
{
    std::cout << "=====================================================\n";
    std::cout << "Num Reqs:" << num_reqs << "\n";
    std::cout << "Summary of skipped cycles in cxl device\n";
    std::cout << "Packer " << empty_cycle_packer << "\n";
    std::cout << "Unpacker " << empty_cycle_unpacker << "\n";
    std::cout << "Transmit " << empty_cycle_transmit << "\n";
    std::cout << "Send to ramulator " << empty_cycle_sent_to_ram << "\n";
    std::cout << "Skippable cycle " << skippable_cycle << "\n";
    std::cout << "=====================================================\n";
}