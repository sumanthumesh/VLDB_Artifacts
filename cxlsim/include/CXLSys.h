#ifndef __CXL_SYSTEM_H
#define __CXL_SYSTEM_H

#include "CXLNode.h"
#include "CXLSwitch.h"

namespace CXL
{
    class CXLSystem
    {
        public:
            CXLSystem(){};
            ~CXLSystem();
            CXLSystem(uint64_t num_host, uint64_t num_device, bool has_DAM, const std::vector<std::pair<uint64_t, uint64_t>>& address_interval_DAM, const std::vector<std::pair<uint64_t, uint64_t>>& address_intervals, const std::vector<uint64_t>& device_host);
            void update();
            std::vector<ramulator::DirectAttached*> DAMs;
            std::vector<CXLDevice> devices;
            std::vector<CXLHost> hosts;
            std::vector<std::pair<CXLBus, CXLBus>> interconnects;
            std::vector<int> last_DAM_update;
            CXLSwitch switch_;

    };
}
#endif