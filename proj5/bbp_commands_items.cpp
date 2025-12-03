#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sstream>
#include <vector>

void BbpCommandProcessor::handleAdd(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 3);
    std::vector<std::string> parts = splitBy(rest, ";;");

    std::string typeStr;
    std::string title;
    std::string body;

    if (!parts.empty())
        typeStr = trim(parts[0]);
    if (parts.size() >= 2)
        title = trim(parts[1]);
    if (parts.size() >= 3)
        body = parts[2];

    bool missingTitle = (parts.size() < 2) || title.empty();
    bool missingBody = (parts.size() < 3) || body.empty();

    if (missingTitle)
    {
        send_error(out, ErrorCode::MissingTitle);
        return;
    }

    if (missingBody)
    {
        send_error(out, ErrorCode::MissingBody);
        return;
    }

    if (parts.size() > 3)
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    ItemType type;
    if (!parseItemType(typeStr, type))
    {
        send_error(out, ErrorCode::UnknownType);
        return;
    }

    int id = store_.addItem(type, title, body);
    if (id == 0)
    {
        send_error(out, ErrorCode::ItemExists);
        return;
    }

    send_ok_payload(out, std::to_string(id) + " ;; " + title);
}

void BbpCommandProcessor::handleGet(const std::string &line, FILE *out)
{
    int id;
    if (!parseSingleIdCommand(line, 3, id, out))
        return;

    const Item *item = findItem(id);
    if (!item)
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok_payload(out, formatItemFull(*item));
}

void BbpCommandProcessor::handleList(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 4);
    ItemType type;
    if (!parseItemType(rest, type))
    {
        send_error(out, ErrorCode::UnknownType);
        return;
    }

    const auto &bucket = store_.typeBuckets[static_cast<size_t>(type)];
    if (bucket.empty())
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok(out);
    for (int id : bucket)
    {
        const Item *item = findItem(id);
        if (!item)
            continue;
        send_line(out, formatItemSummary(*item));
    }
    send_line(out, ".END");
}
