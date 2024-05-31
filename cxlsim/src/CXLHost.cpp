#include "CXLNode.h"
#include "message.h"
#include "flit.h"
#include <vector>
#include <cstdint>
#include "utils.h"

using namespace CXL;

namespace CXL
{
    extern int64_t curr_tick;
    extern CXLParams params;
    extern CXLLog log;
    extern uint64_t num_reqs_completed;
    #ifdef TRACK_LATENCY
        extern std::string latency_file_prefix;
    #endif
}

CXLHost::CXLHost() : CXLNode(), text_to_trace_buf(20)
{
    this->node_id = 0;
    packer_rollover = 0;
    unpacker_rollover = 0;
    packer_stall = false;
    packer_state = round_robin_state::R;
    Req_packed = 0;
    last_transmitted_at = 0;
    trace_line_pending = 0;
    rcvd_rsp_state = round_robin_state::R;
    is_trace_finished = false;
    started_packing_at = 0;
    packer_wait_time = (int64_t)(0.1 * (float)params.ticks_per_ns); // 0.1ns
    pkr2_state = round_robin_state::R;
    req_ctr = 0;
    rwd_ctr = 0;
    is_packer_waiting = false;
    // Initialize your own credits counter to the max size -1 of your available NDR and DRS VCs
    int_cred = {0, S2M_NDR.size() - S2M_NDR.buf_occupancy(), S2M_DRS.size() - S2M_DRS.buf_occupancy()};
    log.CXLEventLog("Internal Credit initialization [" + print_cred(int_cred) + "]\n", this->node_id);
}

CXLHost::CXLHost(int vc_size, int buf_size, uint node_id) : CXLNode(vc_size, buf_size), text_to_trace_buf(20), S2M_DRS(1024), S2M_NDR(1024)
{
    for (int i = 0; i < NUM_VC; i++)
    {
        M2S_Req[i] = CXLBuf<message>(vc_size / NUM_VC);
        M2S_RWD[i] = CXLBuf<message>(vc_size / NUM_VC);
    }
    DAM = nullptr;
    cur_Req_vc = 0;
    cur_RWD_vc = 0;
    this->node_id = node_id;
    packer_rollover = 0;
    unpacker_rollover = 0;
    packer_stall = false;
    packer_state = round_robin_state::R;
    Req_packed = 0;
    last_transmitted_at = 0;
    trace_line_pending = 0;
    rcvd_rsp_state = round_robin_state::R;
    is_trace_finished = false;
    started_packing_at = 0;
    packer_wait_time = 10; // 0.1ns
    pkr2_state = round_robin_state::R;
    req_ctr = 0;
    rwd_ctr = 0;
    is_packer_waiting = false;
    // Initialize your own credits counter to the max size -1 of your available NDR and DRS VCs
    int_cred = {0, S2M_NDR.size() - S2M_NDR.buf_occupancy(), S2M_DRS.size() - S2M_DRS.buf_occupancy()};
#ifdef EVENTLOG
    log.CXLEventLog("Internal Credit initialization [" + print_cred(int_cred) + "]\n", this->node_id);
#endif
    empty_cycle_packer = 0;
    empty_cycle_unpacker = 0;
    empty_cycle_req_completed = 0;
    empty_cycle_transmit = 0;
    skippable_cycle = 0;
#ifdef TRACK_LATENCY
    // Open latency.csv and latency_dam.csv in write mode so that we overwrite previous file
    ofstream temp(latency_file_prefix+"_cxl.csv");
    temp.close();
    ofstream temp2(latency_file_prefix+"_dam.csv");
    temp2.close();
#endif
    last_text_to_trace_buf_dequeue = 0;
    int_credit_reject_ctr = 0;
    ext_credit_reject_ctr = 0;
    total_credit_checks = 0;
    printf("Host, %d,%d,%d,%d\n", M2S_Req[0].size() * M2S_Req.size(), tx_buffer.size(), S2M_DRS.size(), rx_buffer.size());
}

void CXLHost::update()
{
    // Get trace
    if (!is_trace_finished || !text_to_trace_buf.is_buf_empty())
        if (!text_to_trace()) // Get trace line by line. If all entries in trace file are done, set is_trace_finished to true
            is_trace_finished = true;
    // Check that no extra external credits are added
    CXL_ASSERT(ext_creds.size() == connected_devices.size() && "External credits for unknown device added");

    bool packer_flag = false,
         transmit_flag = false,
         unpacker_flag = false,
         complete_flag = false;

    // Pack messages
    bool packer_flag2 = !skip_packer_check();
#ifdef SKIP_CYCLE
    if (packer_flag2)
        packer_flag = FLIT_packer();
    empty_cycle_packer = packer_flag2 ? empty_cycle_packer : empty_cycle_packer + 1;
#else
    packer_flag = FLIT_packer();
#endif
    // Send to the bus

    bool transmit_flag2 = !skip_transmit_check();
#ifdef SKIP_CYCLE
    if (transmit_flag2)
        transmit_flag = transmit();
    empty_cycle_transmit = transmit_flag2 ? empty_cycle_transmit : empty_cycle_transmit + 1;
#else
    transmit_flag = transmit();
#endif
    // Unpack received messages

    bool unpacker_flag2 = !skip_unpacker_check();
#ifdef SKIP_CYCLE
    if (unpacker_flag2)
        unpacker_flag = FLIT_unpacker();
    empty_cycle_unpacker = unpacker_flag2 ? empty_cycle_unpacker : empty_cycle_unpacker + 1;
#else
    unpacker_flag = FLIT_unpacker();
#endif
    // Check received messaged

    bool complete_flag2 = !skip_req_complete_check();
#ifdef SKIP_CYCLE
    if (complete_flag2)
        complete_flag = check_if_req_completed();
    empty_cycle_req_completed = complete_flag2 ? empty_cycle_req_completed : empty_cycle_req_completed + 1;
#else
    complete_flag = check_if_req_completed();
#endif

    if (!packer_flag2 && !transmit_flag2 && !unpacker_flag2 && !complete_flag2)
        skippable_cycle++;

#ifdef SKIP_CYCLE
    // It should not be such that the skip check function says cycle is skippable (i.e, false) and the actual component function itself returns true
    // Flasely assuming a cycle to be skippable is not acceptable
    CXL_ASSERT(!((!packer_flag2 && packer_flag)) && "Packer Flag Error");
    CXL_ASSERT(!((!transmit_flag2 && transmit_flag)) && "Transmit Flag Error");
    CXL_ASSERT(!((!unpacker_flag2 && unpacker_flag)) && "Unpacker Flag Error");
    CXL_ASSERT(!((!complete_flag2 && complete_flag)) && "Complete Flag Error");
#endif
}

bool CXLHost::no_active_transaction()
{
    //There is no active transaction in the system if
    //1. Host Tx buffers are empty
    //2. Host Rx buffers are empty
    //3. The messages_sent_to_device map is empty i.e, there are no CXL reqs that have been sent to device but yet to be fulfilled
    //4. The reqs_in_dam map is emmpty i.e., there are no DAM reqs sent to device but yet to be fulfilled

    //Return true if there is no active transaction

    bool tx_buf_empty = true;
    for(size_t i=0;i<NUM_VC;i++)
    {
        if(!M2S_Req[i].is_buf_empty() || !M2S_RWD[i].is_buf_empty())
        {
            tx_buf_empty = false;
            break;
        }
    }
    return tx_buf_empty && S2M_DRS.is_buf_empty() && S2M_NDR.is_buf_empty() && 
           reqs_in_dam.empty() && messages_sent_to_device.empty();
}

void CXLHost::register_device(uint64_t device_id, const std::pair<uint64_t, uint64_t> &addr_interval)
{
    // Check if the device is already registered
    for (uint64_t i : connected_devices)
    {
        CXL_ASSERT(i != device_id && "Re-registering device");
    }
    connected_devices.push_back(device_id);
    address_intervals.insert({device_id, addr_interval});
    // Initialize credit counters
    credits crd = {1, 0, 1}; // Downstream CXLDevice will only have req and data credits not rsp credits
    ext_creds.insert({device_id, crd});
}

void CXLHost::register_DAM(ramulator::DirectAttached *dam, const std::pair<uint64_t, uint64_t> &addr_interval)
{
    DAM = dam;
    DAM_addr = addr_interval;
}

void CXLHost::register_memory(ramulator::RamDevice* ram_ptr)
{
    device_memories.push_back(ram_ptr);
}

// bool CXLHost::check_send()
// {
//     uint64_t ld_id = tx_buffer.get_head().slots[0].msg.ld_id;
//     // check if the bus is full
//     if (0)
//     {
//         return false;
//     }
//     // check the counter to ensure latency of the transmitter
//     else if (0)
//     {
//         return false;
//     }
//     // check credit for non-control flits
//     else if (0)
//     {
//         return false;
//     }
//     else
//     {
//     }
//     return true;
// }

/*!< Function unpacks flits and populates the rx side VCs and updates credit counters*/
bool CXLHost::FLIT_unpacker()
{
    // rx buffer, Req queue and RwD queue should not be full at same time
    CXL_ASSERT(!(rx_buffer.is_buf_full() && S2M_DRS.is_buf_full() && S2M_NDR.is_buf_full()) && "Rx side stall!");

    // Check if rx buffer is not empty
    if (rx_buffer.is_buf_empty())
        return false;

    // TODO: Prevent flit copying
    // Do a latency check on the head of the rx buffer
    flit f = rx_buffer.get_head();
    if (curr_tick - f.time.time_of_receipt < params.delay_rx_buf_to_unpack)
        return false;

    // Keep track of which device sent this flit
    uint device_id = 4096; // Use  default of 4096 just cause

#ifdef EVENTLOG
    // Device started to unpack flit
    log.CXLEventLog("Host unpacking flit " + f.sprint() + "\n", this->node_id);
#endif

    // Make sure all non empty slots have same destination device
    for (slot s : f.slots)
    {
        if (s.type == slot_type::empty)
            continue;
        if (device_id == 4096)
        {
            device_id = destination_device(s.msg.address);
            continue;
        }
        if (device_id != destination_device(s.msg.address))
            CXL_ASSERT(false && "Single flit has messages from different devices");
        else
            device_id = destination_device(s.msg.address);
    }

    // Counters for keeping count of different message types that were received
    int s2m_ndr_ctr = f.slot_count_in_flit(slot_type::s2m_ndr);
    int s2m_drs_ctr = f.slot_count_in_flit(slot_type::s2m_drs_hdr);
    int s2m_data_ctr = f.slot_count_in_flit(slot_type::data);

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

    // Check max number of messages of a type in the flit
    flag = true;
    if (s2m_ndr_ctr > 2 || s2m_ndr_ctr < 0)
        flag = false;
    if (s2m_data_ctr > 4 || s2m_data_ctr < 0)
        flag = false;
    if (s2m_drs_ctr > 1 || s2m_drs_ctr < 0)
        flag = false;
    CXL_ASSERT(flag && "Packing rule violated");

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
                log.CXLEventLog("Unpacked message on host " + last_drs_hdr.sprint() + "\n", this->node_id);
#endif
                // Set time of unpacking
                last_drs_hdr.time.tick_resp_unpacked = curr_tick;
                S2M_DRS.enqueue(last_drs_hdr); // Assume that s2m_drs_hdr is accompanied by its data in the S2M_DRS queue
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
        case s2m_ndr:
#ifdef EVENTLOG
            log.CXLEventLog("Unpacked message on host " + s.msg.sprint() + "\n", this->node_id);
#endif
            // Set the tick unpacked
            s.msg.time.tick_resp_unpacked = curr_tick;
            S2M_NDR.enqueue(s.msg);
            // std::cout << "Unpacked Req with id " << s.msg.msg_id << "\n";
            break;
        case s2m_drs_hdr:
            CXL_ASSERT(unpacker_rollover == 0 && "Received RwD header without getting previous data first!");
            unpacker_rollover = 4; // TODO: May need to paramterize the 4 later
            last_drs_hdr.copy(s.msg);
            break;
        case data:
            CXL_ASSERT(unpacker_rollover > 0 && "Data received when not expected!");
            unpacker_rollover--;
            if (unpacker_rollover == 0)
            {
#ifdef EVENTLOG
                log.CXLEventLog("Unpacked message on host " + last_drs_hdr.sprint() + "\n", this->node_id);
#endif
                // Set the tick unpacked
                last_drs_hdr.time.tick_resp_unpacked = curr_tick;
                S2M_DRS.enqueue(last_drs_hdr); // Assume that s2m_drs_hdr is accompanied by its data in the S2M_DRS queue
                // std::cout << "Unpacked RWD with id " << last_drs_hdr.msg_id << "\n";
            }
            break;
        case empty: // Do nothing
            break;
        default:
            CXL_ASSERT(false && "Illegal message type on rx buffer");
        }
    }

    // Dequeue the flit
    rx_buffer.dequeue();

    // Update credit counters
    // int source_id = f.slots[0].msg.sp_id;
    ext_creds[device_id].req_credit += f.header.credit.req_credit;
    ext_creds[device_id].data_credit += f.header.credit.data_credit;
    // credits_counter[0].req_credit = f.header.req_credit;
    // credits_counter[0].rsp_credit = f.header.rsp_credit;

    return true;
}

void CXLHost::connect_rx(CXLBus *rx_bus)
{
    CXLNode::connect_rx(rx_bus);
}

void CXLHost::connect_tx(CXLBus *tx_bus)
{
    CXLNode::connect_tx(tx_bus);
}

//! Check tx buffer and put flits on the bus
bool CXLHost::transmit()
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
    // Set tick_transmitted for each message
    f.set_time(&msg_timing::tick_transmitted, curr_tick);
#ifdef EVENTLOG
    // Add to the bus
    log.CXLEventLog("Host to bus " + f.sprint() + "\n", this->node_id, this->tx_bus->node_id);
#endif
    tx_bus->add_to_bus(f);
    // Dequeue from tx buffer
    tx_buffer.dequeue();
    return true; // True means successfully dequeued
}

std::map<int, credits> *CXLHost::get_cred()
{
    return &ext_creds;
}

std::vector<std::string> split(const std::string &s, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim))
    {
        result.push_back(item);
    }

    return result;
}

//! Convert traces from the text file into messages and put them on the VCs
bool CXLHost::text_to_trace()
{

    // Check if the system is inactive, if so advance curr tick by the gap required to issue next request
    if (no_active_transaction() && !text_to_trace_buf.is_buf_empty())
    {
        int64_t curr_tick_before=curr_tick;
        curr_tick += (text_to_trace_buf.get_head().first * params.ticks_per_ins) - (curr_tick - last_text_to_trace_buf_dequeue);
        //While the host n device may be inactive, there might still be cycles in the ramulator itself. We need to continue these cycles. For this purpose, when we skip we update ramulator as many number of times as it would have without skipping
        for(int64_t i=((curr_tick_before/6)+1)*6;i<=curr_tick;i+=6)
        {
            if(i!=curr_tick)
                DAM->update();
            for(ramulator::RamDevice *ram_ptr: device_memories)
            {
                ram_ptr->update();
            }
        }
    }

    // Move messages from the text to trace buf to the VCs or DAM buffer using round robin if buffer is not empty and we have waited for enough time between message generations
    if (!text_to_trace_buf.is_buf_empty() && (curr_tick - last_text_to_trace_buf_dequeue >= text_to_trace_buf.get_head().first * params.ticks_per_ins))
    {
        last_text_to_trace_buf_dequeue = curr_tick;

        // Check if message belongs to DAM
        message t = text_to_trace_buf.get_head().second;
        if (DAM != nullptr && t.address >= DAM_addr.first && t.address < DAM_addr.second)
        {
            if (!DAM->inp_buf.isFull())
            {
                CXL_IF::ramulator_req req = message2ramulator_req(t);
                // Send to the DAM
                DAM->inp_buf.buf_add(req);
                reqs_in_dam.insert({req.req_id, curr_tick});
                // Dequeue buffer
                text_to_trace_buf.dequeue();
#ifdef EVENTLOG
                log.CXLEventLog("Created message from trace " + t.sprint() + "\n", node_id);
                log.CXLEventLog("Sent req to DAM " + t.sprint() + "\n", DAM->node_id);
#endif
            }
        }
        else
        {
            message temp;
            temp.copy(text_to_trace_buf.get_head().second);
            temp.time.tick_created = curr_tick;
            // temp.copy(text_to_trace_buf.get_head());
            switch (temp.opCode)
            {
            case opcode::Req:
                if (M2S_Req[cur_Req_vc].is_buf_full())
                    return true;
                temp.time.read_write = false;
                M2S_Req[cur_Req_vc].enqueue(temp);
                cur_Req_vc = cur_Req_vc == NUM_VC - 1 ? 0 : cur_Req_vc + 1;
#ifdef EVENTLOG
                log.CXLEventLog("Created message from trace " + temp.sprint() + "\n", this->node_id);
#endif
                break;
            case opcode::RwD:
                if (M2S_RWD[cur_RWD_vc].is_buf_full())
                    return true;
#ifdef EVENTLOG
                log.CXLEventLog("Created message from trace " + temp.sprint() + "\n", this->node_id);
#endif
                temp.time.read_write = true;
                M2S_RWD[cur_RWD_vc].enqueue(temp);
                cur_RWD_vc = cur_RWD_vc == NUM_VC - 1 ? 0 : cur_RWD_vc + 1;
                break;
            default:
                CXL_ASSERT(false && "Illegal message type");
            }
            text_to_trace_buf.dequeue();
            // Add this message to the std map keeping track of all messages sent to the devices
            messages_sent_to_device.insert({temp.msg_id, temp});
        }
    }
    // If the text_to_trace buf is full, skip creating new messages, but return true to show that file has not ended yet
    if (text_to_trace_buf.is_buf_full())
        return true;

    // If the buffer is not full we can put stuff into it now
    std::string s;
    // Create message from a line of text in the trace file
    if (!std::getline(trace_in, s))
        return false; // False means file has ended

    // Process the line
    // Split string at the spaces
    std::vector<std::string> words = split(s, ' ');
    CXL_ASSERT(words.size() == 3 && "Incorrect string splitting");
    // Make the first word the address and the second word the type
    uint64_t addr = (uint64_t)stoul(words[0], NULL, 0);
    opcode opCode;
    if (words[1].compare("R") == 0)
        opCode = opcode::Req;
    else if (words[1].compare("W") == 0)
        opCode = opcode::RwD;
    else
        CXL_ASSERT(false && "Illegal opcode");
    // The third word is the number of instructions executed by CPU before this access
    uint64_t clk_interval = (uint64_t)stoul(words[2], NULL, 0);

    // Create the message to be put on the buffer and then later onto the virtual channels
    message m = message(opCode, addr);
    text_to_trace_buf.enqueue(std::pair<uint64_t, message>(clk_interval, m));
    return true; // True means file has not ended
}

// bool CXLHost::text_to_trace()
// {
//     std::string s;
//     // If there is a pending message to send, send it first
//     if (trace_line_pending)
//     {
//         switch (message_to_send->opCode)
//         {
//         case opcode::Req:
//             if (M2S_Req.is_buf_full())
//                 return true;
//             M2S_Req.enqueue(*message_to_send);
//             log.CXLEventLog("Created message from trace " + message_to_send->sprint() + "\n", this->node_id);
//             break;
//         case opcode::RwD:
//             if (M2S_RWD.is_buf_full())
//                 return true;
//             log.CXLEventLog("Created message from trace " + message_to_send->sprint() + "\n", this->node_id);
//             M2S_RWD.enqueue(*message_to_send);
//             break;
//         default:
//             CXL_ASSERT(false, "Illegal message type");
//         }
//         trace_line_pending = false;
//         // Add this message to the std map keeping track of all messages sent to the devices
//         messages_sent_to_device.insert({message_to_send->msg_id, *message_to_send});
//         return true; // True means file has not ended
//     }

//     // Create message
//     if (!std::getline(trace_in, s))
//         return false; // False means file has ended

//     // Process the line
//     // Split string at the space
//     std::vector<std::string> words = split(s, ' ');
//     CXL_ASSERT(words.size() == 2, "Incoreect string splitting");
//     // Make the first word the address and the second word the type
//     uint64_t addr = (uint64_t)stoul(words[0], NULL, 0);
//     opcode opCode;
//     if (words[1].compare("R") == 0)
//         opCode = opcode::Req;
//     else if (words[1].compare("W") == 0)
//         opCode = opcode::RwD;
//     else
//         CXL_ASSERT(false, "Illegal opcode");

//     message_to_send = new message(opCode, addr);

//     std::string temp = message_to_send->sprint();

//     // Chech if correct buffer is full or not
//     switch (message_to_send->opCode)
//     {
//     case opcode::Req:
//         if (M2S_Req.is_buf_full())
//             trace_line_pending = true;
//         else
//         {
//             log.CXLEventLog("Created message from trace " + temp + "\n", this->node_id);
//             M2S_Req.enqueue(*message_to_send);
//         }
//         break;
//     case opcode::RwD:
//         if (M2S_RWD.is_buf_full())
//             trace_line_pending = true;
//         else
//         {
//             log.CXLEventLog("Created message from trace " + temp + "\n", this->node_id);
//             M2S_RWD.enqueue(*message_to_send);
//         }
//         break;
//     default:
//         CXL_ASSERT(false, "Illegal message type");
//     }
//     // Add this message to the std map keeping track of all messages sent to the devices
//     messages_sent_to_device.insert({message_to_send->msg_id, *message_to_send});
//     return true;
// }

void CXLHost::set_trace_file(std::string filename)
{
    // Load trace file
    trace_in.open(filename);
}

//! Function checks given VC to see if there is some request whose response is received
bool CXLHost::check_rx_vc(CXLBuf<message> &vc)
{
    // Check that the VC is not empty
    if (vc.is_buf_empty())
        return false;
    // Do a latency check before retiring
    if (curr_tick - vc.get_head().time.tick_resp_unpacked < params.delay_vc_to_retire)
        return false;
    message m = vc.get_head();
    // Set message completion time
    m.time.tick_req_complete = curr_tick;
    // Check if the message id of the VC head is present in the std map, or else raise an error
    CXL_ASSERT(messages_sent_to_device.count(m.msg_id) == 1 && "Response received is for an unknown request");
#ifdef DUMP
    // Store the completed memory access' timing data
    timing_tracker.insert({m.msg_id, m.time});
#endif
    auto& entry = amat_per_table_cxl[(m.address >> 48) & 0xFFF];
    auto& count = entry[0];
    auto& avg = entry[1];
    auto& avg_dram = entry[2];
    ++count;
    avg = avg + (m.time.tick_req_complete - m.time.tick_created - avg) / count;
    avg_dram = avg_dram + (m.time.tick_ramulator_complete - m.time.tick_at_ramulator - avg_dram) / count;
    
#ifdef TRACK_LATENCY
    // latency_data.push_back(m.time.tick_req_complete - m.time.tick_created);
    // ram_latency_data.push_back(m.time.tick_ramulator_complete - m.time.tick_at_ramulator);
    print_latency_verbose(m.time);
#endif

    // Remove message from the std map
    messages_sent_to_device.erase(m.msg_id);
    if (num_reqs_completed % 100000 == 0)
        std::cout << num_reqs_completed << " reqs completed!\n";
    // Increment the num reqs completed counter
    num_reqs_completed++;
    // Once requests are completed we can free internal credits to send to new devices
    switch (m.opCode)
    {
    case opcode::DRS:
        int_cred.data_credit++;
        break;
    case opcode::NDR:
        int_cred.rsp_credit++;
        break;
    default:
        CXL_ASSERT(false && "Illegal message type received");
    }
#ifdef EVENTLOG
    // Log the message completion
    log.CXLEventLog("Memory access completed " + m.sprint() + " [" + print_cred(int_cred) + "] " + "[" + print_cred(ext_creds[destination_device(m.address)]) + "]" + "\n", this->node_id);
#endif
    vc.dequeue();
    return true;
}

//! Function checks if there is any message in the received VCs and ensures that the address corresponds to a sent request and removes that request from the map
bool CXLHost::check_if_req_completed()
{
    bool active_cycle = true; /*!< Flag to tell if this function performs active task in this cycle*/
    // Check if receiving VCs are epmty
    if (S2M_DRS.is_buf_empty() && S2M_NDR.is_buf_empty())
        return false;
    // If there is a message in VC, check the queues using round robin
    switch (rcvd_rsp_state)
    {
    case round_robin_state::R:
        // First check RwD and then Req
        if (check_rx_vc(S2M_NDR)) // This functions adds reqs to ramulator inp buffer
            rcvd_rsp_state = round_robin_state::w;
        else if (check_rx_vc(S2M_DRS))
            rcvd_rsp_state = round_robin_state::r;
        else
            active_cycle = false;
        break;
    case round_robin_state::r:
        // First check Req and then RwD
        if (check_rx_vc(S2M_DRS)) // This functions adds reqs to ramulator inp buffer
            rcvd_rsp_state = round_robin_state::R;
        else if (check_rx_vc(S2M_NDR))
            rcvd_rsp_state = round_robin_state::w;
        else
            active_cycle = false;
        break;
    case round_robin_state::w:
        // First check Req and then RwD
        if (check_rx_vc(S2M_DRS)) // This functions adds reqs to ramulator inp buffer
            rcvd_rsp_state = round_robin_state::R;
        else if (check_rx_vc(S2M_NDR))
            rcvd_rsp_state = round_robin_state::w;
        else
            active_cycle = false;
        break;
    default:
        CXL_ASSERT(false && "Unknown state in round robin");
    }
    return active_cycle;
}

// //! Returns a message by reading the vc's nth element and checking if it satisfies latency checks
// bool CXLHost::check_vc_for_packing(CXLBuf<message> &vc, std::vector<message>::iterator &it)
// {
//     // Check if empty
//     if (vc.is_buf_empty())
//         return false;
//     // Check vc for num_read_ports and size
//     // Do a latency check
//     // If all is well, return that iterator
//     switch (vc.get_head().opCode)
//     {
//     case opcode::Req:
//         if (req_ctr >= num_read_ports || req_ctr >= vc.buf_occupancy())
//             return false;
//         if (curr_tick - vc.get_nth_ele(req_ctr).time.tick_created >= params.delay_vc_to_pack)
//         {
//             // Check if device under consideration matches the message we are checking
//             if (device_under_consideration != 4096 &&
//                 device_under_consideration != destination_device(vc.get_nth_ele(req_ctr).address))
//                 return false;
//             // Don't choose a message to whose destination device we dont have any external credits or message type for which we dont have internal credits
//             if (ext_creds[destination_device(vc.get_nth_ele(req_ctr).address)].req_credit == 0 || int_cred.data_credit == 0)
//                 return false;
//             it = vc.get_it() + req_ctr;
//             // // Decrement the request credits
//             // log.CXLEventLog("Decrement req creds\n", this->node_id);
//             // ext_creds[destination_device(vc.get_nth_ele(req_ctr).address)].req_credit--;
//             // // Decrement internal credits
//             // int_cred.data_credit--;
//             req_ctr++;
//             return true;
//         }
//         req_ctr++;
//         break;
//     case opcode::RwD:
//         if (rwd_ctr >= num_read_ports || rwd_ctr >= vc.buf_occupancy())
//             return false;
//         if (curr_tick - vc.get_nth_ele(rwd_ctr).time.tick_created >= params.delay_vc_to_pack)
//         {
//             // Check if device under consideration matches the message we are checking
//             if (device_under_consideration != 4096 &&
//                 device_under_consideration != destination_device(vc.get_nth_ele(rwd_ctr).address))
//                 return false;
//             // Don't choose a message to whose destination device we dont have any external credits or message type for which we dont have internal credits
//             if (ext_creds[destination_device(vc.get_nth_ele(rwd_ctr).address)].data_credit == 0 || int_cred.rsp_credit == 0)
//                 return false;
//             it = vc.get_it() + rwd_ctr;
//             // // Decrement the data credits
//             // ext_creds[destination_device(vc.get_nth_ele(rwd_ctr).address)].data_credit--;
//             // log.CXLEventLog("Decrement data creds\n", this->node_id);
//             // // Decrement internal credits
//             // int_cred.rsp_credit--;
//             rwd_ctr++;
//             return true;
//         }
//         rwd_ctr++;
//         break;
//     default:
//         CXL_ASSERT(false && "Illegal message type");
//     }
//     return false;
// }

// bool CXLHost::get_next_message_to_transmit(std::vector<message>::iterator &it, bool &valid)
// {
//     if (req_ctr >= num_read_ports && rwd_ctr >= num_read_ports)
//         return true;
//     if (req_ctr >= M2S_Req.buf_occupancy() && rwd_ctr >= M2S_RWD.buf_occupancy())
//         return true;
//     std::vector<message>::iterator temp;
//     // Round robin the order of looking for the messages in the VCs
//     switch (pkr2_state)
//     {
//     case round_robin_state::r:
//         if (check_vc_for_packing(M2S_Req, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::R;
//         }
//         else if (check_vc_for_packing(M2S_RWD, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::w;
//         }
//         else
//             valid = false;
//         if (valid)
//             it = temp;

//         break;
//     case round_robin_state::R:
//         if (check_vc_for_packing(M2S_RWD, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::w;
//         }
//         else if (check_vc_for_packing(M2S_Req, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::R;
//         }
//         else
//             valid = false;
//         if (valid)
//             it = temp;
//         break;
//     case round_robin_state::w:
//         if (check_vc_for_packing(M2S_Req, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::r;
//         }
//         else if (check_vc_for_packing(M2S_RWD, temp))
//         {
//             valid = true;
//             pkr2_state = round_robin_state::w;
//         }
//         else
//             valid = false;
//         if (valid)
//             it = temp;
//         break;
//     default:
//         CXL_ASSERT(false && "Illegal state in round robin");
//     }
//     return false;
// }

//! Just a function to reduce repeated lines of code in sending a packed flit to tx buffer
void CXLHost::add_flit_to_buf(flit &f)
{
    // Set time of creation
    f.time.time_of_creation = curr_tick;
    // Set the credits
    f.header.credit = get_Host_credits();
    f.set_time(&msg_timing::tick_packed, curr_tick);
    // Assign flit ID and increment flit counter
    f.flit_id = CXL::flit::flit_counter;
    CXL::flit::flit_counter++;
    // Send to tx buf
    tx_buffer.enqueue(f);
#ifdef EVENTLOG
    // Log it
    log.CXLEventLog("Host packed flit " + f.sprint() + " [" + print_cred(int_cred) + "] " + "[" + print_cred(ext_creds[device_under_consideration]) + "]" + "\n", this->node_id);
#endif
}

//! Do a sanity check on the host's internal credit counters
void CXLHost::credit_sanity_check()
{
    CXL_ASSERT(int_cred.req_credit == 0 && "Req cred of host should be 0");
    CXL_ASSERT((int_cred.rsp_credit >= 0 && int_cred.rsp_credit <= S2M_NDR.size()) && "Rsp cred of host out of range");
    CXL_ASSERT((int_cred.data_credit >= 0 && int_cred.data_credit <= S2M_DRS.size()) && "Data cred of host out of range");
}

//! Once flit is packed, we need to put the credits host is willing to allot into the header. Also need to decrement our internal credit counter
credits CXLHost::get_Host_credits()
{
    int rsp_credit = 0, data_credit = 0; /*!< These are the ones which will be sent out this cycle*/
    // Sanity checks
    credit_sanity_check();
    // Send out only those many credits as you have messages in your flit
    // I am making an assumption here that we have packed those messages because we knew we had internal and external credits for them
    // Rsp credits
    rsp_credit = flit_to_pack.slot_count_in_flit(slot_type::m2s_rwd_hdr); // How many rwd we packed
    data_credit = flit_to_pack.slot_count_in_flit(slot_type::m2s_req);    // How many req we packed
    credits crds = {0, rsp_credit, data_credit};
    return crds;
}

bool CXLHost::check_vcs_for_packing(std::array<CXLBuf<message>, NUM_VC> &vcs, CXLBuf<message> *&vc)
{
    for (int i = packer_cur_vc; i < NUM_VC + packer_cur_vc; i++)
    {
        if (!vcs[i % NUM_VC].is_buf_empty() && curr_tick - vcs[i % NUM_VC].get_head().time.tick_created >= params.delay_vc_to_pack)
        {
            int dest = destination_device(vcs[i % NUM_VC].get_head().address);
            if (device_under_consideration == 4096 || device_under_consideration == dest)
            {
                switch (vcs[i % NUM_VC].get_head().opCode)
                {
                case opcode::Req:
                    total_credit_checks++;
                    if (int_cred.data_credit == 0)
                        int_credit_reject_ctr++;
                    if (ext_creds[dest].req_credit == 0)
                        ext_credit_reject_ctr++;
                    if (ext_creds[dest].req_credit != 0 && int_cred.data_credit != 0)
                    {
                        vc = &vcs[i % NUM_VC]; // assign vc
                        packer_cur_vc = packer_cur_vc == NUM_VC - 1 ? 0 : i % NUM_VC + 1;
                        // cout << packer_cur_vc << "@\n";
                        return true;
                    }
                    break;
                case opcode::RwD:
                    if (ext_creds[dest].data_credit != 0 && int_cred.rsp_credit != 0)
                    {
                        vc = &vcs[i % NUM_VC]; // assign vc
                        packer_cur_vc = packer_cur_vc == NUM_VC - 1 ? 0 : i % NUM_VC + 1;
                        // cout << packer_cur_vc << " *&\n";
                        return true;
                    }
                    break;
                }
            }
        }
    }
    return false;
}

void CXLHost::get_msg_from_vcs(CXLBuf<message> *&vc, bool &valid)
{
    switch (pkr2_state)
    {
    case round_robin_state::r:
        if (check_vcs_for_packing(M2S_Req, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::R;
        }
        else if (check_vcs_for_packing(M2S_RWD, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::w;
        }
        else
            valid = false;
        break;
    case round_robin_state::R:
        if (check_vcs_for_packing(M2S_RWD, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::w;
        }
        else if (check_vcs_for_packing(M2S_Req, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::R;
        }
        else
            valid = false;
        break;
    case round_robin_state::w:
        if (check_vcs_for_packing(M2S_Req, vc))
        {
            valid = true;
            pkr2_state = round_robin_state::r;
        }
        else if (check_vcs_for_packing(M2S_RWD, vc))
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

//! Function to check if we can skip calling the packer for this cycle. Returns true if we can skip the cycle
bool CXLHost::skip_packer_check()
{
    // Packer can be skipped based on following conditions
    // If the tx VCs are both empty and
    // If there is no rollover
    // If there are messages in tc VCs then we need to make sure that the first #read port number of messages do not satisfy latency constraints
    if (packer_rollover != 0 || is_packer_waiting)
        return false;
    // If all tx VCs are empty, we can skip cycle
    bool flag = true;
    for (int i = 0; i < NUM_VC; i++)
    {
        if (!M2S_Req[i].is_buf_empty() || !M2S_RWD[i].is_buf_empty())
            flag = false;
    }
    if (flag)
        return true;
    // Now check the heads of all tx VCs
    // If there are any latency satisfying messages then this cycle cannot be guaranteed to be inactive so return false
    for (int i = 0; i < NUM_VC; i++)
    {
        // Check Req channel
        if (!M2S_Req[i].is_buf_empty() && curr_tick - M2S_Req[i].get_head().time.tick_created >= params.delay_vc_to_pack)
            return false;
        // Check RWD channel
        if (!M2S_RWD[i].is_buf_empty() && curr_tick - M2S_RWD[i].get_head().time.tick_created >= params.delay_vc_to_pack)
            return false;
    }
    // We have checked all conditions now, can return true at this point
    return true;
}

//! Function to check if we can skip calling the transmit function for this cycle. Returns true if we can skip the cycle
bool CXLHost::skip_transmit_check()
{
    // Transmit can be skipped if
    // 1. tx buffer is empty
    // 2. If tx bus is fmull
    // 3. If tx buffer is not empty then check if head of the buffer has satisfied latency constraints or buffer is not exceeding bus bandwidth
    return ((tx_buffer.is_buf_empty() || tx_bus->is_full()) ||
            (!tx_buffer.is_buf_empty() &&
             (curr_tick - tx_buffer.get_head().time.time_of_creation < params.delay_tx_buf_to_bus ||
              curr_tick - last_transmitted_at < params.cxl_bus_ticks_per_dequeue)));
}

//! Function to check if we can skip calling the unpacker for this cycle. Returns true if we can skip the cycle
bool CXLHost::skip_unpacker_check()
{
    // To skip cycle for unpacker need to check the following
    // 1. Check if rx buffer is empty? If it is empty then it is definitely a skippable cycle
    // 2. In case rx buffer is not empty, check the head element and see if the head element satisfies latency requirements. If it doesn't then it is a skippable cycle
    return (rx_buffer.is_buf_empty() ||
            (!rx_buffer.is_buf_empty() && (curr_tick - rx_buffer.get_head().time.time_of_receipt < params.delay_rx_buf_to_unpack)));
}

//! Function to check if we can skip calling the check_if_req_completed function for this cycle. Returns true if we can skip the cycle
bool CXLHost::skip_req_complete_check()
{
    // To skip req completed function we need to check the following
    // 1. Are the Rx VCs empty?? If both are empty then it is definitely a skippable cycle
    // 2. In case both VCs are not empty, check the head element in both VCs and skip only if both heads do not satisfy latency requirements

    if (S2M_DRS.is_buf_empty() && S2M_NDR.is_buf_empty())
        return true;
    if (!S2M_DRS.is_buf_empty() && curr_tick - S2M_DRS.get_head().time.tick_resp_unpacked >= params.delay_vc_to_retire)
        return false;
    if (!S2M_NDR.is_buf_empty() && curr_tick - S2M_NDR.get_head().time.tick_resp_unpacked >= params.delay_vc_to_retire)
        return false;
    return true;
}

//! Packs messages from VCs into flits
bool CXLHost::FLIT_packer()
{
    bool active_flag = true; /*!< This flag indicates if during this cycle the packer is active or not*/
    // If we need to create a new flit, it necessarily doesnt mean that the packer is inactive
    // If we dont have a half packed flit, create a new filt
    if (!is_packer_waiting)
    {
        flit_to_pack = flit();
        started_packing_at = curr_tick;
        // Reset device under consideration if there is no rollover else maintain same device under consideration
        device_under_consideration = packer_rollover > 0 ? device_under_consideration : 4096; // 4096 is chosen because it is a large enough number that we wont have these many devices attached
    }
    // If we are sending a flit out, then it definitely means the packer is active in this cycle
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
    // If we have exceeded the waiting time and the current packed flit needs to go, it means packer is still active
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

    // If we are handling rollover, it still means that the packer is active for this cycle
    // If there is rollover from previous cycle and this is a brand new flit, do that first
    if (packer_rollover != 0 && !is_packer_waiting)
    {
        switch (packer_rollover)
        {
        case 1:
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_rwd_hdr.address;
            packer_rollover--;
            break;
        case 2:
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[2].msg.address = last_rwd_hdr.address;
            packer_rollover -= 2;
            break;
        case 3:
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[3].type = slot_type::data;
            flit_to_pack.slots[1].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[3].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[1].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[2].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[3].msg.address = last_rwd_hdr.address;
            packer_rollover -= 3;
            break;
        case 4:
            flit_to_pack.slots[0].type = slot_type::data;
            flit_to_pack.slots[1].type = slot_type::data;
            flit_to_pack.slots[2].type = slot_type::data;
            flit_to_pack.slots[3].type = slot_type::data;
            flit_to_pack.slots[0].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[1].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[2].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[3].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[0].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[1].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[2].msg.address = last_rwd_hdr.address;
            flit_to_pack.slots[3].msg.address = last_rwd_hdr.address;
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
    // std::vector<message>::iterator it;
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
            flit_to_pack.slots[i].type = slot_type::data;
            flit_to_pack.slots[i].msg.msg_id = last_rwd_hdr.msg_id;
            flit_to_pack.slots[i].msg.address = last_rwd_hdr.address;
            packer_rollover--;
            continue;
        }
        // Try to get a message from the VCs
        get_msg_from_vcs(vc, valid);
        if (!valid)
            break; // Break from this loop if flag is true, flag is true means there is no possible message that can be packed
        // Break from loop if valid is true. Reset the ctrs
        if (valid)
        {
            // Set device under consideration if not already set
            device_under_consideration = device_under_consideration == 4096 ? destination_device(vc->get_head().address) : device_under_consideration;
        }
        if (valid)
        {
            switch (vc->get_head().opCode)
            {
            case opcode::Req:
                // Can't have more than two requests in a single flit
                if (flit_to_pack.slot_count_in_flit(slot_type::m2s_req) <= 1)
                {
                    flit_to_pack.slots[i].type = slot_type::m2s_req;
                    flit_to_pack.slots[i].msg.copy(vc->get_head());
#ifdef EVENTLOG
                    // Decrement the request credits
                    log.CXLEventLog("Decrement req creds\n", this->node_id);
#endif
                    ext_creds[destination_device(vc->get_head().address)].req_credit--;
                    // Erase from VC
                    vc->dequeue();
                    // Decrement internal credits
                    int_cred.data_credit--;
                    continue;
                }
                break;
            case opcode::RwD:
                // We can add an rwd header to a flit's slot 0 as long as there is no other rwd header in the same flit
                if ((flit_to_pack.slot_count_in_flit(slot_type::m2s_rwd_hdr) == 0 && i == 0) ||
                    (flit_to_pack.slot_count_in_flit(slot_type::m2s_rwd_hdr) == 0 && flit_to_pack.are_all_slots_after_this_empty(i)))
                {
                    flit_to_pack.slots[i].type = slot_type::m2s_rwd_hdr;
                    last_rwd_hdr.copy(vc->get_head());
                    flit_to_pack.slots[i].msg.copy(vc->get_head());
                    packer_rollover = 4;
                    // Decrement the data credits
                    ext_creds[destination_device(vc->get_head().address)].data_credit--;
#ifdef EVENTLOG
                    log.CXLEventLog("Decrement data creds\n", this->node_id);
#endif
                    // Decrement internal credits
                    int_cred.rsp_credit--;
                    vc->dequeue();
                    continue; // do we need this continue?
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
    int req_count = flit_to_pack.slot_count_in_flit(slot_type::m2s_req);
    int rwd_count = flit_to_pack.slot_count_in_flit(slot_type::m2s_rwd_hdr);
    int data_count = flit_to_pack.slot_count_in_flit(slot_type::data);

    // Packing rule check
    bool flag = false; // True means some error has occured;
    if (req_count > 2 || req_count < 0)
        flag = true;
    if (data_count > 4 || data_count < 0)
        flag = true;
    if (rwd_count > 1 || rwd_count < 0)
        flag = true;
    if (rwd_count + data_count + req_count > 4)
        flag = true;
    CXL_ASSERT(!flag && "Packing rule violated on packer side");
    return active_flag;
}

//! Returns node id of the destination CXL Device by looking at the address
uint CXLHost::destination_device(uint64_t addr)
{
    uint64_t dest = 0;
    bool found = false;

    for (auto &iter : this->address_intervals)
    {
        // check if this is correct, do we need to consider the offset, in this case, 8 bytes?
        if (addr < iter.second.second && addr >= iter.second.first)
        {
            found = true;
            dest = iter.first;
            break;
        }
    }
    CXL_ASSERT(found == true && "Invalid address");
    return dest;
}

//! Dump all the message timing data gathered from completed memory accesses
void CXLHost::dump_data()
{
    // Create new file
    ofstream dump("dump.csv");

    // First line is the header
    dump << "msg_id,R/W,tick_created,tick_packed,tick_transmitted,tick_received,tick_unpacked,tick_at_ramulator,tick_ramulator_complete,tick_repacked,tick_retransmitted,tick_resp_received,tick_resp_unpacked,tick_req_complete,ramulator_clk_start,ramulator_clk_end\n";

    // Put the data
    for (std::map<uint64_t, msg_timing>::iterator it = timing_tracker.begin(); it != timing_tracker.end(); ++it)
    {
        dump << it->first << ","
             << it->second.read_write << ","
             << it->second.tick_created << ","
             << it->second.tick_packed << ","
             << it->second.tick_transmitted << ","
             << it->second.tick_received << ","
             << it->second.tick_unpacked << ","
             << it->second.tick_at_ramulator << ","
             << it->second.tick_ramulator_complete << ","
             << it->second.tick_repacked << ","
             << it->second.tick_retransmitted << ","
             << it->second.tick_resp_received << ","
             << it->second.tick_resp_unpacked << ","
             << it->second.tick_req_complete << ","
             << it->second.ramulator_clk_start << ","
             << it->second.ramulator_clk_end << "\n";
    }
}

//! Print out all the message end to end latencies
void CXLHost::dump_latency()
{
    // Create new file
    ofstream lat("stats.csv");
    std::cout << "Num Reqs Fulfilled: " << latency_data.size() << "\n";
    for (int i = 0; i < latency_data.size(); i++)
    {
        lat << latency_data[i] << "," << ram_latency_data[i] << "\n";
    }
    lat.close();
}

//! Print all the skipped cycles for each of the component functions within host update
void CXLHost::print_skipped_cycles()
{
    std::cout << "Summary of skipped cycles in cxl host\n";
    std::cout << "Packer " << empty_cycle_packer << "\n";
    std::cout << "Unpacker " << empty_cycle_unpacker << "\n";
    std::cout << "Transmit " << empty_cycle_transmit << "\n";
    std::cout << "Complete " << empty_cycle_req_completed << "\n";
    std::cout << "Skippable cycle " << skippable_cycle << "\n";
}

//! Print very verbose latency information as a csv file
#ifdef TRACK_LATENCY
void CXLHost::print_latency_verbose(msg_timing &t)
{
    // Create file
    ofstream timing(latency_file_prefix + "_cxl.csv", std::ios_base::app);

    timing << t.read_write << ","
           << t.tick_created << ","
           << t.tick_packed << ","
           << t.tick_transmitted << ","
           << t.tick_switch_ds_rx << ","
           << t.tick_switch_ds_tx << ","
           << t.tick_received << ","
           << t.tick_unpacked << ","
           << t.tick_at_ramulator << ","
           << t.tick_ramulator_complete << ","
           << t.tick_repacked << ","
           << t.tick_retransmitted << ","
           << t.tick_switch_us_rx << ","
           << t.tick_switch_us_tx << ","
           << t.tick_resp_received << ","
           << t.tick_resp_unpacked << ","
           << t.tick_req_complete << ","
           << t.ramulator_clk_start << ","
           << t.ramulator_clk_end << "\n";

    timing.close();
}
#endif

#ifdef TRACK_LATENCY
void CXLHost::print_direct_attached_latency(uint64_t msg_id)
{
    ofstream dam_lat(latency_file_prefix + "_dam.csv", std::ios_base::app);

    dam_lat << reqs_in_dam[msg_id] << "," << curr_tick << "\n";
}
#endif