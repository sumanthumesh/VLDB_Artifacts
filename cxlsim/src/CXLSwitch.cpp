#include "CXLNode.h"
#include "message.h"
#include "CXLSwitch.h"
#include "flit.h"
#include <vector>
#include <cstdint>
#include "utils.h"
#include "CXLParams.h"

using namespace CXL;

CXLSwitch::CXLSwitch(uint64_t buf_size, uint node_id)
{
    num_devices = 0;
    this->buf_size = buf_size;
    ARB_NOC_d2h = CXLBuf<flit>(buf_size);
    ARB_NOC_h2d = CXLBuf<flit>(buf_size);
    previous_destination = 0;
    curr_port = 0;
    expected_rollover = 0;
    this->node_id = node_id;
    last_transmission = 0;
    skip_cycle = false;
}

void CXLSwitch::connect_upstream(CXLBus *bus_to_host, CXLBus *bus_from_host)
{
    this->bus_to_host = bus_to_host;
    this->bus_from_host = bus_from_host;
    upstream_buffer_tx = CXLBuf<flit>(buf_size);
    upstream_buffer_rx = CXLBuf<flit>(buf_size);
}

void CXLSwitch::connect_downstream(CXLBus *bus_to_device, CXLBus *bus_from_device, uint64_t device_id, const std::pair<uint64_t, uint64_t> &address_interval)
{
    CXL_ASSERT(this->connected_downstream_tx.find(device_id) == this->connected_downstream_tx.end() && "Already connected");
    this->connected_downstream_tx[device_id] = bus_to_device;
    this->connected_downstream_rx[device_id] = bus_from_device;
    this->address_intervals[device_id] = address_interval;
    this->downstream_buffers_tx[device_id] = CXLBuf<flit>(buf_size);
    this->downstream_buffers_rx[device_id] = CXLBuf<flit>(buf_size);
    downstream_last_transmission[device_id] = 0;
    num_devices++;
}

void CXLSwitch::disconnect_downstream(uint64_t device_id)
{
    CXL_ASSERT(this->connected_downstream_tx.find(device_id) != this->connected_downstream_tx.end() && "Not connected");
    this->connected_downstream_tx.erase(device_id);
    this->connected_downstream_rx.erase(device_id);
    this->address_intervals.erase(device_id);
    this->downstream_buffers_tx.erase(device_id);
    this->downstream_buffers_rx.erase(device_id);
    num_devices--;
}

uint64_t CXLSwitch::destination_device(uint64_t addr)
{
    uint64_t dest;
    bool found = false;
    for (auto &iter : this->address_intervals)
    {
        // check if this is correct, do we need to consider the offset, in this case, 8 bytes?
        if (addr < iter.second.second && addr >= iter.second.first)
        {
            found = true;
            dest = iter.first;
            if (addr == 0x064d4840)
                cout << "DEBUG: " << dest << "\n";
            break;
        }
    }
    CXL_ASSERT(found == true && "Invalid address");
    return dest;
}

bool CXLSwitch::check_buffer_condition(CXLBuf<flit> &buf, uint64_t delay, bool &flag)
{
    flag &= buf.is_buf_empty();
    return !buf.is_buf_empty() && buf.latency_check_flit(delay);
}

bool CXLSwitch::check_transmission(uint64_t l_t, CXLBus *bus)
{
    if (bus->is_full())
        return false;
    if (curr_tick - l_t < params.cxl_bus_ticks_per_dequeue)
        return false;
    return true;
}

bool CXLSwitch::all_rx_buf_empty()
{
    bool flag = true;
    for (int i = 0; i < num_devices; i++)
    {
        flag &= downstream_buffers_rx[i].is_buf_empty();
    }
    return flag & upstream_buffer_rx.is_buf_empty();
}

void CXLSwitch::update()
{
#ifdef SKIP_CYCLE
    if (skip_cycle == true && all_rx_buf_empty())
        return;
#endif
    bool flag = true;

    // std::cout<<"Inside switch\n";

    // bus pushes to upstream buffer
    flit f;
    // host to device flow
    if (check_buffer_condition(upstream_buffer_rx, params.delay_cxl_port_switch, flag))
    {
        f.copy(upstream_buffer_rx.get_head());
        upstream_buffer_rx.dequeue();
        f.time.time_of_receipt = curr_tick;//set receipt not transmission for check_latency_flit
        ARB_NOC_h2d.enqueue(f);
    }
    if (check_buffer_condition(ARB_NOC_h2d, params.delay_cxl_noc_switch, flag))
    {
        uint64_t destination_decode = ARB_NOC_h2d.get_head().slots[0].type == data ? previous_destination : destination_device(ARB_NOC_h2d.get_head().get_first_address());
        previous_destination = destination_decode;
        f.copy(ARB_NOC_h2d.get_head());
        ARB_NOC_h2d.dequeue();
        f.time.time_of_receipt = curr_tick;//set receipt not transmission for check_latency_flit
        downstream_buffers_tx[destination_decode].enqueue(f);
    }
    for (auto &iter : downstream_buffers_tx)
    {
        if (check_buffer_condition(iter.second, params.delay_cxl_port_switch, flag) && check_transmission(downstream_last_transmission[iter.first], connected_downstream_tx[iter.first]))
        {
            f.copy(iter.second.get_head());
            iter.second.dequeue();
            f.time.time_of_transmission = curr_tick;
            downstream_last_transmission[iter.first] = curr_tick;
            // cout << f.time.time_of_transmission << " " << curr_tick << endl;
            f.set_time(&msg_timing::tick_switch_ds_tx, curr_tick);
            this->connected_downstream_tx[iter.first]->add_to_bus(f);
        }
    }
    // device to host flow
    //  for(auto& iter : downstream_buffers_rx)
    //  {
    //      if(check_buffer_condition(iter.second, params.delay_cxl_port_switch))
    //      {
    //          f.copy(iter.second.dequeue());
    //          f.time.time_of_transmission = curr_tick;
    //          ARB_NOC_d2h.enqueue(f);
    //      }
    //  }
    for (int i = 0; i < num_devices; ++i)
    {
        // cout << "checking port" << curr_port << "@@" << endl;
        if (check_buffer_condition(downstream_buffers_rx[curr_port], params.delay_cxl_port_switch, flag))
        {
            f.copy(downstream_buffers_rx[curr_port].get_head());
            downstream_buffers_rx[curr_port].dequeue();
            // cout << curr_port << "!!!!!" << endl;
            f.time.time_of_receipt = curr_tick;//set receipt not transmission for check_latency_flit
            ARB_NOC_d2h.enqueue(f);
            update_rollover(f);
            update_port();
            break;
        }
        update_port();
    }
    if (check_buffer_condition(ARB_NOC_d2h, params.delay_cxl_noc_switch, flag))
    {
        f.copy(ARB_NOC_d2h.get_head());
        ARB_NOC_d2h.dequeue();
        f.time.time_of_receipt = curr_tick;//set receipt not transmission for check_latency_flit
        upstream_buffer_tx.enqueue(f);
    }
    if (check_buffer_condition(upstream_buffer_tx, params.delay_cxl_port_switch, flag) && check_transmission(last_transmission, bus_to_host))
    {
        f.copy(upstream_buffer_tx.get_head());
        upstream_buffer_tx.dequeue();
        f.time.time_of_transmission = curr_tick;
        last_transmission = curr_tick;
        f.set_time(&msg_timing::tick_switch_us_tx, curr_tick);
        this->bus_to_host->add_to_bus(f);
    }
// RR_state = RR_state < num_devices ? RR_state + 1 : 0;
#ifdef SKIP_CYCLE
    skip_cycle = flag;
#endif
}

void CXLSwitch::update_port()
{
    if (expected_rollover == 0)
    {
        // cout << "currport " << curr_port << " >> ";
        curr_port = curr_port < this->num_devices - 1 ? curr_port + 1 : 0;
        // cout << "currport " << curr_port << endl;
    }
}

void CXLSwitch::update_rollover(const flit &f)
{
    for (int i = 0; i < 4; ++i)
    {
        // cout << "curr_rollover: " << expected_rollover << ">>";
        if (f.header.slots[i] == s2m_drs_hdr)
        {
            expected_rollover += 4;
        }
        else if (f.header.slots[i] == data)
        {
            expected_rollover -= 1;
        }
        // cout << "curr_rollover: " << expected_rollover << endl;
    }
}