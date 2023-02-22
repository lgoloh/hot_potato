#include <vector>
#include <sys/socket.h>
#include <netdb.h>

#include "player.hpp"

using std::vector;
using std::string;
#define MAX_CONN 3

int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cerr << "Usage ./player <machine_name> <port_num>\n";
        return EXIT_FAILURE;
    }
    int player_fd;
    int status;
    const char * master_name = argv[1];
    const char * port = argv[2];
    const char * hostname = NULL;
    struct addrinfo playeraddr;
    struct addrinfo *playeraddr_list;
    struct addrinfo masteraddr;
    struct addrinfo *masteraddr_list;
    vector<Player> neigbours;
    int max_fd;
    struct timeval timeout;

    //1.Open a socket to listen for incoming player connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    memset(&playeraddr, 0, sizeof(playeraddr));
    playeraddr.ai_family = AF_UNSPEC;
    playeraddr.ai_socktype = SOCK_STREAM;
    playeraddr.ai_flags = AI_PASSIVE;
    status = getaddrinfo(hostname, port, &playeraddr, &playeraddr_list);
    errorListener("getaddrinfo", status);

    player_fd = socket(playeraddr_list->ai_family, 
                    playeraddr_list->ai_socktype, 
                    playeraddr_list->ai_protocol);
    errorListener("create socket", player_fd);
    int reuse = 1;
    status = setsockopt(player_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    errorListener("set socket options", status);
    socklen_t sz = sizeof(INADDR_ANY);
    status = bind(player_fd, INADDR_ANY, sz);   //player binds to any address
    errorListener("bind to socket", status);


    //2.Connect listener to master to get neighbor info; get address of master_name
    memset(&masteraddr, 0, sizeof(masteraddr));
    masteraddr.ai_family = AF_UNSPEC;
    masteraddr.ai_socktype = SOCK_STREAM;
    status = getaddrinfo(master_name, port, &masteraddr, &masteraddr_list);
    status = connect(player_fd, masteraddr_list->ai_addr, masteraddr_list->ai_addrlen);

    const char *message = "connecting to ringmaster!";
    send(player_fd, message, strlen(message), 0);


    //2.Start listening on the socket; create a list of N socket connections
        //-use select() to check the queue for an incoming connection
        //-accept() the connection to create a new connection socket for the player
        //-create a Player object for the new connection
        //-add Player obj to the list of player sockets 
    /* status = listen(player_fd, MAX_CONN);
    errorListener("listening to socket", status);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(master_fd, &readfds);
    max_fd = master_fd;

    for (int i = 0; i < num_players; i++) {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        status = select(max_fd+1, &readfds, &writefds, NULL, &timeout);
        errorListener("select socket connection", status);
         //adds a new connection to the list of players
         //reading from the master socket means there's a new socket connection to it; 
         if (FD_ISSET(master_fd, &readfds)) {
            listenForNewPlayers(master_fd, i, &max_fd, num_players); 
         }
         updateSocketSets(&readfds, master_fd);
    } */
    return EXIT_SUCCESS;
    
}