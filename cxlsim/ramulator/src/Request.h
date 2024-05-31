#ifndef __REQUEST_H
#define __REQUEST_H

#include <vector>
#include <functional>
#include <cstdint>
#include <string>
// #include "CXLNode.h"

using namespace std;

namespace CXL
{
    class CXLDevice;
    class CXLHost;
}

namespace ramulator
{

    struct time
    {
        int64_t start, end;         // Time in ticks
        int64_t ram_start, ram_end; // Time in ramulator cycles
    };

    class Request
    {
    public:
        bool is_first_command;
        long addr;
        // long addr_row;
        vector<int> addr_vec;
        // specify which core this request sent from, for virtual address translation
        int coreid;
        // Unique request id
        uint64_t req_id;
        // Used only for direct attached RAM, CXL memory has its own way of timekeeping
        time dam_time;
        // Requesting device
        CXL::CXLDevice *req_device;
        // Requesting host
        CXL::CXLHost *req_host;

        enum class Type
        {
            READ,
            WRITE,
            REFRESH,
            POWERDOWN,
            SELFREFRESH,
            EXTENSION,
            MAX
        } type;

        long arrive = -1;
        long depart = -1;
        function<void(Request &)> callback; // call back with more info

        Request(long addr, Type type, int coreid = 0)
            : is_first_command(true), addr(addr), coreid(coreid), type(type),
              callback([](Request &req) {}) {}

        Request(long addr, Type type, function<void(Request &)> callback, int coreid = 0)
            : is_first_command(true), addr(addr), coreid(coreid), type(type), callback(callback) {}

        Request(long addr, Type type, uint64_t req_id, function<void(Request &)> callback, int coreid = 0)
            : is_first_command(true), addr(addr), coreid(coreid), type(type), req_id(req_id), callback(callback) {}

        Request(long addr, Type type, uint64_t req_id, function<void(Request &)> callback, CXL::CXLDevice *cxld, int coreid = 0)
            : is_first_command(true), addr(addr), coreid(coreid), type(type), req_id(req_id), callback(callback), req_device(cxld) {}

        Request(long addr, Type type, uint64_t req_id, function<void(Request &)> callback, CXL::CXLHost *cxlh, int coreid = 0)
            : is_first_command(true), addr(addr), coreid(coreid), type(type), req_id(req_id), callback(callback), req_host(cxlh) {}

        Request(vector<int> &addr_vec, Type type, function<void(Request &)> callback, int coreid = 0)
            : is_first_command(true), addr_vec(addr_vec), coreid(coreid), type(type), callback(callback) {}

        Request()
            : is_first_command(true), coreid(0) {}
        std::string sprint()
        {
            char s[100];
            sprintf(s, "#%d A: %08x T:%d", req_id, addr, type);
            return std::string(s);
        }
    };

} /*namespace ramulator*/

#endif /*__REQUEST_H*/
