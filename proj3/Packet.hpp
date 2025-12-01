/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: Packet.hpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code declares the Packet class, which encapsulates all relevant
 *  metadata extracted from a single packet in the binary trace file. It
 *  handles reading Ethernet, IPv4, TCP, and UDP headers, storing key
 *  attributes such as timestamps, source and destination addresses, port
 *  numbers, protocol type, header lengths, and payload size. The class also
 *  provides printing and validation methods used by the program’s different
 *  operational modes.
 */

#ifndef PACKET_HPP
#define PACKET_HPP

#include <cstdint>
#include <string>
#include <fstream>

class Packet
{
public:
    Packet();

    bool readFromStream(std::ifstream &file);
    bool isValidForPrint() const;
    void printPacket() const;
    double getTimestamp() const { return timestamp_; }
    uint32_t getSip() const { return sip_; }
    uint16_t getSport() const { return sport_; }
    uint32_t getDip() const { return dip_; }
    uint16_t getDport() const { return dport_; }
    char getProtoChar() const { return proto_char_; }
    uint16_t getPaylen() const { return paylen_; }
    uint32_t getSeqno() const { return seqno_; }
    uint32_t getAckno() const { return ackno_; }

private:
    bool valid_;
    double timestamp_;

    uint16_t ethertype_;

    bool is_ipv4_;
    uint32_t sip_;
    uint32_t dip_;
    uint16_t ip_len_;
    uint8_t ip_header_len_;
    uint8_t ip_proto_;

    uint16_t sport_;
    uint16_t dport_;
    char proto_char_;
    uint16_t thlen_;
    int32_t paylen_;

    uint32_t seqno_;
    uint32_t ackno_;
    bool is_ack_;

    static bool readFully(std::ifstream &file, char *buf, std::streamsize n);
};

#endif