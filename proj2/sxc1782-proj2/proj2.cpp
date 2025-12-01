/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: proj2.cpp
 * Date created: 2025-10-07
 * Brief description:
 *  This program simulates a router with three modes:
 *   -p : packet printing mode
 *   -r : forwarding table printing mode
 *   -s : simulation mode
 *
 * Usage:
 *   ./proj2 <-p|-r|-s> [-f forward_file] [-t trace_file]
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <unistd.h>
#include "forwarding_table.hpp"

using namespace std;

struct CliArgs
{
    bool packet_mode = false;
    bool table_mode = false;
    bool sim_mode = false;
    string forward_file;
    string trace_file;
};

void usage(const char *progname)
{
    cerr << "Usage: " << progname << " <-p|-r|-s> [-f forward_file] [-t trace_file]\n"
         << "  -p : Packet printing mode (requires -t)\n"
         << "  -r : Forwarding table printing mode (requires -f)\n"
         << "  -s : Simulation mode (requires -f and -t)\n";
    exit(EXIT_FAILURE);
}

void parseArgs(int argc, char *argv[], CliArgs &args)
{
    int opt;
    while ((opt = getopt(argc, argv, "prs f:t:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            args.packet_mode = true;
            break;
        case 'r':
            args.table_mode = true;
            break;
        case 's':
            args.sim_mode = true;
            break;
        case 'f':
            args.forward_file = optarg;
            break;
        case 't':
            args.trace_file = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    int mode_count = args.packet_mode + args.table_mode + args.sim_mode;
    if (mode_count != 1)
    {
        cerr << "Error: Specify exactly one mode (-p, -r, or -s)\n";
        usage(argv[0]);
    }

    if ((args.packet_mode && args.trace_file.empty()) ||
        (args.table_mode && args.forward_file.empty()) ||
        (args.sim_mode && (args.forward_file.empty() || args.trace_file.empty())))
    {
        usage(argv[0]);
    }
}

ifstream openFile(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Cannot open file '" << filename << "'\n";
        exit(EXIT_FAILURE);
    }
    return file;
}

double readTimestamp(ifstream &file)
{
    uint32_t sec, usec;
    if (!file.read(reinterpret_cast<char *>(&sec), 4))
        return -1;
    if (!file.read(reinterpret_cast<char *>(&usec), 4))
        return -1;

    return static_cast<double>(ntohl(sec)) +
           static_cast<double>(ntohl(usec)) / 1'000'000.0;
}

bool readIpHeader(ifstream &file, iphdr &hdr)
{
    return static_cast<bool>(file.read(reinterpret_cast<char *>(&hdr), sizeof(hdr)));
}

string ipToString(uint32_t ip)
{
    in_addr addr{ip};
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, str, sizeof(str));
    return str;
}

bool isChecksumValid(const iphdr &hdr)
{
    return ntohs(hdr.check) == 1234;
}

void printPacketTrace(const string &tracefile)
{
    ifstream file = openFile(tracefile);

    while (true)
    {
        double timestamp = readTimestamp(file);
        if (timestamp < 0)
            break;

        iphdr hdr{};
        if (!readIpHeader(file, hdr))
            break;

        string src = ipToString(hdr.saddr);
        string dst = ipToString(hdr.daddr);
        bool checksum_ok = isChecksumValid(hdr);

        cout << fixed << setprecision(6)
             << timestamp << " "
             << src << " "
             << dst << " "
             << (checksum_ok ? "P" : "F") << " "
             << static_cast<int>(hdr.ttl) << "\n";
    }

    file.close();
}

void printForwardingTable(const string &fname)
{
    ForwardingTable ft(fname);

    for (const auto &entry : ft.entries())
    {
        uint32_t ip = entry.addr;
        uint16_t prefix = entry.prefix_len;
        uint16_t iface = entry.iface;

        uint8_t a = (ip >> 24) & 0xFF;
        uint8_t b = (ip >> 16) & 0xFF;
        uint8_t c = (ip >> 8) & 0xFF;
        uint8_t d = ip & 0xFF;

        cout << dec << (int)a << "." << (int)b << "." << (int)c << "." << (int)d
             << " " << prefix << " " << iface << "\n";
    }
}

string determinePacketAction(const iphdr &hdr, const ForwardingTable &ft)
{
    if (!isChecksumValid(hdr))
        return "drop checksum";
    if (hdr.ttl == 1)
        return "drop expired";

    bool is_default = false;
    int iface = ft.lookup(ntohl(hdr.daddr), is_default);

    if (iface == 0)
        return "drop policy";
    if (iface > 0 && !is_default)
        return "send " + to_string(iface);
    if (is_default)
        return "default " + to_string(iface);

    return "drop unknown";
}

void simulatePackets(const string &forward_file, const string &trace_file)
{
    ForwardingTable ft(forward_file);
    ifstream file = openFile(trace_file);

    while (true)
    {
        double timestamp = readTimestamp(file);
        if (timestamp < 0)
            break;

        iphdr hdr{};
        if (!readIpHeader(file, hdr))
            break;

        string action = determinePacketAction(hdr, ft);
        cout << fixed << setprecision(6) << timestamp << " " << action << "\n";
    }

    file.close();
}

int main(int argc, char *argv[])
{
    CliArgs args;
    parseArgs(argc, argv, args);

    if (args.packet_mode)
    {
        printPacketTrace(args.trace_file);
    }
    else if (args.table_mode)
    {
        printForwardingTable(args.forward_file);
    }
    else if (args.sim_mode)
    {
        simulatePackets(args.forward_file, args.trace_file);
    }

    return 0;
}
