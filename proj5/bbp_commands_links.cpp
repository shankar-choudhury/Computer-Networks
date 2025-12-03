#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sstream>
#include <vector>

void BbpCommandProcessor::handleLink(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 4);
    std::istringstream iss(rest);

    std::string fromStr, toStr;
    if (!(iss >> fromStr >> toStr))
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    int fromId, toId;
    if (!parseId(fromStr, fromId) || !parseId(toStr, toId))
    {
        send_error(out, ErrorCode::MalformedId);
        return;
    }

    if (fromId == toId)
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    const Item *fromItem = findItem(fromId);
    const Item *toItem = findItem(toId);
    if (!fromItem || !toItem)
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    if (!store_.addLink(fromId, toId))
    {
        send_error(out, ErrorCode::LinkExists);
        return;
    }

    send_ok(out);
}

void BbpCommandProcessor::handleContext(const std::string &line, FILE *out)
{
    int id;
    if (!parseSingleIdCommand(line, 7, id, out))
        return;

    const Item *center = findItem(id);
    if (!center)
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok(out, OkCode::Context);
    send_line(out, "ITEM:");
    send_line(out, formatItemFull(*center));
    send_line(out, "");
    send_line(out, "LINKED-TO:");

    std::vector<int> nbrs = store_.neighborsOf(id);
    for (int nid : nbrs)
    {
        const Item *target = findItem(nid);
        if (!target)
            continue;

        send_line(out, formatItemFull(*target));
    }

    send_line(out, ".END");
}

void BbpCommandProcessor::handleOutline(const std::string & /*line*/, FILE *out)
{
    send_ok(out, OkCode::Outline);

    auto printBucket = [&](ItemType type, const std::string &header)
    {
        send_line(out, header + ":");
        const auto &bucket = store_.typeBuckets[static_cast<size_t>(type)];
        for (int id : bucket)
        {
            auto it = store_.indexById.find(id);
            if (it == store_.indexById.end())
                continue;
            const Item &item = store_.items[it->second];
            std::string l = "  " + std::to_string(item.id) + " ;; " + item.title;
            send_line(out, l);
        }
        send_line(out, "");
    };

    printBucket(ItemType::THEME, "THEMES");
    printBucket(ItemType::CHAR, "CHARACTERS");
    printBucket(ItemType::PLOT, "PLOT");
    printBucket(ItemType::PHIL, "PHILOSOPHIES");
    printBucket(ItemType::QUOTE, "QUOTES");

    send_line(out, ".END");
}
