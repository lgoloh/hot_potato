#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <netdb.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

using std::string;

class Player {
    private:
        int player_id;
        int socket;
        struct sockaddr sock_addr;
    public:
        Player * next;
        Player * prev;

        Player(int id, int sock_fd, struct sockaddr addr): player_id(id), socket(sock_fd), sock_addr(addr), next(NULL), prev(NULL) {};
        int getPlayerId() { return player_id; }
        int getSocket() { return socket; }
        struct sockaddr getAddress() { return sock_addr;}
};

int errorListener(string funcName, int status) {
    if (funcName == "getaddrinfo") {
        if (status != 0) {
            std::cerr << "Error: cannot get address info for host" << std::endl;
            return EXIT_FAILURE;
        }
    } else if  (status == -1) {
        std::cerr << "Error: cannot " << funcName << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#endif
