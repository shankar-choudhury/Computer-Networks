#ifndef BBP_COMMANDS_HPP
#define BBP_COMMANDS_HPP

#include "bbp.hpp"
#include "bbp_status.hpp"

#include <cstdio>
#include <string>
#include <vector>
#include <sstream>

using bbp::ErrorCode;
using bbp::OkCode;

#define BBP_COMMANDS(X)                                                                                                                \
    X(ADD, handleAdd, "ADD TYPE;;title;;body - Add a new item of the given TYPE with the given title and body.")                       \
    X(GET, handleGet, "GET id - Retrieve the full details of the item with the given id.")                                             \
    X(LIST, handleList, "LIST TYPE - List all items of the given TYPE.")                                                               \
    X(SEARCH, handleSearch, "SEARCH TYPE|TITLE|KEYWORDS ... - Search items by type, title, or keywords.")                              \
    X(LINK, handleLink, "LINK id1 id2 - Create a link between two existing items.")                                                    \
    X(CONTEXT, handleContext, "CONTEXT id - Show the item and all items it is directly linked to.")                                    \
    X(OUTLINE, handleOutline, "OUTLINE - Show a high-level outline of items for current book grouped by type.")                        \
    X(DELETE, handleDelete, "DELETE id - Delete the item with the given id.")                                                          \
    X(NEWB, handleNewBook, "NEWB name - Create a new empty book backed by name_items.db/name_links.db and switch to it.")              \
    X(LOADB, handleLoadBook, "LOADB name - Load an existing book (name_items.db/name_links.db) into the server.")                      \
    X(DELETEB, handleDeleteBook, "DELETEB name <secret key> - Delete the files for the named book (not allowed for the active book).") \
    X(WHICHB, handleWhichBook, "WHICHB - Show the name of the currently active book.")                                                 \
    X(USAGE, handleUsage, "USAGE [command] - Show help for all commands or for a specific command.")

class BbpCommandProcessor
{
public:
    explicit BbpCommandProcessor(ItemStore &store);

    void handleLine(const std::string &line, FILE *out);

private:
    ItemStore &store_;
    std::string currentBookName_;

    void send_line(FILE *out, const std::string &line);
    void send_error(FILE *out, ErrorCode code);
    void send_ok(FILE *out, OkCode code = OkCode::Simple);
    void send_ok_payload(FILE *out, const std::string &payload);

    static std::string commandRest(const std::string &line, std::size_t prefixLen);
    static bool parseId(const std::string &s, int &idOut);
    bool parseSingleIdCommand(const std::string &line,
                              std::size_t prefixLen,
                              int &idOut,
                              FILE *out);

    const Item *findItem(int id) const;
    static std::string formatItemSummary(const Item &item);
    static std::string formatItemFull(const Item &item);

    template <typename Range, typename MatchFn>
    std::vector<const Item *> collectMatches(const Range &range, MatchFn match)
    {
        std::vector<const Item *> matches;
        for (const auto &elem : range)
        {
            const Item *it = match(elem);
            if (it)
                matches.push_back(it);
        }
        return matches;
    }

    void writeMatchesOrNotFound(FILE *out, const std::vector<const Item *> &matches);

    static bool isValidBookName(const std::string &name);
    static bool fileExists(const std::string &path);
    static std::string deriveBookNameFromItemsFile(const std::string &itemsFile);
    static std::string toUpper(const std::string &s);

    // Defined in bbp_commands_items
    void handleAdd(const std::string &line, FILE *out);
    void handleGet(const std::string &line, FILE *out);
    void handleList(const std::string &line, FILE *out);

    // Defined in bbp_commands_search
    void handleSearch(const std::string &line, FILE *out);
    void handleSearchType(std::istringstream &iss, FILE *out);
    void handleSearchTitle(std::istringstream &iss, FILE *out);
    void handleSearchKeywords(std::istringstream &iss, FILE *out);

    // Defined in bbp_commands_links
    void handleLink(const std::string &line, FILE *out);
    void handleContext(const std::string &line, FILE *out);
    void handleOutline(const std::string &line, FILE *out);

    // Defined in bbp_commands_books
    void handleDelete(const std::string &line, FILE *out);
    void handleNewBook(const std::string &line, FILE *out);
    void handleLoadBook(const std::string &line, FILE *out);
    void handleDeleteBook(const std::string &line, FILE *out);
    void handleWhichBook(const std::string &line, FILE *out);

    // Defined in bbp_commands_usage
    void handleUsage(const std::string &line, FILE *out);
};

#endif
