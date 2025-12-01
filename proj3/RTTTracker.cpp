/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: RTTTracker.cpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code implements the RTTTracker class, which analyzes TCP packet
 *  flows to identify the first observed round-trip time (RTT) in each
 *  direction. It processes each packet, detects when a data segment is
 *  acknowledged by the reverse flow, and calculates the time difference
 *  between transmission and acknowledgment. The resulting RTT statistics
 *  are printed for all TCP flows found in the packet trace.
 */

#include "RTTTracker.hpp"
#include <iostream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <vector>
#include <tuple>
#include <arpa/inet.h>

void RTTTracker::addPacket(const Packet &p)
{
    if (!p.isValidForPrint())
        return;
    if (p.getProtoChar() != 'T')
        return;

    RTTKey key{p.getSip(), p.getSport(), p.getDip(), p.getDport()};
    auto it = flow_map_.find(key);
    if (it == flow_map_.end())
    {
        flow_map_[key] = RTTFlow();
        flow_order_.push_back(key);
    }

    RTTFlow &flow = flow_map_[key];
    double ts = p.getTimestamp();

    if (!flow.has_data && p.getPaylen() > 0)
    {
        flow.has_data = true;
        flow.send_ts = ts;
        flow.seq_start = p.getSeqno();
    }

    RTTKey rev_key{p.getDip(), p.getDport(), p.getSip(), p.getSport()};
    auto rev_it = flow_map_.find(rev_key);
    if (rev_it != flow_map_.end())
    {
        RTTFlow &rev_flow = rev_it->second;
        if (rev_flow.has_data && !rev_flow.has_rtt && p.getAckno() > rev_flow.seq_start)
        {
            rev_flow.has_rtt = true;
            rev_flow.rtt = ts - rev_flow.send_ts;
        }
    }
}

void RTTTracker::printFlows() const
{
    for (const auto &key : flow_order_)
    {
        const RTTFlow &flow = flow_map_.at(key);

        struct in_addr sa{htonl(key.sip)};
        struct in_addr da{htonl(key.dip)};
        char ssrc[INET_ADDRSTRLEN];
        char sdst[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sa, ssrc, sizeof(ssrc));
        inet_ntop(AF_INET, &da, sdst, sizeof(sdst));

        std::cout << ssrc << " " << key.sport << " "
                  << sdst << " " << key.dport << " ";

        if (flow.has_rtt)
            std::cout << std::fixed << std::setprecision(6) << flow.rtt;
        else
            std::cout << "-";

        std::cout << "\n";
    }
}
