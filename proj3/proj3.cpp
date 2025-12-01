/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: proj3.cpp
 * Date created: 2025-10-28
 * Brief description:
 *  This program simulates a router with three modes:
 *   -p : packet printing mode
 *   -n : netflow mode
 *   -s : rount trip time mode
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include "Packet.hpp"
#include "FlowTracker.hpp"
#include "RTTTracker.hpp"

using namespace std;

struct CliArgs
{
    bool packet_mode = false;
    bool netflow_mode = false;
    bool rtt_mode = false;
    string trace_file;
};

void usage(const char *progname)
{
    cerr << "Usage: " << progname << " <-p|-n|-r> -f trace_file\n"
         << " -p : Packet printing mode (requires -f)\n"
         << " -n : NetFlow mode (requires -f)\n"
         << " -r : RTT mode (requires -f)\n";
    exit(EXIT_FAILURE);
}

void parseArgs(int argc, char *argv[], CliArgs &args)
{
    int opt;
    while ((opt = getopt(argc, argv, "pnrf:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            args.packet_mode = true;
            break;
        case 'n':
            args.netflow_mode = true;
            break;
        case 'r':
            args.rtt_mode = true;
            break;
        case 'f':
            args.trace_file = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    int mode_count = args.packet_mode + args.netflow_mode + args.rtt_mode;
    if (mode_count != 1)
    {
        cerr << "Error: Specify exactly one mode (-p, -n, or -r)\n";
        usage(argv[0]);
    }

    if (args.trace_file.empty())
    {
        cerr << "Error: -f trace_file is required\n";
        usage(argv[0]);
    }
}

void runPacketMode(const string &tracefile)
{
    ifstream file(tracefile, ios::binary);
    if (!file.is_open())
    {
        cerr << "Error: Cannot open trace file '" << tracefile << "'\n";
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        Packet p;
        if (!p.readFromStream(file))
            break;

        if (!p.isValidForPrint())
            continue;

        p.printPacket();
    }

    file.close();
}

void runNetflowMode(const std::string &tracefile)
{
    std::ifstream file(tracefile, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open trace file '" << tracefile << "'\n";
        exit(EXIT_FAILURE);
    }

    FlowTracker tracker;
    while (true)
    {
        Packet p;
        if (!p.readFromStream(file))
            break;
        if (!p.isValidForPrint())
            continue;
        tracker.addPacket(p);
    }
    file.close();
    tracker.printFlows();
}

void runRTTMode(const std::string &tracefile)
{
    std::ifstream file(tracefile, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open trace file '" << tracefile << "'\n";
        exit(EXIT_FAILURE);
    }

    RTTTracker tracker;
    while (true)
    {
        Packet p;
        if (!p.readFromStream(file))
            break;
        if (!p.isValidForPrint())
            continue;
        tracker.addPacket(p);
    }
    file.close();
    tracker.printFlows();
}

int main(int argc, char *argv[])
{
    CliArgs args;
    parseArgs(argc, argv, args);

    if (args.packet_mode)
    {
        runPacketMode(args.trace_file);
    }
    else if (args.netflow_mode)
    {
        runNetflowMode(args.trace_file);
    }
    else if (args.rtt_mode)
    {
        runRTTMode(args.trace_file);
    }

    return 0;
}