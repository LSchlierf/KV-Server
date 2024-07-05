#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <argp.h>
#include <cstdlib>
#include <optional>
#include <pthread.h>

#include "lib/Hashtable.cpp"

#define CHECK(stmt, msg)                                 \
    if ((stmt) < 0)                                      \
    {                                                    \
        std::cerr << (msg) << " " << errno << std::endl; \
        exit(EXIT_FAILURE);                              \
    }

void *handleConnection(void * args);

bool verifyInsert(std::string query);
bool verifyGet(std::string query);
bool verifyRemove(std::string query);

void printHelp(char *progname)
{
    std::cout << "Usage: " << progname << " -p <port> -b <buckets>" << std::endl;
}

struct arg {
    int socket;
    Hashtable<std::string, std::string> *ht;
};

int main(int argc, char *argv[])
{
    int opt;
    int port = 8080;
    int buckets = 32;
    while ((opt = getopt(argc, argv, "p:b:h")) != -1)
    {
        switch (opt)
        {
        case 'p':
            port = atoi(optarg);
            break;

        case 'b':
            buckets = atoi(optarg);
            break;

        case 'h':
            printHelp(argv[0]);
            exit(EXIT_SUCCESS);

        default:
            printHelp(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    std::cout << "Using port " << port << " and " << buckets << " buckets." << std::endl;

    Hashtable<std::string, std::string> table(buckets);

    int serverSocket;
    CHECK(serverSocket = socket(AF_INET, SOCK_STREAM, 0), "socket() failed")

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    CHECK(bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)), "bind() failed")

    CHECK(listen(serverSocket, 16), "listen() failed")

    while (1)
    {
        int clientSocket;
        CHECK(clientSocket = accept(serverSocket, NULL, NULL), "accept() failed")

        pthread_t thread_id;

        struct arg args;
        args.ht = &table;
        args.socket = clientSocket;

        int ret;

        if((ret = pthread_create(&thread_id, nullptr, handleConnection, (void*)(&args))) != 0) {
            std::cout << "pthread_create() failed " << ret << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

void* handleConnection(void * args)
{
    struct arg a = *(struct arg *) args;
    int socket = a.socket;
    Hashtable<std::string, std::string> *ht = a.ht;
    std::cout << "Got request" << std::endl;
    char buffer[1024] = {0};
    int rec;
    CHECK(rec = recv(socket, buffer, sizeof(buffer), 0), "recv() failed")
    char suc[] = "Success";
    char err[] = "Failure";
    std::string key;
    std::string val;
    std::optional<std::string> result;
    switch (buffer[0])
    {
    case 'I':
        std::cout << "  INSERT " << buffer + 2 << std::endl;
        if (!verifyInsert(buffer))
        {
            std::cout << "    invalid argument" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }
        key = buffer + 2;
        val = key.substr(key.find(':') + 1);
        key = key.substr(0, key.find(':'));
        if (!ht->insert(key, val))
        {
            std::cout << "    key already present" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }
        CHECK(send(socket, suc, sizeof(suc), 0), "send() failed")
        break;

    case 'G':
        std::cout << "  GET " << buffer + 2 << std::endl;
        if (!verifyGet(buffer))
        {
            std::cout << "    invalid argument" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }
        key = buffer + 2;
        result = ht->get(key);
        if(!result.has_value()) {
            std::cout << "    not found" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }
        CHECK(send(socket, result.value().c_str(), sizeof(result.value().c_str()), 0), "send() failed")
        break;

    case 'R':
        std::cout << "  REMOVE " << buffer + 2 << std::endl;
        if (!verifyRemove(buffer))
        {
            std::cout << "    invalid argument" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }
        key = buffer + 2;
        if(!ht->remove(key)) {
            std::cout << "    key not present" << std::endl;
            CHECK(send(socket, err, sizeof(err), 0), "send() failed")
            return nullptr;
        }

        CHECK(send(socket, suc, sizeof(suc), 0), "send() failed")
        break;

    default:
        std::cout << "  invalid request" << std::endl;
        CHECK(send(socket, err, sizeof(err), 0), "send() failed")
        break;
    }
    return nullptr;
}

bool verifyInsert(std::string query)
{
    int counter = 0;
    size_t found = -1;
    while (true)
    {
        found = query.find(':', found + 1);
        if (found == std::string::npos)
        {
            break;
        }
        counter++;
    }
    return counter == 2;
}

bool verifyGet(std::string query)
{
    int counter = 0;
    size_t found = -1;
    while (true)
    {
        found = query.find(':', found + 1);
        if (found == std::string::npos)
        {
            break;
        }
        counter++;
    }
    return counter == 1;
}

bool verifyRemove(std::string query)
{
    int counter = 0;
    size_t found = -1;
    while (true)
    {
        found = query.find(':', found + 1);
        if (found == std::string::npos)
        {
            break;
        }
        counter++;
    }
    return counter == 1;
}