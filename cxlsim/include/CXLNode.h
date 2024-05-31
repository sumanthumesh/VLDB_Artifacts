#ifndef __CXL_NODE_H
#define __CXL_NODE_H

#define VC_SIZE 10000
#define REQ_BUFFER_SIZE 10000
#define RX_TX_SIZE 10000
#define NUM_VC 4

#include <vector>
#include <cstdint>
#include <array>
#include "message.h"
#include "flit.h"
#include "CXLBus.h"
#include "CXLBuf.h"
#include "RamDevice.h"
#include <utility>
#include <map>
#include <list>
#include <algorithm>
#include <fstream>
#include <sstream>

// class RamDevice;

namespace CXL
{

    enum type_packed
    {
        None,
        D,
        ND
    };

    /*!
    CXL node parent class
    */
    class CXLNode
    {
    public:
        /*!
        \brief Check the state of the node, do operations accordingly
        \param tick universal tick
        */
        virtual void update() = 0;
        /*!
        \brief Connect node to bus (Shared by both host and device)
        \param bus a bus pointer pointing to the connected bus
        */
        void connect_tx(CXLBus *bus);
        void connect_rx(CXLBus *bus);
        CXLBuf<flit> tx_buffer; /*!< access by bus*/
        CXLBuf<flit> rx_buffer; /*!< access by bus*/
        CXLNode(){};
        CXLNode(int vc_size, int buf_size);
        void add_to_rxbuf(flit f);
        bool check_connection();

        uint node_id;                     /*!< Unique ID assigned to every single instantiated object in the main loop*/
        std::map<int, credits> ext_creds; /*!< Keeping track of credits alloted to you by other CXL Nodes*/
        credits int_cred;                 /*!< Keeping track of credits you have available to give to other devices*/

    protected:
        /*!
        check if we can send out a msg
        check latency/credit to determine if we can transmit the first message in the tx buffer
        */
        // virtual bool check_send() = 0;
        /*!
        check if we can pack messages to a flit
        check latency
        */
        virtual bool check_pack() = 0;
        /*!
        check if we can unpack a flitflit FLIT_2_send()
        check latency/tx_buffer occupancy
        */
        virtual bool check_unpack() = 0;
        /*!
        pack one flit based on the messages we have in virtual channels
        */
        // virtual void FLIT_packer() = 0;
        /*!
        unpack a flit to a vector of messages
        */
        // virtual std::vector<message> FLIT_unpacker() = 0;
        /*!
        FLIT transmitting rules
        */
        virtual flit FLIT_2_send() = 0;
        /*!
        error checking
        */
        virtual bool CRC(flit f) = 0;
        int packer_cur_vc;
        CXLRETRYBuf resend_buf; /*!<Buffer storing flits for resending*/
        CXLBus *tx_bus;         /*!< tx bus pointer, set by the connect function */
        CXLBus *rx_bus;         /*!< rx bus pointer, set by the connect function */
        int64_t counter;        /*!< counter to keep track of the correspondence of message */

        std::string print_cred(credits);

        // Paramters
        int64_t wait_tx_buf; /*!< Minimum umber of ticks a flit has to wait in tx buffer before going onto the tx bus*/
    };

    /*!
    CXL host class
    */
    class CXLHost : public CXLNode
    {
    public:
        CXLHost();
        CXLHost(int vc_size, int buf_size, uint node_id);
        void update();
        void register_device(uint64_t device_id, const std::pair<uint64_t, uint64_t> &addr_interval);
        std::pair<uint64_t, uint64_t> DAM_addr;
        void register_DAM(ramulator::DirectAttached *dam, const std::pair<uint64_t, uint64_t> &addr_interval);
        void connect_tx(CXLBus *bus);
        void connect_rx(CXLBus *bus);
        std::map<int, credits> *get_cred();
        bool text_to_trace();
        void set_trace_file(std::string filename);
        std::map<uint64_t, message> messages_sent_to_device; /*!< Map with a list of all messages sent to the devices with msg_id as key*/
        std::map<uint64_t, msg_timing> timing_tracker;       /*!< To store the timing parameters of all the completed memory accesses*/
        std::vector<uint64_t> latency_data;
        std::vector<uint64_t> ram_latency_data;
        std::vector<msg_timing> latency_data_full;
        void dump_data();
        ramulator::DirectAttached *DAM; /*!<Pointer to direct attached memory*/
        bool skip_packer_check();
        bool skip_transmit_check();
        bool skip_unpacker_check();
        bool skip_req_complete_check();
        void print_skipped_cycles();
        void dump_latency();
        void print_latency_verbose(msg_timing &t);
        void print_direct_attached_latency(uint64_t msg_id); /*!< Function to print out DAM req latencies*/

        bool no_active_transaction(); /*!< Returns true if there are no active transactions in the system i.e., its okay to skip this cycle*/
        void register_memory(ramulator::RamDevice*); /*!<Prove host with pointer to ramulator instance within a cxl device. Useful for cycle skipping*/

        uint64_t empty_cycle_packer;
        uint64_t empty_cycle_unpacker;
        uint64_t empty_cycle_transmit;
        uint64_t empty_cycle_req_completed;

        uint64_t skippable_cycle;

        uint64_t total_credit_checks;
        uint64_t int_credit_reject_ctr;
        uint64_t ext_credit_reject_ctr;

        std::map<uint64_t, int64_t> reqs_in_dam;
        std::map<uint64_t, std::pair<uint64_t,double>> amat_per_table_dam;
        std::map<uint64_t, double[3]> amat_per_table_cxl;

    protected:
        std::map<uint64_t, std::pair<uint64_t, uint64_t>> address_intervals; /*!< Stores the address mapping for different devices*/
        int64_t latency_counter;                                             /*!< increment after every tick*/
        // bool check_send() override;
        bool check_pack() override { return false; }
        bool check_unpack() override { return false; }
        flit FLIT_2_send() override { return flit(); }
        bool CRC(flit f) override { return 0; }
        bool check_if_req_completed();
        bool check_rx_vc(CXLBuf<message> &vc);
        uint destination_device(uint64_t addr);
        void credit_sanity_check();

        // void FLIT_packer2();
        bool FLIT_packer();
        bool FLIT_unpacker();
        std::vector<uint64_t> connected_devices; /*!< connected_device id */
        credits get_Host_credits();
        bool transmit();
        // bool check_vc_for_packing(CXLBuf<message> &vc, std::vector<message>::iterator &it);
        bool check_vcs_for_packing(std::array<CXLBuf<message>, NUM_VC> &, CXLBuf<message> *&vc);
        // bool get_next_message_to_transmit(std::vector<message>::iterator &it, bool &nothing_more_to_send);
        void get_msg_from_vcs(CXLBuf<message> *&vc, bool &valid);
        void add_flit_to_buf(flit &f);

    private:
        std::vector<ramulator::RamDevice*> device_memories;
        std::array<CXLBuf<message>, NUM_VC> M2S_Req; /*!<M2S Req virtual channels, connect to rx*/
        int cur_Req_vc;
        std::array<CXLBuf<message>, NUM_VC> M2S_RWD; /*!<M2CXLDeviceS RWD virtual channels, connect to rx*/
        int cur_RWD_vc;
        CXLBuf<message> S2M_NDR; /*!<S2M NDR virtual channel, connect to tx*/
        CXLBuf<message> S2M_DRS; /*!<S2M_DRS virtual channel, connect to tx*/
        round_robin_state packer_state;
        int unpacker_rollover;
        int packer_rollover;
        int Req_packed;                                         // number of NDR packed
        bool packer_stall;                                      // stalled for unfilled flit
        int unfilled_offset;                                    // index start to fill in the next try
        flit tmp_unfilled_flit;                                 // tmp flit
        int64_t last_transmitted_at;                            // Tick at which last tx_buf->bus transaction happened
        bool trace_line_pending;                                // True if a line was read from trace file last tick but not put on the VCs yet
        ifstream trace_in;                                      // ifstream buffer pointed to the trace file
        round_robin_state rcvd_rsp_state;                       // Round robin state for processing received responses
        message last_drs_hdr;                                   // Termporary variable to store the most recent drs_hdr
        message last_rwd_hdr;                                   // Termporary variable to store the most recent rwd_hdr
        bool is_trace_finished;                                 // True means all requests in trace file have been converted into messages and put onto the VCs
        CXLBuf<std::pair<uint64_t, message>> text_to_trace_buf; /*!< Buffer to keep all newly formed messages before they are put on the virtual channels. It also keeps the numer of cpu instructions executed before this access*/

        // Packer2
        bool is_packer_waiting;
        int64_t started_packing_at;
        int64_t packer_wait_time;
        flit flit_to_pack;
        int req_ctr, rwd_ctr;
        // int num_read_ports;
        round_robin_state pkr2_state;
        uint device_under_consideration;       /*!< Node id of the device we are packing stuff to*/
        int64_t last_text_to_trace_buf_dequeue; /*!< Counter which gatekeeps when a message from text_to_trace_buf reaches VC. It keeps track of tick at which the most recent message moved from text_to_trace_buf to the VC*/
    };

    //! Struct to keep track of reqs that have been sent to ramulator and are yet to be completed
    typedef struct
    {
        message msg;
        int counter;
    } req_sent_to_ramulator;

    /*!
    CXL Device class
    */
    class CXLDevice : public CXLNode
    {
    private:
        CXLBuf<message> M2S_Req; /*!<S2M NDR virtual channel, connect to tx*/
        CXLBuf<message> M2S_RWD;
        std::array<CXLBuf<message>, NUM_VC> S2M_NDR; /*!<M2S Req virtual channels, connect to rx*/
        int cur_NDR_vc;
        std::array<CXLBuf<message>, NUM_VC> S2M_DRS; /*!<M2CXLDeviceS RWD virtual channels, connect to rx*/
        int cur_DRS_vc;

        ramulator::RamDevice dram;
        round_robin_state ramulator_inp_state;
        round_robin_state packer_state;
        int unpacker_rollover;
        int packer_rollover;
        std::vector<req_sent_to_ramulator> reqs_in_ramulator;
        int NDR_packed;                // number of NDR packed
        bool packer_stall;             // stalled for unfilled flit
        int unfilled_offset;           // index start to fill in the next try
        flit tmp_unfilled_flit;        // tmp flit
        int64_t last_ramulator_update; // Down counter to keep track of when ramulator update was last called
        message last_rwd_hdr;          // Termporary variable to store the most recent rwd_hdr
        message last_drs_hdr;          // Termporary variable to store the most recent drs_hdr
        int64_t last_transmitted_at;   // Tick at which last tx_buf->bus transaction happened
        int packer_seq_length;         /*!<Length of the current dependence sequence formed by the currently packed flits*/
        int seq_threshold;             /*!< Max threshold number of flits that can have a dependency sequence*/

        // Packer2
        bool is_packer_waiting;
        int64_t started_packing_at;
        int64_t packer_wait_time;
        flit flit_to_pack;
        int ndr_ctr, drs_ctr;
        // int num_read_ports;
        round_robin_state pkr2_state;

    public:
        bool check_connection();
        CXLDevice() = default;
        RamDevice *access_dram();
        CXLDevice(int vc_size, int buf_size, uint node_id);
        void update() override;
        // void register_host(uint64_t host_id); /*!< reserved for PBR */
        void ramulator_req_notification(Request &r, CXLDevice *dev);
        void device_init();
        std::map<uint64_t, message> messages_in_ramulator;
        bool transmit(); /*!< Put flit from tx buffer to tx bus*/
        bool skip_packer_check();
        bool skip_unpacker_check();
        bool skip_send_to_ram_check();
        bool skip_transmit_check();
        void print_skipped_cycles();

        uint64_t empty_cycle_packer;
        uint64_t empty_cycle_unpacker;
        uint64_t empty_cycle_sent_to_ram;
        uint64_t empty_cycle_transmit;
        uint64_t skippable_cycle;
        uint64_t num_reqs;

    protected:
        // bool check_send() override { return 0; };
        /*!
        should be similar to host but follow different rules
        */
        /*!
        check if we can send to dram
        */
        bool check_dev2ram();
        /*!
        check if we can receive from dram
        */
        bool check_ram2dev();
        /*!
        ram response to message
        */
        message ram2msg(ram_msg response) { return message(); };
        /*!
        message to ram response
        */
        ram_msg msg2ram(message request) { return ram_msg(); };
        bool FLIT_packer();
        /*!
        could be exactly the same, need to check this
        */
        int search_reqs_in_ramulator(Request &r);
        bool FLIT_unpacker();
        bool send_to_ramulator();
        flit FLIT_2_send() override { return flit(); }
        bool CRC(flit f) override { return 0; }
        bool check_unpack() override { return 0; }
        bool check_pack() override { return 0; }
        CXLBuf<ram_msg> device_ram_buf; /*!<buffer from device 2 ram */
        CXLBuf<ram_msg> ram_device_buf; /*!<buffer from ram 2 device */
        std::vector<uint64_t> connected_hosts;
        bool check_rx_vc(CXLBuf<message> &);
        type_packed free_allocation(slot &s);
        credits get_Device_credits();
        bool check_vcs_for_packing(std::array<CXLBuf<message>, NUM_VC> &vcs, CXLBuf<message> *&vc);
        void get_msg_from_vcs(CXLBuf<message> *&vc, bool &valid);
        void add_flit_to_buf(flit &f);
        void credit_sanity_check();
        credits get_device_credits();
        uint destination_device(uint64_t addr);
    };
}

#endif