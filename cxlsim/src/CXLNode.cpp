#include "CXLNode.h"
#include "message.h"
#include "flit.h"
#include <vector>
#include <cstdint>
#include "utils.h"

using namespace CXL;

CXLNode::CXLNode(int vc_size, int buf_size) : tx_buffer(buf_size), rx_buffer(buf_size), packer_cur_vc(0) {}

void CXLNode::connect_tx(CXLBus *bus)
{
    CXL_ASSERT(bus != NULL && "Bus not provided");
    tx_bus = bus;
}

void CXLNode::connect_rx(CXLBus *bus)
{
    CXL_ASSERT(bus != NULL && "Bus not provided");
    rx_bus = bus;
}

void CXLNode::add_to_rxbuf(flit f)
{
    rx_buffer.enqueue(f);
}

bool CXLNode::check_connection()
{
    if(rx_bus==NULL || tx_bus==NULL)
        return false;
    return true;
}

std::string CXLNode::print_cred(credits c)
{
    std::string s;
    s += "(" + std::to_string(c.req_credit) + " " + std::to_string(c.rsp_credit) + " " + std::to_string(c.data_credit) + ")";
    return s;
}
