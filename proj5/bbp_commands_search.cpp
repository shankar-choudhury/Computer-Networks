#include "bbp_commands.hpp"
#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sstream>
#include <vector>

void BbpCommandProcessor::handleSearch(const std::string &line, FILE *out)
{
    std::string rest = commandRest(line, 6);
    if (rest.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    std::istringstream iss(rest);
    std::string mode;
    iss >> mode;

    if (mode == "TYPE")
    {
        handleSearchType(iss, out);
    }
    else if (mode == "TITLE")
    {
        handleSearchTitle(iss, out);
    }
    else if (mode == "KEYWORDS")
    {
        handleSearchKeywords(iss, out);
    }
    else
    {
        send_error(out, ErrorCode::MalformedRequest);
    }
}

void BbpCommandProcessor::handleSearchType(std::istringstream &iss, FILE *out)
{
    std::string typeStr;
    iss >> typeStr;
    if (typeStr.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    std::string term;
    std::getline(iss, term);
    term = trim(term);
    if (term.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    ItemType type;
    if (!parseItemType(typeStr, type))
    {
        send_error(out, ErrorCode::TypeNotFound);
        return;
    }

    std::string termLower = toLower(term);
    const auto &bucket = store_.typeBuckets[static_cast<size_t>(type)];

    auto matches = collectMatches(bucket, [&](int id) -> const Item *
                                  {
                                      const Item *item = findItem(id);
                                      if (!item)
                                          return nullptr;

                                      std::string titleLower = toLower(item->title);
                                      std::string bodyLower = toLower(item->body);

                                      if (titleLower.find(termLower) != std::string::npos ||
                                          bodyLower.find(termLower) != std::string::npos)
                                      {
                                          return item;
                                      }
                                      return nullptr; });

    writeMatchesOrNotFound(out, matches);
}

void BbpCommandProcessor::handleSearchTitle(std::istringstream &iss, FILE *out)
{
    std::string term;
    std::getline(iss, term);
    term = trim(term);
    if (term.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    std::string termLower = toLower(term);

    auto matches = collectMatches(store_.items, [&](const Item &item) -> const Item *
                                  {
                                      std::string titleLower = toLower(item.title);
                                      if (titleLower.find(termLower) != std::string::npos)
                                      {
                                          return &item;
                                      }
                                      return nullptr; });

    writeMatchesOrNotFound(out, matches);
}

void BbpCommandProcessor::handleSearchKeywords(std::istringstream &iss, FILE *out)
{
    std::vector<std::string> keywords;
    std::string kw;
    while (iss >> kw)
    {
        keywords.push_back(toLower(kw));
    }
    if (keywords.empty())
    {
        send_error(out, ErrorCode::MalformedRequest);
        return;
    }

    auto matches = collectMatches(store_.items, [&](const Item &item) -> const Item *
                                  {
                                      std::string titleLower = toLower(item.title);
                                      std::string bodyLower = toLower(item.body);

                                      for (const auto &k : keywords)
                                      {
                                          if (titleLower.find(k) == std::string::npos &&
                                              bodyLower.find(k) == std::string::npos)
                                          {
                                              return nullptr;
                                          }
                                      }
                                      return &item; });

    writeMatchesOrNotFound(out, matches);
}
