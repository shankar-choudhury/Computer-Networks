#ifndef BBP_HPP
#define BBP_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
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
    CHAR,
    THEME,
    Count
};

namespace bbp
{
    class ItemTypeMap
    {
    public:
        static const std::string &toString(ItemType t)
        {
            const auto &tbl = table();
            std::size_t idx = static_cast<std::size_t>(t);
            if (idx >= tbl.size())
            {
                static const std::string unknown = "UNKNOWN";
                return unknown;
            }
            return tbl[idx];
        }

        static bool tryParse(const std::string &s, ItemType &out)
        {
            const auto &tbl = table();
            for (std::size_t i = 0; i < tbl.size(); ++i)
            {
                if (s == tbl[i])
                {
                    out = static_cast<ItemType>(i);
                    return true;
                }
            }
            return false;
        }

    private:
        static const std::array<std::string,
                                static_cast<std::size_t>(ItemType::Count)> &
        table()
        {
            static const std::array<std::string,
                                    static_cast<std::size_t>(ItemType::Count)>
                TABLE = {
                    "QUOTE",
                    "PLOT",
                    "PHIL",
                    "CHAR",
                    "THEME"};
            static_assert(
                TABLE.size() == static_cast<std::size_t>(ItemType::Count),
                "ItemTypeMap TABLE size must match ItemType::Count");
            return TABLE;
        }
    };
}

inline std::string itemTypeToString(ItemType t)
{
    return bbp::ItemTypeMap::toString(t);
}

inline bool parseItemType(const std::string &s, ItemType &out)
{
    return bbp::ItemTypeMap::tryParse(s, out);
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
    while (start < s.size() && std::isspace((unsigned char)s[start]))
        ++start;
    if (start == s.size())
        return "";

    size_t end = s.size() - 1;
    while (end > start && std::isspace((unsigned char)s[end]))
        --end;

    return s.substr(start, end - start + 1);
}

inline std::string toLower(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back(std::tolower((unsigned char)c));
    return out;
}

inline bool startsWith(const std::string &s, const std::string &prefix)
{
    return s.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), s.begin());
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
        std::ofstream ofs(itemsFile, std::ios::app);
        if (!ofs)
            return;
        ofs << i.id << '|' << itemTypeToString(i.type) << '|'
            << escape(i.title) << '|' << escape(i.body) << "\n";
    }

    void appendLinkToDisk(int a, int b)
    {
        std::ofstream ofs(linksFile, std::ios::app);
        if (!ofs)
            return;
        ofs << a << '|' << b << "\n";
    }

    int addItem(ItemType type, const std::string &title, const std::string &body)
    {
        std::string nt = normalizeTitle(title);
        if (titles.find(nt) != titles.end())
        {
            return 0;
        }

        Item it{next_id++, type, title, body};

        items.push_back(it);
        indexById[it.id] = items.size() - 1;
        typeBuckets[(size_t)type].push_back(it.id);

        titles.insert(nt);
        appendItemToDisk(it);

        return it.id;
    }

    bool addLink(int from, int to)
    {
        if (from == to)
            return false;

        auto &nbrsFrom = adj[from];
        if (nbrsFrom.find(to) != nbrsFrom.end())
        {
            return false;
        }

        nbrsFrom.insert(to);
        adj[to].insert(from);

        int a = std::min(from, to);
        int b = std::max(from, to);
        appendLinkToDisk(a, b);

        return true;
    }

    bool hasLink(int a, int b) const
    {
        if (a == b)
            return false;

        auto it = adj.find(a);
        if (it == adj.end())
            return false;

        return it->second.find(b) != it->second.end();
    }

    std::vector<int> neighborsOf(int center) const
    {
        std::vector<int> out;
        auto it = adj.find(center);
        if (it == adj.end())
            return out;

        out.reserve(it->second.size());
        for (int nbr : it->second)
        {
            out.push_back(nbr);
        }
        return out;
    }

    bool deleteItem(int id)
    {
        auto it = indexById.find(id);
        if (it == indexById.end())
        {
            return false;
        }

        size_t idx = it->second;
        Item victim = items[idx];

        titles.erase(normalizeTitle(victim.title));

        auto &bucket = typeBuckets[static_cast<size_t>(victim.type)];
        bucket.erase(std::remove(bucket.begin(), bucket.end(), victim.id), bucket.end());

        size_t lastIndex = items.size() - 1;
        if (idx != lastIndex)
        {
            Item &moved = items[lastIndex];
            items[idx] = moved;
            indexById[moved.id] = idx;
        }
        items.pop_back();
        indexById.erase(it);

        auto adjIt = adj.find(id);
        if (adjIt != adj.end())
        {
            for (int nbr : adjIt->second)
            {
                auto nIt = adj.find(nbr);
                if (nIt != adj.end())
                {
                    nIt->second.erase(id);
                }
            }
            adj.erase(adjIt);
        }

        rewriteItemsFile();
        rewriteLinksFile();

        return true;
    }

private:
    std::unordered_set<std::string> titles;
    std::unordered_map<int, std::unordered_set<int>> adj;

    static std::string normalizeTitle(const std::string &s)
    {
        return toLower(trim(s));
    }

    static std::string escape(const std::string &s)
    {
        std::string out;
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
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                if (s[i + 1] == 'n')
                {
                    out += '\n';
                    i++;
                    continue;
                }
                if (s[i + 1] == 'p')
                {
                    out += '|';
                    i++;
                    continue;
                }
            }
            out += s[i];
        }
        return out;
    }

    void loadItems()
    {
        std::ifstream ifs(itemsFile);
        if (!ifs)
            return;

        items.clear();
        indexById.clear();
        titles.clear();

        for (auto &b : typeBuckets)
            b.clear();

        std::string line;
        int maxId = 0;

        while (std::getline(ifs, line))
        {
            if (line.empty())
                continue;

            auto parts = splitBy(line, "|");
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
            indexById[id] = items.size() - 1;
            typeBuckets[(size_t)type].push_back(id);
            titles.insert(normalizeTitle(it.title));

            maxId = std::max(maxId, id);
        }

        next_id = maxId + 1;
    }

    void loadLinks()
    {
        std::ifstream ifs(linksFile);
        if (!ifs)
            return;

        adj.clear();

        std::string line;
        while (std::getline(ifs, line))
        {
            if (line.empty())
                continue;

            auto parts = splitBy(line, "|");
            if (parts.size() != 2)
                continue;

            int from = std::atoi(parts[0].c_str());
            int to = std::atoi(parts[1].c_str());

            if (indexById.count(from) == 0 || indexById.count(to) == 0)
                continue;

            int a = std::min(from, to);
            int b = std::max(from, to);

            adj[a].insert(b);
            adj[b].insert(a);
        }
    }

    void rewriteItemsFile()
    {
        std::ofstream ofs(itemsFile, std::ios::trunc);
        if (!ofs)
            return;

        for (const auto &it : items)
        {
            ofs << it.id << '|' << itemTypeToString(it.type) << '|'
                << escape(it.title) << '|' << escape(it.body) << "\n";
        }
    }

    void rewriteLinksFile()
    {
        std::ofstream ofs(linksFile, std::ios::trunc);
        if (!ofs)
            return;

        for (const auto &entry : adj)
        {
            int a = entry.first;
            for (int b : entry.second)
            {
                if (a < b)
                {
                    ofs << a << '|' << b << "\n";
                }
            }
        }
    }
};

#endif
