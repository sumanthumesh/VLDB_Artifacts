#ifndef __CXL_MESSAGE_H
#define __CXL_MESSAGE_H

#include <cstdint>
#include <string>

namespace CXL
{
	//!  Generic description of message
	/*!
	  Common message class including all fields for Req, RwD, NDR and DRS messages.
	*/

	typedef struct
	{
		int64_t tick_sent_to_ramulator;
		int64_t tick_rcvd_from_ramulator;
		uint64_t ramulator_clk_start;
		uint64_t ramulator_clk_end;
	} ramulator_timing;

	typedef struct
	{
		bool read_write;				 /*!< false means read true means write*/
		int64_t tick_created;			 /*!< When the message was created on the host*/
		int64_t tick_packed;			 /*!< When the message was packed on the host*/
		int64_t tick_transmitted;		 /*!< When the message was put on the bus by the host*/
		int64_t tick_switch_ds_rx;       /*!< When the flit was received at the rx of switch from host*/
		int64_t tick_switch_ds_tx;       /*!< When the flit was transmitted from the switch to device*/
		int64_t tick_received;			 /*!< When the message entered the device's rx buffer*/
		int64_t tick_unpacked;			 /*!< When the message was unpacked by the device*/
		int64_t tick_at_ramulator;		 /*!< When the message was received by ramulator*/
		int64_t tick_ramulator_complete; /*!< When ramulator finished processing the message*/
		int64_t tick_repacked;			 /*!< When the response from ramulator was packed by the device*/
		int64_t tick_retransmitted;		 /*!< When the message was put on the bus by the device*/
		int64_t tick_switch_us_rx;       /*!< When the flit was received at the rx of switch from device*/
		int64_t tick_switch_us_tx;       /*!< When the flit was transmitted from the switch to host*/
		int64_t tick_resp_received;		 /*!< When the host received the message back*/
		int64_t tick_resp_unpacked;		 /*!< When the host unpacked the resp*/
		int64_t tick_req_complete;		 /*!< When the host verified that the req is not complete*/
		int64_t ramulator_clk_start;	 /*!< When ramulator received the request in ramulator clk count*/
		int64_t ramulator_clk_end;		 /*!< When ramulator completed the request in ramulator clk count*/
	} msg_timing;

	enum opcode
	{
		Req,
		RwD,
		NDR,
		DRS
	};

	class message
	{
	public:
		static uint64_t msg_count;
		bool valid;		   /*!< Is the request valid*/
		opcode opCode;	   /*!< Opcode for particular transaction, can be made into enum later*/
		int meta_field;	   /*!< Not sure what this is supposed to be*/
		int meta_value;	   /*!< Not sure what this is supposed to be*/
		int snoop_type;	   /*!< Not used in cxl.mem, keeping here for later upgrade to cxl.mem*/
		int tag;		   /*!< Looks similar in use to the tag in 470 memory subsystem, need to confirm*/
		uint64_t address;  /*!< Address to access 46+5 bits for 68B flit, 46 for 256B and PBR flits*/
		int ld_id;		   /*!< Used to address LD-ID inside MLD. Not applicable to PBR flits*/
		int sp_id;		   /*!< Source PBR ID, only applicable to PBR flits*/
		int dp_id;		   /*!< Destination PBR ID, only applicable to PBR flits*/
		int traffic_class; /*!< Quality of service requirements specified by the master*/
		// int64_t tick_created;  /*!< the tick when it was created*/
		// int64_t tick_unpacked; /*!< Tick at which message is unpacked from flit and added into the VCs*/

		// Applicable only to RwD requests
		bool poison;			  /*!< Only in RwD requests*/
		bool byte_enable_present; /*!< Indicates presence of byte-enables used in 68B flits*/

		// Applicable only to Device responses
		int dev_load; /*!< Device load communication for enforcing QoS*/

		uint64_t msg_id; /*!< Unique id for every message*/

		msg_timing time;
		// ramulator_timing dram_time; /*!< Keeps track of time taken by ramulator to service this particular message if it is Req or RwD*/

		// Contrusctor
		message(){};
		void print();
		std::string sprint() const;
		message(opcode oc, uint64_t addr);
		message(opcode oc, uint64_t addr, int dp_id, int sp_id);
		// message(opcode oc, uint64_t addr, int dp_id, int sp_id, int64_t time);
		void copy(const message &m);
		// message(message &m2);

	private:
		void init_time();
	};

	class ram_msg
	{
		// dummy
	public:
		void print(){};
	};
}

#endif