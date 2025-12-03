#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sstream>
#include <fstream>
#include <cstdio>

void BbpCommandProcessor::handleDelete(const std::string &line, FILE *out)
{
    int id;
    if (!parseSingleIdCommand(line, 6, id, out))
        return;

    const Item *item = findItem(id);
    if (!item)
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    Item deleted = *item;

    if (!store_.deleteItem(id))
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok_payload(out, formatItemFull(deleted));
}

void BbpCommandProcessor::handleNewBook(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 4);
    std::istringstream iss(rest);
    std::string name, extra;
    iss >> name;
    if (name.empty() || (iss >> extra))
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    if (!isValidBookName(name))
    {
        send_error(out, ErrorCode::InvalidBookName);
        return;
    }

    std::string itemsPath = name + "_items.db";
    std::string linksPath = name + "_links.db";

    if (fileExists(itemsPath) || fileExists(linksPath))
    {
        send_error(out, ErrorCode::BookExists);
        return;
    }

    {
        std::ofstream items(itemsPath);
        if (!items)
        {
            send_error(out, ErrorCode::BookCreateFailed);
            return;
        }
    }
    {
        std::ofstream links(linksPath);
        if (!links)
        {
            std::remove(itemsPath.c_str());
            send_error(out, ErrorCode::BookCreateFailed);
            return;
        }
    }

    store_.itemsFile = itemsPath;
    store_.linksFile = linksPath;
    store_.loadFromDisk();
    currentBookName_ = name;

    send_ok(out, OkCode::NewBookCreated);
}

void BbpCommandProcessor::handleLoadBook(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 5);
    std::istringstream iss(rest);
    std::string name, extra;
    iss >> name;
    if (name.empty() || (iss >> extra))
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    if (!isValidBookName(name))
    {
        send_error(out, ErrorCode::InvalidBookName);
        return;
    }

    std::string itemsPath = name + "_items.db";
    std::string linksPath = name + "_links.db";

    if (!fileExists(itemsPath))
    {
        send_error(out, ErrorCode::BookNotFound);
        return;
    }

    store_.itemsFile = itemsPath;
    store_.linksFile = linksPath;
    store_.loadFromDisk();
    currentBookName_ = name;

    send_ok(out, OkCode::BookLoaded);
}

void BbpCommandProcessor::handleDeleteBook(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 7);
    std::istringstream iss(rest);
    std::string name, key, extra;
    iss >> name >> key;
    if (name.empty() || key.empty() || (iss >> extra))
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    if (!isValidBookName(name))
    {
        send_error(out, ErrorCode::InvalidBookName);
        return;
    }

    if (key != "m0u53!")
    {
        send_error(out, ErrorCode::Unauthorized);
        return;
    }

    std::string itemsPath = name + "_items.db";
    std::string linksPath = name + "_links.db";

    bool anyExists = fileExists(itemsPath) || fileExists(linksPath);
    if (!anyExists)
    {
        send_error(out, ErrorCode::BookNotFound);
        return;
    }

    if (name == currentBookName_)
    {
        send_error(out, ErrorCode::CannotDeleteActiveBook);
        return;
    }

    bool ok = true;
    if (fileExists(itemsPath) && std::remove(itemsPath.c_str()) != 0)
    {
        ok = false;
    }
    if (fileExists(linksPath) && std::remove(linksPath.c_str()) != 0)
    {
        ok = false;
    }

    if (!ok)
    {
        send_error(out, ErrorCode::BookDeleteFailed);
        return;
    }

    send_ok(out, OkCode::BookDeleted);
}

void BbpCommandProcessor::handleWhichBook(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 6);
    if (!rest.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    if (currentBookName_.empty())
    {
        send_error(out, ErrorCode::NotFound);
        return;
    }

    send_ok_payload(out, currentBookName_);
}
