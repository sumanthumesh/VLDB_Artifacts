#ifndef __CXL_SWITCH_H
#define __CXL_SWITCH_H

#include "CXLNode.h"
#include "message.h"
#include "flit.h"
#include <vector>
#include <cstdint>
#include "utils.h"
#include <iostream>
#include "CXLBuf.h"
#include "CXLBus.h"
#include <map>

namespace CXL
{
    /*! One host version switch */
    class CXLSwitch
    {
    public:
        /*! constructor, pass in a vector of even size, containing start and end addr of device 0,1... */
        CXLSwitch(){};
        CXLSwitch(uint64_t buf_size, uint node_id);
        CXLBuf<flit> upstream_buffer_tx;
        CXLBuf<flit> upstream_buffer_rx;
        std::map<uint64_t, CXLBuf<flit>> downstream_buffers_tx;
        std::map<uint64_t, CXLBuf<flit>> downstream_buffers_rx;
        void connect_upstream(CXLBus *bus_to_host, CXLBus *bus_from_host);
        void connect_downstream(CXLBus *bus_to_device, CXLBus *bus_from_device, uint64_t device_id, const std::pair<uint64_t, uint64_t> &address_interval);
        void disconnect_downstream(uint64_t device_id);
        void update();
        uint node_id;
        bool check_transmission(uint64_t l_t, CXLBus *);
        void dump_latency();

    private:
        uint64_t previous_destination;
        uint64_t last_transmission;
        std::map<uint64_t, uint64_t> downstream_last_transmission;
        uint64_t destination_device(uint64_t addr);
        bool check_buffer_condition(CXLBuf<flit> &, uint64_t delay, bool &flag);
        uint64_t buf_size;
        std::map<uint64_t, CXLBus *> connected_downstream_tx;
        std::map<uint64_t, CXLBus *> connected_downstream_rx;
        CXLBus *bus_to_host;
        CXLBus *bus_from_host;
        uint64_t num_devices;
        // uint8_t RR_state; // round robin not implement, need further consideration
        CXLBuf<flit> ARB_NOC_h2d;
        CXLBuf<flit> ARB_NOC_d2h;
        std::map<uint64_t, std::pair<uint64_t, uint64_t>> address_intervals;
        // device statemachine module
        uint64_t last_port;
        uint64_t curr_port;
        uint64_t expected_rollover;
        void update_port();
        void update_rollover(const flit &f);
        std::vector<int64_t> time_in_switch;
        bool skip_cycle;
        bool all_rx_buf_empty();
    };
}
#endif