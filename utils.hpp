#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <cstring>
#include <cstdlib>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <netdb.h>

#include "potato.hpp"
#define END_GAME "end_game"

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

/*
 * Code citation: Beej's Guide to Network Programming
 * Get port number given socket descriptor
 */
int getPort(int sock_fd) {
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    if (getsockname(sock_fd, (struct sockaddr *)&sin, &len) == -1) {
        std::cerr << "Error: error getting port for player socket\n";
        exit(EXIT_FAILURE);
    }
    return ntohs(sin.sin_port);
}

/*
 * Create server socket  
 */
int setupServer(struct addrinfo * hostaddress, 
                struct addrinfo ** addr_list, 
                const char * hostname,
                const char * port) {

    memset(hostaddress, 0, sizeof(*hostaddress));

    hostaddress->ai_family = AF_INET;
    hostaddress->ai_socktype = SOCK_STREAM;
    hostaddress->ai_flags = AI_PASSIVE;

    int status = getaddrinfo(hostname, port, hostaddress, addr_list);
    errorListener("getaddrinfo", status);
    struct sockaddr_in * addr_in = (struct sockaddr_in *)((*addr_list)->ai_addr);
    socklen_t addr_size = sizeof(addr_in);

    if (port == "") {
        addr_in->sin_port = 0;
    }

    int server_fd = socket((*addr_list)->ai_family, 
                    (*addr_list)->ai_socktype, 
                    (*addr_list)->ai_protocol);
    errorListener("create socket", server_fd);

    int reuse = 1;
    status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    errorListener("set socket options", status);

    status = bind(server_fd, (struct sockaddr *)addr_in, (*addr_list)->ai_addrlen);
    errorListener("binding to socket", status);

    return server_fd;

}

void printAddrList(struct addrinfo * list) {
    struct addrinfo * curr = list;
    for (; curr != NULL; curr= curr->ai_next) {
        std::cout << "curr address: " << curr->ai_addr << std::endl;
        std::cout << "address family: " << curr->ai_family << std::endl;
        std::cout << "address protocol: " << curr->ai_protocol << std::endl;
    }
}

void tokenize(string & input, string & delim, vector<string> & tokens, int start_indx) {
    if (start_indx >= input.size()) {
        return;
    }
    size_t delim_pos = input.find(delim, start_indx);
    if (delim_pos == string::npos) {
        tokens.push_back(input.substr(start_indx));
        return;
    } else {
        tokens.push_back(input.substr(start_indx, delim_pos-start_indx));
        start_indx = delim_pos + 1;
        tokenize(input, delim, tokens, start_indx);
    }
}

vector<string> parse(char * data, const char * delim) {
    vector<string> tokens;
    string input = data;
    string d = delim;
    tokenize(input, d, tokens, 0);
    return tokens;
    
}

#endif