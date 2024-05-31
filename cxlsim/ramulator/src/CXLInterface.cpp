#include "CXLInterface.h"
#include "CXLNode.h"

namespace CXL_IF
{
    void ramulator_req_complete(Request &r)
    {
        printf("Req with ID %lu completed execution\n", r.req_id);
        // ramulator_req_notification(r.req_id, Stats::curTick);
    }

    bool CXL_if_buf::isEmpty()
    {
        if (buf.size() == 0)
            return true;
        return false;
    }

    bool CXL_if_buf::isFull()
    {
        if (buf.size() >= size_)
            return true;
        return false;
    }

    status_type CXL_if_buf::get_dramtrace_request(long &req_addr, Request::Type &req_type, uint64_t &req_id)
    {
        if (isEmpty())
        {
            req_addr = NULL;
            req_type = Request::Type::REFRESH;
            return EMPTY;
        }
        else if (buf.front().type == END)
        {
            return END;
        }
        else
        {
            req_addr = buf.front().req_addr;
            req_type = buf.front().req_type;
            req_id = buf.front().req_id;
            buf.pop();
            return VALID;
        }
    }

    ramulator_req build_req(long req_addr, Request::Type req_type, status_type stat_type)
    {
        ramulator_req r = {0, stat_type, req_addr, req_type};
        return r;
    }

    ramulator_req build_req(uint64_t req_id, long req_addr, Request::Type req_type, status_type stat_type)
    {
        ramulator_req r = {req_id, stat_type, req_addr, req_type};
        return r;
    }

    void CXL_if_buf::buf_add(ramulator_req r)
    {
        CXL_ASSERT(!isFull() && "Overflow CXLDevice->Ramulator buffer");
        buf.push(r);
    }

    CXL_if_buf::CXL_if_buf() {size_ = 32;}

    CXL_if_buf::~CXL_if_buf() {}
}