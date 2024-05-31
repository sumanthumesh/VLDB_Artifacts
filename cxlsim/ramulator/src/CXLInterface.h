#ifndef __CXLINTERFACE_H
#define __CXLINTERFACE_H

#include "Request.h"
#include <queue>
#include <cstdint>

using namespace ramulator;

namespace CXL_IF
{
    enum status_type {VALID, EMPTY, END};
    typedef struct 
    {
        uint64_t req_id;        
        status_type type;
        long req_addr;
        Request::Type req_type;
    }ramulator_req;
    ramulator_req build_req(long req_addr, Request::Type req_type, status_type stat_type);
    ramulator_req build_req(uint64_t req_id, long req_addr, Request::Type req_type, status_type stat_type);

    //Function to be called once request has finished exeuction
    void ramulator_req_complete(Request& r);
    

    class CXL_if_buf
    {
    private:
    public:
        std::queue<ramulator_req> buf;
        int size_;
        CXL_if_buf();
        bool isEmpty();
        bool isFull();
        status_type get_dramtrace_request(long& req_addr, Request::Type& req_type, uint64_t &req_id);
        ~CXL_if_buf();
        void buf_add(ramulator_req r);
    };
}

#endif