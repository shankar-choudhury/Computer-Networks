// proj5.cpp
// Shankar Choudhury (sxc1782)
// BBP client - proj5 (interactive CLI)

#include "bbp.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>


static void parse_args(int argc, char **argv, std::string &host, int &port)
{
    host.clear();
    port = 0;
    for (int i = 1; i < argc - 1; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-h")
        {
            host = argv[i + 1];
        }
        else if (arg == "-p")
        {
            port = std::atoi(argv[i + 1]);
        }
    }
    if (host.empty() || port <= 0)
    {
        std::fprintf(stderr, "Usage: %s -h <hostname> -p <port>\n", argv[0]);
        std::exit(1);
    }
}


class BbpClient
{
public:
    BbpClient(const std::string &host, int port)
        : sockfd_(-1), in_(nullptr), out_(nullptr)
    {
        connectToServer(host, port);
    }

    ~BbpClient()
    {
        if (in_)
            std::fclose(in_);
        if (out_)
            std::fclose(out_);
    }

    void runInteractive()
    {
        printBanner();

        std::string input;
        while (true)
        {
            std::cout << "C: ";
            if (!std::getline(std::cin, input))
            {
                break;
            }
            std::string trimmed = trim(input);
            if (trimmed.empty())
                continue;

            if (!sendLine(trimmed))
            {
                std::perror("write");
                break;
            }

            bool multi = isMultiLineCommand(trimmed);
            if (!multi)
            {
                if (!readSingleResponse())
                {
                    std::cout << "S: <connection closed>" << std::endl;
                    break;
                }
            }
            else
            {
                if (!readMultiResponse())
                {
                    std::cout << "S: <connection closed>" << std::endl;
                    break;
                }
            }
        }
    }

private:
    int sockfd_;
    FILE *in_;
    FILE *out_;

    void connectToServer(const std::string &host, int port)
    {
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo *res = nullptr;
        int err = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
        if (err != 0)
        {
            std::fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
            std::exit(1);
        }

        sockfd_ = -1;
        for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next)
        {
            sockfd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sockfd_ == -1)
                continue;
            if (connect(sockfd_, rp->ai_addr, rp->ai_addrlen) == 0)
            {
                break;
            }
            close(sockfd_);
            sockfd_ = -1;
        }
        freeaddrinfo(res);

        if (sockfd_ == -1)
        {
            std::fprintf(stderr, "Could not connect to %s:%d\n", host.c_str(), port);
            std::exit(1);
        }

        in_ = fdopen(sockfd_, "r");
        out_ = fdopen(dup(sockfd_), "w");
        if (!in_ || !out_)
        {
            std::fprintf(stderr, "fdopen failed\n");
            if (in_) std::fclose(in_);
            if (out_) std::fclose(out_);
            close(sockfd_);
            std::exit(1);
        }
    }


    static void printBanner()
    {
        std::cout << "Starting Client" << std::endl;
        std::cout << "Welcome! You are connected to the Book Builder Protocol (BBP) server.\n"
                  << "Enter commands like:\n"
                  << "  ADD QUOTE;;title;;body\n"
                  << "  GET 1\n"
                  << "  LIST PLOT\n"
                  << "  SEARCH TYPE PLOT hero\n"
                  << "  SEARCH TITLE redemption\n"
                  << "  SEARCH KEYWORDS modernity failure\n"
                  << "  LINK 1 4\n"
                  << "  CONTEXT 1\n"
                  << "  OUTLINE\n"
                  << "Type Ctrl-D (EOF) to exit.\n";
    }

    static bool isMultiLineCommand(const std::string &input)
    {
        std::string t = trim(input);
        if (startsWith(t, "LIST "))
            return true;
        if (startsWith(t, "SEARCH "))
            return true;
        if (startsWith(t, "CONTEXT "))
            return true;
        if (t == "OUTLINE")
            return true;
        return false;
    }

    bool sendLine(const std::string &line)
    {
        std::string toSend = line + "\n";
        if (std::fputs(toSend.c_str(), out_) == EOF)
        {
            return false;
        }
        std::fflush(out_);
        return true;
    }

    bool readSingleResponse()
    {
        char buf[4096];
        if (std::fgets(buf, sizeof(buf), in_) == nullptr)
        {
            return false;
        }
        std::string line = trim(std::string(buf));
        std::cout << "S: " << line << std::endl;
        return true;
    }

    bool readMultiResponse()
    {
        char buf[4096];
        while (true)
        {
            if (std::fgets(buf, sizeof(buf), in_) == nullptr)
            {
                return false;
            }
            std::string line = trim(std::string(buf));
            std::cout << "S: " << line << std::endl;

            if (line == ".END" || startsWith(line, "ERR"))
                break;
        }
        return true;
    }
};


int main(int argc, char **argv)
{
    std::string host;
    int port = 0;
    parse_args(argc, argv, host, port);

    BbpClient client(host, port);
    client.runInteractive();

    return 0;
}
