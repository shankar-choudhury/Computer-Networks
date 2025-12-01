/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: FlowTracker.hpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code defines data structures and methods for aggregating packets
 *  into logical network flows using the five-tuple: source IP, source port,
 *  destination IP, destination port, and protocol. The FlowTracker class
 *  records per-flow statistics including start time, duration, total packets,
 *  and total payload bytes, supporting the NetFlow summary mode of the router
 *  simulator.
 */

#ifndef FLOWTRACKER_HPP
#define FLOWTRACKER_HPP

#include "Packet.hpp"
#include <map>
#include <string>

struct FlowKey
{
    uint32_t sip;
    uint16_t sport;
    uint32_t dip;
    uint16_t dport;
    char proto;

    bool operator<(const FlowKey &other) const
    {
        return std::tie(sip, sport, dip, dport, proto) <
               std::tie(other.sip, other.sport, other.dip, other.dport, other.proto);
    }
};

struct FlowInfo
{
    double first_ts;
    double last_ts;
    uint64_t total_pkts;
    uint64_t total_payload;
};

class FlowTracker
{
public:
    void addPacket(const Packet &p);
    void printFlows() const;

private:
    std::map<FlowKey, FlowInfo> flows_;
};

#endif
