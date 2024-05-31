#ifndef __CXL_PARAMS_H
#define __CXL_PARAMS_H

#include <cstdint>

namespace CXL
{
    class CXLParams
    {
    public:

        int64_t spec_bandwidth; /*!< Per lane BW in Byte per second*/
        int bytes_per_slot; 
        int slots_per_flit;
        int bytes_per_flit;
        int64_t ticks_per_ns;
        int64_t ticks_per_ins; /*!< CPI expressed in terms of ticks*/

        int link_width;
        int64_t cxl_bus_total_latency_ns;     /*!< Time taken to in ns to travel the bus*/
        int64_t delay_tx_buf_to_bus_ns;
        int64_t delay_rx_buf_to_unpack_ns;
        int64_t delay_vc_to_pack_ns;
        int64_t delay_vc_to_ramulator_ns;
        int64_t delay_vc_to_retire_ns;
        int64_t delay_cxl_noc_switch_ns;
        int64_t delay_cxl_port_switch_ns;
        float ramulator_update_delay_ns;


        int bus_size;
        int64_t cxl_bus_total_latency;     /*!< Time taken to in ns to travel the bus*/
        int64_t link_bandwidth_s;
        int64_t link_bandwidth; /*!< Bandwidth in bytes/tick*/
        int64_t delay_tx_buf_to_bus;
        int64_t delay_rx_buf_to_unpack;
        int64_t delay_vc_to_pack;
        int64_t delay_ramulator_update;
        int64_t delay_vc_to_ramulator;
        int64_t delay_vc_to_retire;
        int64_t delay_cxl_port_switch;
        int64_t delay_cxl_noc_switch;



        int64_t cxl_bus_ticks_per_dequeue; // used to model bandwidth

        void recalculate();

        void print();

        // CXLParams(int64_t spec_bandwidth, /*!< Per lane BW in Byte per second*/
        // int bytes_per_slot, 
        // int slots_per_flit,
        // int bytes_per_flit,
        // int64_t ticks_per_ns,
        // int link_width,
        // int64_t cxl_bus_total_latency_ns,     /*!< Time taken to in ns to travel the bus*/
        // int64_t delay_tx_buf_to_bus_ns,
        // int64_t delay_rx_buf_to_unpack_ns) : spec_bandwidth(spec_bandwidth), bytes_per_slot(bytes_per_slot), 
    };

}
#endif