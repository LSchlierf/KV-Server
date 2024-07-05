#include <cstdlib>
#include <argp.h>
#include <iostream>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

#define CHECK(stmt, msg)                 \
    if ((stmt) < 0)                      \
    {                                    \
        std::cerr << (msg) << std::endl; \
        exit(EXIT_FAILURE);              \
    }

void printHelp(char *progname)
{
    std::cout << "Usage: " << progname << " -p <port> -i <key>:<value> -g <key> -r <key>" << std::endl;
    std::cout << "  -i <key>:<value> | insert new key:value pair into hash table" << std::endl;
    std::cout << "  -g <key>         | get value from hash table by key" << std::endl;
    std::cout << "  -r <key>         | remove key from hash table" << std::endl;
}

enum TYPE
{
    INSERT = 0,
    GET = 1,
    REMOVE = 2
};

typedef struct
{
    enum TYPE type;
    std::string arg;
} query;

bool verifyInsert(std::string query);
bool verifyGet(std::string query);
bool verifyRemove(std::string query);

int main(int argc, char *argv[])
{
    std::vector<query> queries;
    int opt;
    int port = 8080;
    while ((opt = getopt(argc, argv, "p:i:g:r:h")) != -1)
    {
        query q;
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;

        case 'h':
            printHelp(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case 'i':
            q.arg = optarg;
            q.type = INSERT;
            queries.push_back(q);
            break;

        case 'g':
            q.arg = optarg;
            q.type = GET;
            queries.push_back(q);
            break;

        case 'r':
            q.arg = optarg;
            q.type = REMOVE;
            queries.push_back(q);
            break;

        default:
            printHelp(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    std::cout << "Using port " << port << std::endl;

    bool (*verify[])(std::string) = {
        verifyInsert,
        verifyGet,
        verifyRemove};

    std::string prefix[] = { "I:",
                             "G:",
                             "R:" };

    for (query q : queries)
    {
        if (!verify[q.type](q.arg))
        {
            continue;
        }
        int clientSocket;
        CHECK(clientSocket = socket(AF_INET, SOCK_STREAM, 0), "socket() failed")

        sockaddr_in serverAddres;
        serverAddres.sin_family = AF_INET;
        serverAddres.sin_port = htons(port);
        serverAddres.sin_addr.s_addr = INADDR_ANY;
        CHECK(connect(clientSocket, (struct sockaddr *)&serverAddres, sizeof(serverAddres)), "connect() failed")

        std::string req = prefix[q.type] + q.arg;
        CHECK(send(clientSocket, req.c_str(), strlen(req.c_str()), 0), "send() failed")

        char buf[1024];
        CHECK(recv(clientSocket, buf, sizeof(buf), 0), "recv() failed")

        std::cout << "  " << buf << std::endl;

        CHECK(close(clientSocket), "close() failed")
    }
    return EXIT_SUCCESS;
}

bool verifyInsert(std::string query)
{
    std::cout << "INSERT " << query << std::endl;
    size_t found = query.find(":");
    if (found == std::string::npos || query.find(":", found + 1) != std::string::npos)
    {
        std::cout << "  invalid argument" << std::endl;
        return false;
    }
    return true;
}

bool verifyGet(std::string query)
{
    std::cout << "GET " << query << std::endl;
    size_t found = query.find(":");
    if (found != std::string::npos)
    {
        std::cout << "  invalid argument" << std::endl;
        return false;
    }
    return true;
}

bool verifyRemove(std::string query)
{
    std::cout << "REMOVE " << query << std::endl;
    size_t found = query.find(":");
    if (found != std::string::npos)
    {
        std::cout << "  invalid argument" << std::endl;
        return false;
    }
    return true;
}
