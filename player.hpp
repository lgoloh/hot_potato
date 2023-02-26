#ifndef __PLAYER_HPP__
#define __PLAYER_HPP__

#include <netdb.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <sstream>

using std::string;

class Player {
    private:
        int player_id;
        int socket;
        char * player_host;
        char * player_port;
        void copyToBuffer(char * buffer, const char * str, int * index) {
            for (size_t i = 0; i < strlen(str); i++) {
                buffer[*index] = str[i];
                (*index)++;
            }
        }
    public:
        Player * next;
        Player * prev;

        Player(int id, int sock_fd, char * host): player_id(id), socket(sock_fd), player_host(host), player_port(NULL), next(NULL), prev(NULL) {};
        int getPlayerId() { return player_id; }

        int getSocket() { return socket; }

        char * getHost() { return player_host;}

        char * getPort() { return player_port;}

        void setHost(char * host) { player_host = host;}

        void setPort(char * port) { player_port = port;}

        char * getPlayerData() {
            const char * id = std::to_string(player_id).c_str();
            char data[100];
            int index = 0;
            copyToBuffer(data, id, &index);
            data[index] = ':';
            index++;
            copyToBuffer(data, player_host, &index);
            data[index] = ':';
            index++;
            copyToBuffer(data, player_port, &index);
            data[index] = 0;
            return strdup(data);
        }
        
        void toStr() {
            std::stringstream playerStr;
            playerStr << "Player { \n";
            playerStr << "id: " << player_id << ",\n";
            playerStr << "host: " << player_host << ",\n";
            playerStr << "port: " << player_port << ",\n";
            if (next != NULL) {
                playerStr << "right: " << "Player " << next->getPlayerId() << ",\n";
            } else {
                playerStr << "right: " << "Player " << next << ",\n";
            }
            if (prev != NULL) {
                playerStr << "left: " << "Player " << prev->getPlayerId() << "\n";
            } else {
                playerStr << "left: " << "Player " << prev << "\n"; 
            }
            playerStr << "}\n";
            std::cout << playerStr.str();
            std::cout << "Printing player data: " << getPlayerData() << std::endl;
        }
};

#endif
