// proj5d.cpp
// Shankar Choudhury (sxc1782)
// BBP server (Book Builder Protocol) - proj5d

#include "bbp.hpp"
#include "bbp_status.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <sstream>

using bbp::ErrorCode;
using bbp::OkCode;


static int parse_port_from_args(int argc, char **argv)
{
    int port = 0;
    for (int i = 1; i < argc - 1; ++i)
    {
        if (std::string(argv[i]) == "-p")
        {
            port = std::atoi(argv[i + 1]);
        }
    }
    if (port <= 0)
    {
        std::fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        std::exit(1);
    }
    return port;
}


class BbpServer
{
public:
    explicit BbpServer(int port)
        : listenfd_(-1)
    {
        store_.loadFromDisk();

        listenfd_ = create_listening_socket(port);
        if (listenfd_ < 0)
        {
            std::fprintf(stderr, "Failed to create listening socket on port %d\n", port);
            std::exit(1);
        }

        std::cout << "BBP server listening on port " << port << std::endl;
    }

    ~BbpServer()
    {
        if (listenfd_ >= 0)
        {
            close(listenfd_);
        }
    }

    void run()
    {
        while (true)
        {
            acceptAndServeOneClient();
        }
    }

private:
    ItemStore store_;
    int listenfd_;

    using HandlerFn = void (BbpServer::*)(const std::string &, FILE *);

    struct CommandEntry
    {
        const char *name;
        HandlerFn handler;
    };

    static const CommandEntry COMMAND_TABLE[];


    static int create_listening_socket(int port)
    {
        int listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd < 0)
        {
            perror("socket");
            return -1;
        }

        int opt = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("setsockopt");
            close(listenfd);
            return -1;
        }

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(listenfd, (sockaddr *)&addr, sizeof(addr)) < 0)
        {
            perror("bind");
            close(listenfd);
            return -1;
        }

        if (listen(listenfd, SOMAXCONN) < 0)
        {
            perror("listen");
            close(listenfd);
            return -1;
        }

        return listenfd;
    }


    void send_line(FILE *out, const std::string &line) const
    {
        if (!out)
            return;
        std::string withNL = line + "\n";
        fputs(withNL.c_str(), out);
        fflush(out);
        std::cout << "S -> C: " << line << std::endl;
    }

    void send_error(FILE *out, ErrorCode code) const
    {
        send_line(out, bbp::ErrorMap::toString(code));
    }

    void send_ok(FILE *out, OkCode code = OkCode::Simple) const
    {
        send_line(out, bbp::OkMap::toString(code));
    }

    void send_ok_payload(FILE *out, const std::string &payload) const
    {
        send_line(out, std::string("OK ") + payload);
    }

    static std::string commandRest(const std::string &line, std::size_t prefixLen)
    {
        return trim(line.substr(prefixLen));
    }

    static bool parseId(const std::string &s, int &idOut)
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

    const Item *findItem(int id) const
    {
        auto it = store_.indexById.find(id);
        if (it == store_.indexById.end())
            return nullptr;
        return &store_.items[it->second];
    }

    static std::string formatItemSummary(const Item &item)
    {
        return std::to_string(item.id) + " ;; " +
               itemTypeToString(item.type) + " ;; " +
               item.title;
    }

    static std::string formatItemFull(const Item &item)
    {
        return std::to_string(item.id) + " ;; " +
               itemTypeToString(item.type) + " ;; " +
               item.title + " ;; " + item.body;
    }


    void acceptAndServeOneClient()
    {
        int connfd = accept(listenfd_, nullptr, nullptr);
        if (connfd < 0)
        {
            perror("accept");
            return;
        }

        FILE *in = fdopen(connfd, "r");
        FILE *out = fdopen(dup(connfd), "w");
        if (!in || !out)
        {
            std::fprintf(stderr, "fdopen failed\n");
            close(connfd);
            return;
        }

        char buffer[4096];
        while (std::fgets(buffer, sizeof(buffer), in))
        {
            std::string raw(buffer);
            std::string line = trim(raw);
            if (line.empty())
                continue;

            std::cout << "C -> S: " << line << std::endl;

            dispatchCommand(line, out);
        }

        std::fclose(in);
        std::fclose(out);
    }


    void dispatchCommand(const std::string &line, FILE *out)
    {
        std::istringstream iss(line);
        std::string cmdWord;
        iss >> cmdWord;
        if (cmdWord.empty())
        {
            send_error(out, ErrorCode::EmptyRequest);
            return;
        }

        for (const CommandEntry *entry = COMMAND_TABLE; entry->name != nullptr; ++entry)
        {
            if (cmdWord == entry->name)
            {
                HandlerFn fn = entry->handler;
                (this->*fn)(line, out);
                return;
            }
        }

        send_error(out, ErrorCode::UnknownRequest);
    }


    void handleAdd(const std::string &line, FILE *out)
    {
        std::string rest = commandRest(line, 3);
        std::vector<std::string> parts = splitBy(rest, ";;");
        if (parts.size() != 3)
        {
            send_error(out, ErrorCode::MalformedRequest);
            return;
        }
        std::string typeStr = trim(parts[0]);
        std::string title = trim(parts[1]);
        std::string body = parts[2];

        ItemType type;
        if (!parseItemType(typeStr, type))
        {
            send_error(out, ErrorCode::MalformedRequest);
            return;
        }

        int id = store_.addItem(type, title, body);
        send_ok_payload(out, std::to_string(id));
    }

    void handleGet(const std::string &line, FILE *out)
    {
        std::string rest = commandRest(line, 3);
        int id;
        if (!parseId(rest, id))
        {
            send_error(out, ErrorCode::MalformedId);
            return;
        }

        const Item *item = findItem(id);
        if (!item)
        {
            send_error(out, ErrorCode::NotFound);
            return;
        }

        send_ok_payload(out, formatItemFull(*item));
    }

    void handleList(const std::string &line, FILE *out)
    {
        std::string rest = commandRest(line, 4);
        ItemType type;
        if (!parseItemType(rest, type))
        {
            send_error(out, ErrorCode::MalformedType);
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

    void handleSearch(const std::string &line, FILE *out)
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
                send_error(out, ErrorCode::UnknownType);
                return;
            }

            std::string termLower = toLower(term);
            const auto &bucket = store_.typeBuckets[static_cast<size_t>(type)];

            send_ok(out);
            for (int id : bucket)
            {
                const Item *item = findItem(id);
                if (!item)
                    continue;

                std::string titleLower = toLower(item->title);
                std::string bodyLower = toLower(item->body);

                if (titleLower.find(termLower) != std::string::npos ||
                    bodyLower.find(termLower) != std::string::npos)
                {
                    send_line(out, formatItemSummary(*item));
                }
            }
            send_line(out, ".END");
        }
        else if (mode == "TITLE")
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
            send_ok(out);

            for (const auto &item : store_.items)
            {
                std::string titleLower = toLower(item.title);
                if (titleLower.find(termLower) != std::string::npos)
                {
                    send_line(out, formatItemSummary(item));
                }
            }
            send_line(out, ".END");
        }
        else if (mode == "KEYWORDS")
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

            send_ok(out);
            for (const auto &item : store_.items)
            {
                std::string titleLower = toLower(item.title);
                std::string bodyLower = toLower(item.body);

                bool allMatch = true;
                for (const auto &k : keywords)
                {
                    if (titleLower.find(k) == std::string::npos &&
                        bodyLower.find(k) == std::string::npos)
                    {
                        allMatch = false;
                        break;
                    }
                }

                if (allMatch)
                {
                    send_line(out, formatItemSummary(item));
                }
            }
            send_line(out, ".END");
        }
        else
        {
            send_error(out, ErrorCode::MalformedRequest);
        }
    }

    void handleLink(const std::string &line, FILE *out)
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
            send_error(out, ErrorCode::UnknownId);
            return;
        }

        const Item *fromItem = findItem(fromId);
        const Item *toItem = findItem(toId);
        if (!fromItem || !toItem)
        {
            send_error(out, ErrorCode::NotFound);
            return;
        }

        store_.addLink(fromId, toId);
        send_ok(out);
    }

    void handleContext(const std::string &line, FILE *out)
    {
        std::string rest = commandRest(line, 7);

        int id;
        if (!parseId(rest, id))
        {
            send_error(out, ErrorCode::MalformedRequest);
            return;
        }

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

        auto range = store_.links.equal_range(id);
        for (auto lit = range.first; lit != range.second; ++lit)
        {
            int toId = lit->second;
            const Item *target = findItem(toId);
            if (!target)
                continue;

            send_line(out, formatItemFull(*target));
        }

        send_line(out, ".END");
    }

    void handleOutline(const std::string & /*line*/, FILE *out)
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
        printBucket(ItemType::CHARACT, "CHARACTERS");
        printBucket(ItemType::PLOT, "PLOT");
        printBucket(ItemType::PHIL, "PHILOSOPHIES");
        printBucket(ItemType::QUOTE, "QUOTES");

        send_line(out, ".END");
    }
};

const BbpServer::CommandEntry BbpServer::COMMAND_TABLE[] = {
    {"ADD",     &BbpServer::handleAdd},
    {"GET",     &BbpServer::handleGet},
    {"LIST",    &BbpServer::handleList},
    {"SEARCH",  &BbpServer::handleSearch},
    {"LINK",    &BbpServer::handleLink},
    {"CONTEXT", &BbpServer::handleContext},
    {"OUTLINE", &BbpServer::handleOutline},
    {nullptr,   nullptr}
};

int main(int argc, char **argv)
{
    int port = parse_port_from_args(argc, argv);
    BbpServer server(port);
    server.run();
    return 0;
}
