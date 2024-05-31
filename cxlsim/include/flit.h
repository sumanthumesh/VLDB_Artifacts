#ifndef __CXL_FLIT_H
#define __CXL_FLIT_H

#include "message.h"
#include <vector>
#include <cstdint>

namespace CXL
{

	typedef struct
	{
		int req_credit;
		int rsp_credit;
		int data_credit;
	} credits;

	enum slot_type
	{
		m2s_req = 0,
		m2s_rwd_hdr = 1,
		s2m_ndr = 2,
		s2m_drs_hdr = 3,
		data = 4,
		empty = 5
	};
	//!  Description of slot in terms of messages
	/*!
	  Basically a vector of messages
	*/
	class slot
	{
	public:
		slot_type type;			   /*!< Type of slot, can be a data slot or one that contains messages. Need to check if other types are possible*/
		std::vector<message> msgs; /*!< Slot can contain more than one message in cxl.cache. cxl.mem uses one message per slot*/
		message msg;			   /*!< To hold msgs[0] in case we are using only cxl.mem*/
		slot(slot_type type, message msg);
		slot(message &msg);
		slot();
	};

	typedef struct
	{
		int64_t time_of_creation;
		int64_t time_of_transmission;
		int64_t time_of_receipt;
	} flit_lifetime;

	enum flit_type
	{
		control,
		protocol
	};

	//!  Description of flit in terms of slots
	/*!
	  Basically a vector of slots
	*/
	class flit_header
	{
	public:
		flit_type type;	  /*!< Type of flit, either control or protocol*/
		bool ack;		  /*!< Acknowledgment of 8 successful flit transfers. For RETRY and CONTROL flits*/
		bool byte_enable; /*!< Need to check what this does. Used in control flits*/
		bool size;		  /*!< Need to check what this does. Used in control flits*/

		// Credits
		credits credit;
		flit_header();

		std::vector<slot_type> slots; /*!< Header contains a record of the types of slots in the flit*/
	};

	enum flit_size
	{
		B68,
		B256,
		PBR
	};
	//!  Description of flit in terms of slots
	/*!
	  Basically a vector of slots
	*/
	class flit
	{
	public:
		bool valid;
		flit_lifetime time;		 /*!< Various times related to lifetime of flit*/
		flit_size size;			 /*!< Type of flit based on size, can be 68B, 256B or PBR flit*/
		flit_header header;		 /*!< Header as per defintiion in spec. Contains metadata on the flit*/
		std::vector<slot> slots; /*!< Vector of regular slots. A flit is composed of multiple slots depending on its type*/
		flit();
		void copy(const flit &);
		void print();
		std::string sprint();
		uint64_t flit_id; /*!< Unique id given to each flit*/
		static uint64_t flit_counter;

		bool is_flit_empty();
		bool is_flit_full();
		int slot_count_in_flit(slot_type stype);
		bool are_all_slots_after_this_empty(int i);
		void set_time(int64_t msg_timing::*member, int64_t value);
		uint64_t get_first_address() const;
	};
}

#endif