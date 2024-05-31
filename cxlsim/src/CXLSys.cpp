#include "CXLSys.h"

#ifndef HOST_VC_SIZE
    #define HOST_VC_SIZE 1024
#endif
#ifndef HOST_BUF_SZIE
    #define HOST_BUF_SIZE 2048
#endif
#ifndef DEV_VC_SIZE
    #define DEV_VC_SIZE 1024
#endif
#ifndef DEV_BUF_SIZE
    #define DEV_BUF_SIZE 2048
#endif

using namespace CXL;

CXLSystem::~CXLSystem()
{
    for (ramulator::DirectAttached *dam : DAMs)
    {
        delete dam;
    }
    DAMs.clear();
}

CXLSystem::CXLSystem(uint64_t num_host, uint64_t num_device, bool has_DAM, const std::vector<std::pair<uint64_t, uint64_t>> &address_intervals_DAM, const std::vector<std::pair<uint64_t, uint64_t>> &address_intervals, const std::vector<uint64_t> &device_host)
{
    uint64_t node_id = 0;
    if (has_DAM)
    {
        DAMs.reserve(num_host);
        last_DAM_update.reserve(num_host);
    }
    hosts.reserve(num_host);
    devices.reserve(num_device);
    interconnects.resize(num_device + num_host); // first is top down, second is bottom up
    switch_ = CXLSwitch(4096, node_id++);
    CXLAssert(address_intervals.size() == device_host.size() && address_intervals.size() == num_device, "insufficient information");
    for (int i = 0; i < num_host; i++)
    {
        hosts.emplace_back(HOST_VC_SIZE, HOST_BUF_SIZE, node_id++);
    }
    for (int i = 0; i < num_device; i++)
    {
        devices.emplace_back(DEV_VC_SIZE, DEV_BUF_SIZE, node_id++);
    }
    for (int i = 0; i < device_host.size(); ++i)
    {
        hosts[device_host[i]].register_device(i, address_intervals[i]);
    }
    //Provide the host with pointers to the ramulator instanes of the devices
    for (CXLDevice& device: devices)
    {
        hosts[0].register_memory(device.access_dram());
    }
    

    for (int i = 0; i < num_device + num_host; ++i)
    {
        if (i < num_host)
        {
            interconnects[i] = {CXLBus(15, node_id++, UpStream), CXLBus(15, node_id++, DwStream)};
            switch_.connect_upstream(&interconnects[i].first, &interconnects[i].second); // to host; from host
            interconnects[i].second.connect_from(&hosts[i].tx_buffer);
            interconnects[i].second.connect_to(&switch_.upstream_buffer_rx); // NOTE: switch has only one host
            interconnects[i].second.bus_type = bus_terminals::host_switch;
            interconnects[i].second.set_from_to_node_id(hosts[i].node_id, switch_.node_id);
            interconnects[i].first.connect_from(&switch_.upstream_buffer_tx); // NOTE: switch has only one host
            interconnects[i].first.connect_to(&hosts[i].rx_buffer);
            interconnects[i].first.bus_type = bus_terminals::switch_host;
            interconnects[i].first.set_from_to_node_id(switch_.node_id, hosts[i].node_id);
            hosts[i].connect_tx(&interconnects[i].second);
            hosts[i].connect_rx(&interconnects[i].first);
            hosts[i].check_connection();
        }
        else
        {
            interconnects[i] = {CXLBus(15, node_id++, DwStream), CXLBus(15, node_id++, UpStream)};
            cout << interconnects[i].first.node_id << " " << interconnects[i].second.node_id << " ";
            switch_.connect_downstream(&interconnects[i].first, &interconnects[i].second, i - num_host, address_intervals[i - num_host]); // to device; from device
            interconnects[i].first.connect_from(&switch_.downstream_buffers_tx[i - num_host]);
            interconnects[i].first.connect_to(&devices[i - num_host].rx_buffer); // NOTE: switch has only one host
            interconnects[i].first.bus_type = bus_terminals::switch_device;
            interconnects[i].first.set_from_to_node_id(switch_.node_id, devices[i - num_host].node_id);
            interconnects[i].second.connect_from(&devices[i - num_host].tx_buffer); // NOTE: switch has only one host
            interconnects[i].second.connect_to(&switch_.downstream_buffers_rx[i - num_host]);
            interconnects[i].second.bus_type = bus_terminals::device_switch;
            interconnects[i].second.set_from_to_node_id(devices[i - num_host].node_id, switch_.node_id);
            devices[i - num_host].connect_tx(&interconnects[i].second);
            devices[i - num_host].connect_rx(&interconnects[i].first);
            devices[i - num_host].check_connection();
        }
    }
    if (has_DAM)
    {

        for (int i = 0; i < num_host; i++)
        {
            CXLAssert(address_intervals_DAM.size() == num_host, "insufficient information");
            this->DAMs.emplace_back(new ramulator::DirectAttached(node_id++));
            hosts[i].register_DAM(DAMs[i], address_intervals_DAM[i]);
            DAMs[i]->set_host(&hosts[i]);
            last_DAM_update.push_back(0);
        }
    }
}

void CXLSystem::update()
{
    for (int i = 0; i < hosts.size(); ++i)
    {
        interconnects[i].first.update();
        interconnects[i].second.update();
    }
    switch_.update();
    for (int i = hosts.size(); i < interconnects.size(); ++i)
    {
        interconnects[i].first.update();
        interconnects[i].second.update();
    }
    for (auto &i : devices)
    {
        i.update();
    }
    for (auto &i : hosts)
    {
        i.update();
    }
    if(curr_tick%params.delay_ramulator_update==0)
    {
        for (int i = 0; i < DAMs.size(); i++)
        {
            DAMs[i]->update();
        }
    }
}
