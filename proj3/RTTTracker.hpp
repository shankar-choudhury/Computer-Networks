/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: RTTTracker.hpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code defines the data structures and methods used to compute
 *  round-trip times (RTTs) for TCP flows. It tracks packets in both
 *  directions of a TCP conversation using their sequence and acknowledgment
 *  numbers to determine when a response has acknowledged the first data
 *  segment. The class stores these RTT values and later prints them in
 *  the format required by the RTT mode of the router simulator.
 */

#ifndef RTTTRACKER_HPP
#define RTTTRACKER_HPP

#include "Packet.hpp"
#include <unordered_map>
#include <vector>

struct RTTKey
{
    uint32_t sip;
    uint16_t sport;
    uint32_t dip;
    uint16_t dport;

    bool operator==(const RTTKey &other) const noexcept
    {
        return sip == other.sip && sport == other.sport &&
               dip == other.dip && dport == other.dport;
    }
};

struct RTTKeyHasher
{
    size_t operator()(const RTTKey &key) const noexcept
    {
        size_t h1 = std::hash<uint32_t>{}(key.sip);
        size_t h2 = std::hash<uint16_t>{}(key.sport);
        size_t h3 = std::hash<uint32_t>{}(key.dip);
        size_t h4 = std::hash<uint16_t>{}(key.dport);
        return (((h1 ^ (h2 << 1)) ^ (h3 << 1)) ^ (h4 << 1));
    }
};

struct RTTFlow
{
    bool has_data = false;
    bool has_rtt = false;
    double send_ts = 0.0;
    uint32_t seq_start = 0;
    double rtt = 0.0;
};

class RTTTracker
{
public:
    void addPacket(const Packet &p);
    void printFlows() const;

private:
    std::unordered_map<RTTKey, RTTFlow, RTTKeyHasher> flow_map_;
    std::vector<RTTKey> flow_order_;
};

#endif
