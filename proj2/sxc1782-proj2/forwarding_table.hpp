#ifndef FORWARDING_TABLE_HPP
#define FORWARDING_TABLE_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <set>
#include <arpa/inet.h>
#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * Name: Shankar Choudhury
 * Case Network ID: sxc1782
 * Filename: forwarding_table.hpp
 * Date created: 2025-10-07
 * Brief description:
 *  This class represents a Forwarding Table. See the details below for a more
 *  complete explanation of the class and its public/private members
 *
 * =============================================================================
 *  Class: ForwardingTable
 *  ---------------------------------------------------------------------------
 *  Description:
 *  This class implements the forwarding-table logic for the router simulation
 *  environment. It encapsulates all operations required to parse, validate,
 *  and use routing records from a binary forwarding table file, supporting
 *  O(1) lookups for fixed prefix lengths (8, 16, 24, 32 bits).
 *
 *  ---------------------------------------------------------------------------
 *  Internal Structure:
 *  - Entry:
 *      A struct representing a single forwarding record.
 *      Contains:
 *        - addr:       IPv4 network address (in host byte order)
 *        - prefix_len: Length of the network prefix (8, 16, 24, 32)
 *        - iface:      Interface number for outgoing packets
 *
 *  - tables_:
 *      A map of prefix-lengths → prefix-to-entry hash tables.
 *      Each table provides O(1) lookup for IPs of the same prefix length.
 *      Only the prefix lengths {8, 16, 24, 32} are supported
 *
 *  - all_entries_:
 *      A linear vector of all parsed entries for debugging,
 *      reporting, or verification.
 *
 *  - default_iface_:
 *      Stores the interface ID for the default route (0.0.0.0/8).
 *      Used when no explicit prefix matches a destination.
 *
 *  ---------------------------------------------------------------------------
 *  File Loading Process (loadFromFile):
 *  1. Opens the binary forwarding table file and reads fixed-size Entry structs.
 *  2. Validates each entry for:
 *        - Correct prefix length (must be 8, 16, 24, or 32)
 *        - Absence of duplicate (prefix, prefix_len) pairs
 *  3. Converts all network byte order values to host order.
 *  4. Detects and stores the default route (addr = 0).
 *  5. Organizes entries into per-prefix tables for O(1) lookup.
 *  6. Validates that the final table is not empty.
 *
 *  ---------------------------------------------------------------------------
 *  Lookup Process (lookup):
 *  Given a destination IP address (in host byte order):
 *    1. The method iterates through prefix lengths in descending order
 *       (32 → 24 → 16 → 8) to enforce longest-prefix matching.
 *    2. For each prefix length, it computes a mask using prefixMask(),
 *       applies it to the destination IP, and checks for a match in
 *       the corresponding hash table.
 *    3. Returns the interface number on a match.
 *    4. If no match exists, and a default route is configured,
 *       returns the default interface.
 *    5. Returns -1 if no route exists.
 *
 *  ---------------------------------------------------------------------------
 *  Supporting Static Utilities:
 *    - prefixMask(int prefix_len):
 *        Returns a 32-bit mask corresponding to the given prefix length.
 *    - ipToString(uint32_t ip):
 *        Converts an IPv4 integer into dotted-decimal format.
 *    - readEntry():
 *        Reads an Entry struct from the file, converting from network byte order.
 *    - validateEntry():
 *        Ensures the prefix length is supported (8, 16, 24, 32).
 *    - checkDuplicate():
 *        Detects duplicate entries and throws an exception if found.
 *    - handleDefaultEntry():
 *        Identifies and records the default route entry (0.0.0.0/8).
 *    - storeEntry():
 *        Inserts entries into both the prefix table and master list.
 *
 *  ---------------------------------------------------------------------------
 *  Design Notes:
 *  - Only four prefix lengths are supported (8, 16, 24, and 32)
 *  - Lookup uses deterministic longest-prefix ordering.
 *  - Uses unordered_map for O(1) lookups and std::set for duplicate detection.
 *
 *  ---------------------------------------------------------------------------
 *  Example Usage:
 *    ForwardingTable table("forwarding_table.bin");
 *    bool is_default = false;
 *    uint32_t ip = inet_addr("192.168.1.5");
 *    int iface = table.lookup(ntohl(ip), is_default);
 *    if (iface >= 0)
 *        std::cout << "Forward packet via interface " << iface << std::endl;
 *    else
 *        std::cerr << "No route found for destination." << std::endl;
 * =============================================================================
 */
class ForwardingTable
{
public:
    struct Entry
    {
        uint32_t addr;
        uint16_t prefix_len;
        uint16_t iface;
    };

    explicit ForwardingTable(const std::string &filename)
    {
        loadFromFile(filename);
    }

    int lookup(uint32_t dest_ip, bool &is_default) const
    {
        for (int plen : PREFIX_LENGTHS)
        {
            uint32_t masked = dest_ip & prefixMask(plen);
            auto it = tables_.at(plen).find(masked);
            if (it != tables_.at(plen).end())
            {
                is_default = false;
                return it->second.iface;
            }
        }

        if (default_iface_ >= 0)
        {
            is_default = true;
            return default_iface_;
        }

        is_default = false;
        return -1;
    }

    bool hasDefault() const noexcept { return default_iface_ >= 0; }
    int getDefault() const noexcept { return default_iface_; }
    const std::vector<Entry> &entries() const noexcept { return all_entries_; }

private:
    inline static constexpr int PREFIX_LENGTHS[4] = {32, 24, 16, 8};

    std::vector<Entry> all_entries_;
    std::unordered_map<int, std::unordered_map<uint32_t, Entry>> tables_;
    int default_iface_ = -1;

    void loadFromFile(const std::string &filename)
    {
        std::ifstream file = openFile(filename);
        initializeTables();

        std::set<std::pair<uint32_t, uint16_t>> seen_prefixes;

        while (true)
        {
            Entry entry;
            if (!readEntry(file, entry))
                break;

            validateEntry(entry);

            uint32_t masked = entry.addr & prefixMask(entry.prefix_len);
            checkDuplicate(entry, masked, seen_prefixes);
            handleDefaultEntry(entry);

            storeEntry(entry, masked);
        }

        validateFinalTable();
    }

    static std::ifstream openFile(const std::string &filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Error: cannot open forwarding file '" + filename + "'");
        return file;
    }

    void initializeTables()
    {
        for (int plen : PREFIX_LENGTHS)
            tables_[plen] = {};
    }

    static bool readEntry(std::ifstream &file, Entry &entry)
    {
        if (!file.read(reinterpret_cast<char *>(&entry), sizeof(entry)))
            return false;

        entry.addr = ntohl(entry.addr);
        entry.prefix_len = ntohs(entry.prefix_len);
        entry.iface = ntohs(entry.iface);
        return true;
    }

    static void validateEntry(const Entry &entry)
    {
        switch (entry.prefix_len)
        {
        case 8:
        case 16:
        case 24:
        case 32:
            return;
        default:
            throw std::runtime_error(
                "Error: invalid prefix length (" + std::to_string(entry.prefix_len) + ")");
        }
    }

    static void checkDuplicate(const Entry &entry, uint32_t masked,
                               std::set<std::pair<uint32_t, uint16_t>> &seen_prefixes)
    {
        auto key = std::make_pair(masked, entry.prefix_len);
        if (!seen_prefixes.insert(key).second)
        {
            throw std::runtime_error(
                "Error: duplicate prefix detected (" +
                ipToString(masked) + "/" + std::to_string(entry.prefix_len) + ")");
        }
    }

    void handleDefaultEntry(Entry &entry)
    {
        if (entry.addr == 0)
        {
            entry.prefix_len = 8;
            default_iface_ = entry.iface;
        }
    }

    void storeEntry(const Entry &entry, uint32_t masked)
    {
        tables_[entry.prefix_len][masked] = entry;
        all_entries_.push_back(entry);
    }

    void validateFinalTable() const
    {
        if (all_entries_.empty() && default_iface_ < 0)
            throw std::runtime_error("Error: forwarding table is empty");
    }

    static uint32_t prefixMask(int prefix_len)
    {
        return prefix_len == 0 ? 0 : 0xFFFFFFFF << (32 - prefix_len);
    }

    static std::string ipToString(uint32_t ip)
    {
        uint8_t a = (ip >> 24) & 0xFF;
        uint8_t b = (ip >> 16) & 0xFF;
        uint8_t c = (ip >> 8) & 0xFF;
        uint8_t d = ip & 0xFF;
        return std::to_string(a) + "." + std::to_string(b) + "." +
               std::to_string(c) + "." + std::to_string(d);
    }
};

#endif
