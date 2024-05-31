#include "utils.h"
#include <iostream>
// #include <string>

using namespace CXL;

namespace CXL
{
    extern int64_t curr_tick;
}

void CXL::CXLAssert(bool pred, std::string s, const char *parent_func)
{
    if (pred)
        return;
    std::cout << "Assertion Failed: " << s << " @" << curr_tick << " " << parent_func << "\n";
    exit(3);
}

//! Constructor for the CXLLog class
CXLLog::CXLLog(std::string filename)
{
    // Open file
    eventlog.open(filename);
}

//! Function to print out to an event log file
void CXLLog::CXLEventLog(std::string s, uint origin, uint dest)
{
    // This will be called when events occur. In our case events are whenever something is enqueued or dequeued into a VC or the bus
    // We will print out the current clock tick and the node_id as well
    if (dest == -1)
        dest = origin;
    eventlog << "@" << curr_tick << ", ORG: " << origin << ", DST: " << dest << ", Event: " << s;
    eventlog.flush();
}

// void CXL::CXLAssert(bool pred, const char* parent_func)
// {
//     if (pred)
//         return;
//     std::cout << "Assertion Failed!"
//               << " @" << curr_tick << " " << parent_func << "\n";
//     exit(3);
// }

CXL_IF::ramulator_req CXL::message2ramulator_req(message m)
{
    std::vector<CXL_IF::ramulator_req> reqs;
    // Make sure only reads or writes are there
    CXL_ASSERT((m.opCode == opcode::Req || m.opCode == opcode::RwD) && "Illegal message type");
    // Making the assumption that ramulator operate at 64B granularity, same as CXL

    CXL_IF::ramulator_req temp;
    temp.req_addr = m.address; // TODO check if we need to align this to 64B boundary
    temp.req_id = m.msg_id;
    temp.req_type = m.opCode == opcode::Req ? ramulator::Request::Type::READ : ramulator::Request::Type::WRITE;
    temp.type = CXL_IF::status_type::VALID;
    return temp;
}

void CXL::customFailureHandler()
{
    std::cout<<"@ "<<curr_tick<<"\n";
}