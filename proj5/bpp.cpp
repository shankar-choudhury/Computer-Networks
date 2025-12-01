// bbp.hpp
// Shankar Choudhury (sxc1782)
// Shared definitions for Book Builder Protocol (BBP)

#ifndef BBP_HPP
#define BBP_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <map>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cstdio>

enum class ItemType
{
    QUOTE = 0,
    PLOT,
    PHIL,
    CHARACT,
    THEME,
    Count
};

inline std::string itemTypeToString(ItemType t)
{
    switch (t)
    {
    case ItemType::QUOTE:
        return "QUOTE";
    case ItemType::PLOT:
        return "PLOT";
    case ItemType::PHIL:
        return "PHIL";
    case ItemType::CHARACT:
        return "CHAR";
    case ItemType::THEME:
        return "THEME";
    default:
        return "UNKNOWN";
    }
}

inline bool parseItemType(const std::string &s, ItemType &out)
{
    if (s == "QUOTE")
    {
        out = ItemType::QUOTE;
        return true;
    }
    if (s == "PLOT")
    {
        out = ItemType::PLOT;
        return true;
    }
    if (s == "PHIL")
    {
        out = ItemType::PHIL;
        return true;
    }
    if (s == "CHAR")
    {
        out = ItemType::CHARACT;
        return true;
    }
    if (s == "THEME")
    {
        out = ItemType::THEME;
        return true;
    }
    return false;
}

struct Item
{
    int id;
    ItemType type;
    std::string title;
    std::string body;
};

inline std::string trim(const std::string &s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        ++start;
    }
    if (start == s.size())
        return "";
    size_t end = s.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end])))
    {
        --end;
    }
    return s.substr(start, end - start + 1);
}

inline std::string toLower(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

inline bool startsWith(const std::string &s, const std::string &prefix)
{
    if (s.size() < prefix.size())
        return false;
    return std::equal(prefix.begin(), prefix.end(), s.begin());
}

inline std::vector<std::string> splitBy(const std::string &s, const std::string &delim)
{
    std::vector<std::string> parts;
    size_t pos = 0, dlen = delim.size();
    while (true)
    {
        size_t p = s.find(delim, pos);
        if (p == std::string::npos)
        {
            parts.push_back(s.substr(pos));
            break;
        }
        parts.push_back(s.substr(pos, p - pos));
        pos = p + dlen;
    }
    return parts;
}

class ItemStore
{
public:
    std::vector<Item> items;
    std::unordered_map<int, size_t> indexById;
    std::array<std::vector<int>, static_cast<size_t>(ItemType::Count)> typeBuckets;
    std::multimap<int, int> links;
    int next_id = 1;

    std::string itemsFile = "bbp_items.db";
    std::string linksFile = "bbp_links.db";

    void loadFromDisk()
    {
        loadItems();
        loadLinks();
    }

    void appendItemToDisk(const Item &i)
    {
        std::ofstream ofs(itemsFile.c_str(), std::ios::app);
        if (!ofs)
            return;
        ofs << i.id << '|' << itemTypeToString(i.type) << '|'
            << escape(i.title) << '|' << escape(i.body) << "\n";
    }

    void appendLinkToDisk(int from, int to)
    {
        std::ofstream ofs(linksFile.c_str(), std::ios::app);
        if (!ofs)
            return;
        ofs << from << '|' << to << "\n";
    }

    int addItem(ItemType type, const std::string &title, const std::string &body)
    {
        Item it;
        it.id = next_id++;
        it.type = type;
        it.title = title;
        it.body = body;
        items.push_back(it);
        indexById[it.id] = items.size() - 1;
        typeBuckets[static_cast<size_t>(type)].push_back(it.id);
        appendItemToDisk(it);
        return it.id;
    }

    void addLink(int from, int to)
    {
        links.insert({from, to});
        appendLinkToDisk(from, to);
    }

private:
    static std::string escape(const std::string &s)
    {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if (c == '\n')
                out += "\\n";
            else if (c == '|')
                out += "\\p";
            else
                out += c;
        }
        return out;
    }

    static std::string unescape(const std::string &s)
    {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                char n = s[i + 1];
                if (n == 'n')
                {
                    out += '\n';
                    ++i;
                    continue;
                }
                if (n == 'p')
                {
                    out += '|';
                    ++i;
                    continue;
                }
            }
            out += s[i];
        }
        return out;
    }

    void loadItems()
    {
        std::ifstream ifs(itemsFile.c_str());
        if (!ifs)
            return;

        items.clear();
        indexById.clear();
        for (auto &bucket : typeBuckets)
            bucket.clear();

        std::string line;
        int maxId = 0;
        while (std::getline(ifs, line))
        {
            if (line.empty())
                continue;
            std::vector<std::string> parts = splitBy(line, "|");
            if (parts.size() != 4)
                continue;
            int id = std::atoi(parts[0].c_str());
            ItemType type;
            if (!parseItemType(parts[1], type))
                continue;
            Item it;
            it.id = id;
            it.type = type;
            it.title = unescape(parts[2]);
            it.body = unescape(parts[3]);
            items.push_back(it);
            indexById[it.id] = items.size() - 1;
            typeBuckets[static_cast<size_t>(type)].push_back(it.id);
            if (id > maxId)
                maxId = id;
        }
        next_id = maxId + 1;
    }

    void loadLinks()
    {
        std::ifstream ifs(linksFile.c_str());
        if (!ifs)
            return;
        links.clear();
        std::string line;
        while (std::getline(ifs, line))
        {
            if (line.empty())
                continue;
            std::vector<std::string> parts = splitBy(line, "|");
            if (parts.size() != 2)
                continue;
            int from = std::atoi(parts[0].c_str());
            int to = std::atoi(parts[1].c_str());

            if (indexById.find(from) != indexById.end() &&
                indexById.find(to) != indexById.end())
            {
                links.insert({from, to});
            }
        }
    }
};

#endif
