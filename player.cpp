#include <vector>
#include <sys/socket.h>
#include <netdb.h>

#include "player.hpp"
#include "utils.hpp"

using std::vector;
using std::string;
#define MAX_CONN 2

fd_set readfds;
fd_set writefds;
int player_sock;
int ringmaster_sock;
int max_fd;
int curr_sockfd;
string player_port;
vector<int> neighbours[3];


/*
 * Accepts a new connection and adds the connection socket to the list
 */
void handleNewConnection() {
    struct sockaddr neighbor_addr;
    socklen_t sz = sizeof(neighbor_addr);

    int conn = accept(player_sock, &neighbor_addr, &sz);
    if (conn == -1) {
        if (neighbours->size() == 3) {
            perror("Error: max number of players reached");
        } else {
            perror("Error: cannot accept new player");
        }
        exit(EXIT_FAILURE);
    }
    neighbours->push_back(conn);
    max_fd = std::max(max_fd, conn);
}

/*
 * Generates a unique port for each player
 */
void setPlayerPort() {
    srand((unsigned int)time(NULL));
    int port_num = rand() % 8000 + 1001;
    player_port = std::to_string(port_num);
}

/*
 * Creates a socket to connect with the ringmaster
 */
int getRingMasterSocket(int protocol) {
    return socket(AF_INET, SOCK_STREAM, protocol);
}

/*
 * Connects the player to the ringmaster process
 */
void connectToRingmaster(const char * master_name, const char * master_port) {
    struct addrinfo masteraddr;
    struct addrinfo *masteraddr_list;

    memset(&masteraddr, 0, sizeof(masteraddr));
    masteraddr.ai_family = AF_UNSPEC;
    masteraddr.ai_socktype = SOCK_STREAM;
    
    int status = getaddrinfo(master_name, master_port, &masteraddr, &masteraddr_list);
    ringmaster_sock = getRingMasterSocket(masteraddr_list->ai_protocol);
    status = connect(ringmaster_sock, masteraddr_list->ai_addr, masteraddr_list->ai_addrlen);
    errorListener("Error connecting to ringmaster", status);
    neighbours->push_back(ringmaster_sock);
    
    //send port number to ringmaster 
    const char * port = player_port.c_str();
    std::cout << "port length: " << strlen(port) << std::endl;
    send(ringmaster_sock, port, strlen(port), 0);
}

/**
 * Build the select lists and update the max socket descriptor
*/
void buildSelectLists() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    max_fd = neighbours->at(0);
    FD_SET(max_fd, &readfds);
    FD_SET(max_fd, &writefds);
    for (int i = 1; i < neighbours->size(); i++) {
        max_fd = std::max(max_fd, neighbours->at(i));
        FD_SET(neighbours->at(i), &readfds);
        FD_SET(neighbours->at(i), &writefds);
    }
}

/**
 * Sets up socket connections for all player processes
*/
void connectToNeighbours() {
    for (int i = 0; i < MAX_CONN; i++) {
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL); //listens for a connection; blocking call
        errorListener("select socket connection", status);

         //reading from the player socket means there's either:
          //-a new connection request to it
          //-incoming data is coming to it from a socket
         if (FD_ISSET(player_sock, &readfds)) {
            handleNewConnection(); 

            char buffer[2000];
            recv(curr_sockfd, buffer, 100, 0);
            buffer[99] = 0;

            std::cout << "Ringmaster received: " << buffer << " from Player with socket " << curr_sockfd << std::endl;
         }
         /* FD_ZERO(&readfds);
         FD_SET(master_fd, &readfds); */
         buildSelectLists();
    }
}

void getNeighborInfoFromRingMaster() {
    FD_ZERO(&readfds);
    FD_SET(ringmaster_sock, &readfds);
    max_fd = ringmaster_sock;

    for (int i = 0; i < MAX_CONN; i++) {
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL);
        errorListener("select socket connection", status);

        if (FD_ISSET(ringmaster_sock, &readfds)) {
            char buffer[200];
            recv(ringmaster_sock, buffer, 100, 0);
            buffer[48] = 0;

            std::cout << "Player received: " << buffer << " from ringmaster\n";
        }
    }
}

/*
 * Connect to neigbours
 */
void listenToSockets() {

}


int main(int argc, char ** argv) {
    if (argc < 3) {
        std::cerr << "Usage ./player <machine_name> <port_num>\n";
        return EXIT_FAILURE;
    }
    int status;
    const char * master_name = argv[1];
    const char * port = argv[2];
    const char * hostname = NULL;
    struct addrinfo playeraddr;
    struct addrinfo *playeraddr_list;
    vector<Player> neigbours;
    int max_fd;

    //1.Open a socket to listen for incoming neigbour connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    memset(&playeraddr, 0, sizeof(playeraddr));
    setAddressOptions(&playeraddr);
    setPlayerPort();

    std::cout << "player port: " << player_port.c_str() << std::endl;
    status = getaddrinfo(hostname, player_port.c_str(), &playeraddr, &playeraddr_list);
    errorListener("getaddrinfo", status);

    player_sock = socket(playeraddr_list->ai_family, 
                    playeraddr_list->ai_socktype, 
                    playeraddr_list->ai_protocol);
    errorListener("create socket", player_sock);
    
    int reuse = 1;
    status = setsockopt(player_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    errorListener("set socket options", status);

    status = bind(player_sock, playeraddr_list->ai_addr, playeraddr_list->ai_addrlen);   //player binds to any address
    perror("Error binding to player socket\n");
    errorListener("bind to socket", status);

    //2.Connect listener to ringmaster
    connectToRingmaster(master_name, port);

    //3. Listen to ringmaster connection to get neighbor info: need ringmaster socket descriptor?
        //- get player address and port from ringmaster
        //- connect() to neigbors with player addresses
        //- use select() to listen for the potato object
    status = listen(player_sock, MAX_CONN);
    perror("listen failed");
    buildSelectLists();
    getNeighborInfoFromRingMaster();

    while(true) {

        listenToSockets();
    }
    
    return EXIT_SUCCESS;
    
}