#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sstream>
#include <unordered_map>
#include <vector>

namespace
{
    struct UsageEntry
    {
        const char *name;
        const char *text;
    };

    constexpr UsageEntry USAGE_ENTRIES[] = {
#define BBP_DEFINE_USAGE_ENTRY(NAME, HANDLER, HELP) {#NAME, HELP},
        BBP_COMMANDS(BBP_DEFINE_USAGE_ENTRY)
#undef BBP_DEFINE_USAGE_ENTRY
    };

    using UsageMap = std::unordered_map<std::string, std::string>;

    const UsageMap &getUsageMap()
    {
        static const UsageMap m = []
        {
            UsageMap tmp;
            for (const auto &e : USAGE_ENTRIES)
            {
                tmp.emplace(e.name, e.text);
            }
            return tmp;
        }();
        return m;
    }

    const std::vector<std::string> &getUsageOrder()
    {
        static const std::vector<std::string> v = []
        {
            std::vector<std::string> tmp;
            tmp.reserve(sizeof(USAGE_ENTRIES) / sizeof(USAGE_ENTRIES[0]));
            for (const auto &e : USAGE_ENTRIES)
            {
                tmp.emplace_back(e.name);
            }
            return tmp;
        }();
        return v;
    }
}

void BbpCommandProcessor::handleUsage(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 5);
    std::string cmd;
    {
        std::istringstream iss(rest);
        iss >> cmd;
    }
    cmd = toUpper(cmd);

    const auto &USAGE_MAP = getUsageMap();
    const auto &USAGE_ORDER = getUsageOrder();

    auto sendAll = [&]()
    {
        send_ok(out);
        for (const auto &name : USAGE_ORDER)
        {
            auto it = USAGE_MAP.find(name);
            if (it != USAGE_MAP.end())
            {
                send_line(out, it->second);
            }
        }
        send_line(out, ".END");
    };

    if (cmd.empty())
    {
        sendAll();
        return;
    }

    auto it = USAGE_MAP.find(cmd);
    if (it == USAGE_MAP.end())
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok(out);
    send_line(out, it->second);
    send_line(out, ".END");
}
