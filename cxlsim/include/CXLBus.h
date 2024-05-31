#ifndef __CXL_BUS_H
#define __CXL_BUS_H

#include "flit.h"
#include "CXLBuf.h"
#include <memory>
#include <string>
#include <map>
#include <deque>
// #include "CXLNode.h"

namespace CXL
{
    class CXLNode;

    //! Enum to indicate whether upstream or downstream
    enum direction
    {
        UpStream,
        DwStream
    };

    //! This enum specifies what the ends of the bus are
    enum bus_terminals
    {
        //We will be using a <to>_<from> notation
        host_device,
        device_host,
        host_switch,
        switch_host,
        device_switch,
        switch_device
    };

    // Things to track when a flit is in the bus
    typedef struct
    {
        int64_t tick_added_to_bus;
        int64_t tick_removed_from_bus;
        uint num_data;
        uint num_empty;
    } bus_timings;

    class CXLBus
    {
    private:
        int size;               /*!< Number of slots. Used to model latency*/
        int ticks_per_dequeue;  /*!< Number of ticks a flit needs to wait in the bus before being dequeued. Used to model bandwidth*/
        int bus_latency;        /*!< Max number of ticks upto which a flit can stay in the bus*/
        CXLBuf<flit> *from_buf; /*!< CXL Endpoint adding data to BUS*/
        CXLBuf<flit> *to_buf;   /*!< CXL Endpoint receiving data from BUS*/
        std::deque<flit> flit_slots;
        uint from_node_id; /*!< Node id of the CXL Node putting flits on the bus*/
        uint to_node_id;   /*!< Node id of the CXL Node which will receive flits from the bus*/
        direction dir;
        std::map<uint64_t, bus_timings> timing_tracker; /*!< To store metrics on flits passing through the bus*/
    public:
        int64_t time_of_last_dequeue; /*!< Tick at which last dequeue occured*/

        bus_terminals bus_type;

        // Constructor and destructor
        CXLBus(){};
        CXLBus(int size, uint node_id, direction d);

        // Connect nodes
        void connect_from(CXLBuf<flit> *from);
        void connect_to(CXLBuf<flit> *to);

        // Set from and to node ids
        void set_from_to_node_id(uint from, uint to);

        // Verify connections
        void check_connections();

        // Check if flits are stagnant
        bool is_flit_stagnant();

        // Check size of bus
        int bus_occupancy();
        bool is_full();
        bool is_empty();

        // Add or remove from bus
        void add_to_bus(flit f);
        void send_to_node();

        // Top level update function
        void update();

        // For testing purpose
        void fill_bus(std::vector<flit> &flits);

        // Set whether upstream or downstream buffer
        void set_dir(direction d);

        int max_size();
        uint node_id; /*!< Unique ID assigned to every single instantiated object in the main loop*/

        void dump_data();
    };
}
#endif