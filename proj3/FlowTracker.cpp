/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: FlowTracker.cpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code implements the FlowTracker class used to generate NetFlow-style
 *  summaries from a packet trace. Each flow is identified by its five-tuple,
 *  and the tracker accumulates total packet count, payload size, and duration.
 *  Once all packets are processed, the aggregated flow data are printed in
 *  the format required for NetFlow mode output.
 */

#include "FlowTracker.hpp"
#include <iostream>
#include <iomanip>
#include <arpa/inet.h>

void FlowTracker::addPacket(const Packet &p)
{
    if (!p.isValidForPrint())
        return;

    FlowKey key{p.getSip(), p.getSport(), p.getDip(), p.getDport(), p.getProtoChar()};
    double ts = p.getTimestamp();

    auto it = flows_.find(key);
    if (it == flows_.end())
    {
        flows_[key] = {ts, ts, 1, static_cast<uint64_t>(p.getPaylen())};
    }
    else
    {
        FlowInfo &info = it->second;
        info.last_ts = ts;
        info.total_pkts++;
        info.total_payload += p.getPaylen();
    }
}

void FlowTracker::printFlows() const
{
    for (const auto &[key, info] : flows_)
    {
        struct in_addr sa{htonl(key.sip)};
        struct in_addr da{htonl(key.dip)};
        char ssrc[INET_ADDRSTRLEN];
        char sdst[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa, ssrc, sizeof(ssrc));
        inet_ntop(AF_INET, &da, sdst, sizeof(sdst));

        double duration = info.last_ts - info.first_ts;

        std::cout << ssrc << " " << key.sport << " "
                  << sdst << " " << key.dport << " "
                  << key.proto << " "
                  << std::fixed << std::setprecision(6)
                  << info.first_ts << " "
                  << duration << " "
                  << info.total_pkts << " "
                  << info.total_payload << "\n";
    }
}
