#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cctype>

BbpCommandProcessor::BbpCommandProcessor(ItemStore &store)
    : store_(store),
      currentBookName_(deriveBookNameFromItemsFile(store_.itemsFile))
{
}

void BbpCommandProcessor::handleLine(const std::string &line, FILE *out)
{
    std::istringstream iss(line);
    std::string cmdWord;
    iss >> cmdWord;
    if (cmdWord.empty())
    {
        send_error(out, ErrorCode::EmptyRequest);
        return;
    }

    struct Entry
    {
        const char *name;
        void (BbpCommandProcessor::*handler)(const std::string &, FILE *);
    };

    static const Entry TABLE[] = {
#define BBP_DEFINE_CMD_ENTRY(NAME, HANDLER, HELP) {#NAME, &BbpCommandProcessor::HANDLER},
        BBP_COMMANDS(BBP_DEFINE_CMD_ENTRY)
#undef BBP_DEFINE_CMD_ENTRY
            {nullptr, nullptr}};

    for (const Entry *e = TABLE; e->name != nullptr; ++e)
    {
        if (cmdWord == e->name)
        {
            (this->*e->handler)(line, out);
            return;
        }
    }

    send_error(out, ErrorCode::CommandNotFound);
}

void BbpCommandProcessor::send_line(FILE *out, const std::string &line)
{
    if (!out)
        return;
    std::string withNL = line + "\n";
    std::fputs(withNL.c_str(), out);
    std::fflush(out);
    std::cout << "S -> C: " << line << std::endl;
}

void BbpCommandProcessor::send_error(FILE *out, ErrorCode code)
{
    send_line(out, bbp::errorCodeToString(code));
}

void BbpCommandProcessor::send_ok(FILE *out, OkCode code)
{
    send_line(out, bbp::okCodeToString(code));
}

void BbpCommandProcessor::send_ok_payload(FILE *out, const std::string &payload)
{
    send_line(out, std::string("OK ") + payload);
}

std::string BbpCommandProcessor::commandRest(const std::string &line, std::size_t prefixLen)
{
    if (line.size() <= prefixLen)
        return "";
    return trim(line.substr(prefixLen));
}

bool BbpCommandProcessor::parseId(const std::string &s, int &idOut)
{
    if (s.empty())
        return false;
    char *endp = nullptr;
    long tmp = std::strtol(s.c_str(), &endp, 10);
    if (*endp != '\0')
        return false;
    idOut = static_cast<int>(tmp);
    return true;
}

bool BbpCommandProcessor::parseSingleIdCommand(const std::string &line,
                                               std::size_t prefixLen,
                                               int &idOut,
                                               FILE *out)
{
    std::string rest = commandRest(line, prefixLen);
    if (!parseId(rest, idOut))
    {
        send_error(out, ErrorCode::MalformedId);
        return false;
    }
    return true;
}

const Item *BbpCommandProcessor::findItem(int id) const
{
    auto it = store_.indexById.find(id);
    if (it == store_.indexById.end())
        return nullptr;
    return &store_.items[it->second];
}

std::string BbpCommandProcessor::formatItemSummary(const Item &item)
{
    return std::to_string(item.id) + " ;; " +
           item.title + " ;; " +
           item.body;
}

std::string BbpCommandProcessor::formatItemFull(const Item &item)
{
    return std::to_string(item.id) + " ;; " +
           itemTypeToString(item.type) + " ;; " +
           item.title + " ;; " + item.body;
}

void BbpCommandProcessor::writeMatchesOrNotFound(FILE *out,
                                                 const std::vector<const Item *> &matches)
{
    if (matches.empty())
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }
    send_ok(out);
    for (const Item *it : matches)
    {
        send_line(out, formatItemSummary(*it));
    }
    send_line(out, ".END");
}

bool BbpCommandProcessor::isValidBookName(const std::string &name)
{
    if (name.empty())
        return false;
    for (char c : name)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

bool BbpCommandProcessor::fileExists(const std::string &path)
{
    std::ifstream in(path);
    return in.good();
}

std::string BbpCommandProcessor::deriveBookNameFromItemsFile(const std::string &itemsFile)
{
    const std::string suffix = "_items.db";
    if (itemsFile.size() >= suffix.size())
    {
        std::size_t pos = itemsFile.rfind(suffix);
        if (pos != std::string::npos && pos + suffix.size() == itemsFile.size())
        {
            return itemsFile.substr(0, pos);
        }
    }
    return itemsFile;
}

std::string BbpCommandProcessor::toUpper(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s)
    {
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return out;
}
