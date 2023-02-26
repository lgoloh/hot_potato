#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <cstring>
#include <cstdlib>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>

using std::string;

int errorListener(string message, int status) {
    if (message == "getaddrinfo") {
        if (status != 0) {
            perror("Error: cannot get address info for host");
            exit(EXIT_FAILURE);
        }
    } else if  (status == -1) {
        perror(message.c_str());
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

void printAddrList(struct addrinfo * list) {
    struct addrinfo * curr = list;
    for (; curr != NULL; curr= curr->ai_next) {
        std::cout << "curr address: " << curr->ai_addr << std::endl;
        std::cout << "address family: " << curr->ai_family << std::endl;
        std::cout << "address protocol: " << curr->ai_protocol << std::endl;
    }
}

void setAddressOptions(struct addrinfo * hostaddress) {
    hostaddress->ai_family = AF_INET;
    hostaddress->ai_socktype = SOCK_STREAM;
    hostaddress->ai_flags = AI_PASSIVE;
}

#endif