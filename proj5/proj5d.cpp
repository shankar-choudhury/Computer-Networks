// proj5d.cpp
// Shankar Choudhury (sxc1782)
// BBP server (Book Builder Protocol) - proj5d

#include "bbp.hpp"
#include "bbp_status.hpp"
#include "bbp_commands.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
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
        : listenfd_(-1),
          store_(),
          processor_(store_)
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
        for (;;)
        {
            acceptAndServeOneClient();
        }
    }

private:
    int listenfd_;
    ItemStore store_;
    BbpCommandProcessor processor_;

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

            processor_.handleLine(line, out);
            std::fflush(out);
        }

        std::fclose(in);
        std::fclose(out);
    }
};

int main(int argc, char **argv)
{
    int port = parse_port_from_args(argc, argv);
    BbpServer server(port);
    server.run();
    return 0;
}
