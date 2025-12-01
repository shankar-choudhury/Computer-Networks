/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: Packet.cpp
 * Date created: 2025-10-28
 * Brief description:
 *  This code implements the Packet class, responsible for parsing raw packet
 *  data from the binary trace file. It reconstructs the timestamp from the
 *  capture headers, interprets Ethernet and IPv4 structures, and extracts
 *  transport-layer details for TCP and UDP packets, including sequence and
 *  acknowledgment numbers. The class also determines payload size and provides
 *  formatted printing for packet inspection mode (-p).
 */

#include "Packet.hpp"

#include <iostream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

using namespace std;

Packet::Packet()
    : valid_(false), timestamp_(0.0), ethertype_(0), is_ipv4_(false), sip_(0), dip_(0),
      ip_len_(0), ip_header_len_(0), ip_proto_(0), sport_(0), dport_(0), proto_char_('\0'),
      thlen_(0), paylen_(0), seqno_(0), ackno_(0), is_ack_(false)
{
}

bool Packet::readFully(ifstream &file, char *buf, streamsize n)
{
    if (!file.read(buf, n))
        return false;
    return true;
}

bool Packet::readFromStream(ifstream &file)
{
    valid_ = false;

    uint32_t sec_n, usec_n;
    if (!readFully(file, reinterpret_cast<char *>(&sec_n), 4))
        return false;
    if (!readFully(file, reinterpret_cast<char *>(&usec_n), 4))
        return false;

    uint32_t sec = ntohl(sec_n);
    uint32_t usec = ntohl(usec_n);
    timestamp_ = static_cast<double>(sec) + static_cast<double>(usec) / 1'000'000.0;

    struct ether_header eth;
    if (!readFully(file, reinterpret_cast<char *>(&eth), sizeof(eth)))
        return false;

    ethertype_ = ntohs(eth.ether_type);
    if (ethertype_ != ETHERTYPE_IP)
        return true;

    struct iphdr iph;
    if (!readFully(file, reinterpret_cast<char *>(&iph), sizeof(iph)))
        return false;

    ip_proto_ = iph.protocol;
    ip_len_ = ntohs(iph.tot_len);
    ip_header_len_ = iph.ihl * 4;
    sip_ = ntohl(iph.saddr);
    dip_ = ntohl(iph.daddr);
    is_ipv4_ = true;

    if (!(ip_proto_ == IPPROTO_TCP || ip_proto_ == IPPROTO_UDP))
        return true;

    if (ip_proto_ == IPPROTO_UDP)
    {
        struct udphdr udph;
        if (!readFully(file, reinterpret_cast<char *>(&udph), sizeof(udph)))
            return false;

        sport_ = ntohs(udph.source);
        dport_ = ntohs(udph.dest);
        proto_char_ = 'U';
        thlen_ = sizeof(udph);
        int plen = static_cast<int>(ip_len_) - static_cast<int>(ip_header_len_) - static_cast<int>(thlen_);
        paylen_ = (plen >= 0) ? plen : 0;
        seqno_ = 0;
        ackno_ = 0;
        is_ack_ = false;
    }
    else if (ip_proto_ == IPPROTO_TCP)
    {
        struct tcphdr tcph;
        if (!readFully(file, reinterpret_cast<char *>(&tcph), sizeof(tcph)))
            return false;

        uint8_t doff = tcph.th_off ? tcph.th_off : 5;
        thlen_ = doff * 4;
        if (thlen_ > 20)
        {
            uint16_t optbytes = thlen_ - 20;
            vector<char> discard(optbytes);
            if (!readFully(file, discard.data(), optbytes))
                return false;
        }

        sport_ = ntohs(tcph.th_sport);
        dport_ = ntohs(tcph.th_dport);
        proto_char_ = 'T';
        seqno_ = ntohl(tcph.th_seq);
        ackno_ = ntohl(tcph.th_ack);
        is_ack_ = (tcph.th_flags & TH_ACK) != 0;
        int plen = static_cast<int>(ip_len_) - static_cast<int>(ip_header_len_) - static_cast<int>(thlen_);
        paylen_ = (plen >= 0) ? plen : 0;
    }

    valid_ = true;
    return true;
}

bool Packet::isValidForPrint() const
{
    if (!valid_)
        return false;
    if (!is_ipv4_)
        return false;
    if (!(ip_proto_ == IPPROTO_TCP || ip_proto_ == IPPROTO_UDP))
        return false;
    return true;
}

void Packet::printPacket() const
{
    cout << fixed << setprecision(6) << timestamp_ << " ";

    struct in_addr sa{};
    sa.s_addr = htonl(sip_);
    char ssrc[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sa, ssrc, sizeof(ssrc));
    cout << ssrc << " ";

    cout << sport_ << " ";

    struct in_addr da{};
    da.s_addr = htonl(dip_);
    char sdst[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &da, sdst, sizeof(sdst));
    cout << sdst << " ";

    cout << dport_ << " ";

    cout << ip_len_ << " ";

    cout << proto_char_ << " ";

    cout << thlen_ << " ";

    cout << paylen_ << " ";

    if (proto_char_ == 'T')
        cout << seqno_ << " ";
    else
        cout << "- ";

    if (proto_char_ == 'T')
    {
        if (is_ack_)
            cout << ackno_;
        else
            cout << "-";
    }
    else
    {
        cout << "-";
    }

    cout << "\n";
}
