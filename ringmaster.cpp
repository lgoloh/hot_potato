#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "player.hpp"
#include "potato.hpp"
#include "utils.hpp"

using std::vector;
using std::string;

Player * head = NULL;
Player * curr_player = NULL;
fd_set readfds;
fd_set writefds;


/**
 * Inserts a new player object into the linked list of player objects
*/
void insertNewPlayer(Player * player) {
    Player * curr = head;
    while (true) {
        if (curr->getPlayerId() == player->getPlayerId() - 1) {
            curr->next = player;
            player->prev = curr;
            break;
        }
        curr = curr->next;
    }
}

/**
 * Accepts a new player connection and creates a Player object. 
 * It attempts to send info to the player, and inserts the player into the Player list
 * inet_ntoa(player_addr->sin_addr)
*/
void listenForNewPlayers(int sock_fd, int id, int * max_fd, int num_players) {
    struct sockaddr player_addr;
    socklen_t addr_sz = sizeof(player_addr);
    
    int player_fd = accept(sock_fd, &player_addr, &addr_sz);
    struct sockaddr_in * in_addr = (struct sockaddr_in *)&player_addr;

    std::cout << "sock address: " << inet_ntoa(in_addr->sin_addr) << std::endl;
    *max_fd = std::max(*max_fd, player_fd);
    Player * n_player = new Player(id, player_fd, inet_ntoa(in_addr->sin_addr));
    if (head == NULL) {
        head = n_player;
    } else {
        insertNewPlayer(n_player);
    }

    //At the end of the list, close the circle
    id = id + 1;
    if (id == num_players) {
        n_player->next = head;
        head->prev = n_player;
    }
    curr_player = n_player;
}

/**
 * Prints the Player objects
*/
void printPlayers() {
    std::cout << "printing current players\n"; 
    Player * curr = head;
    if (curr == NULL) {
        return;
    }
    do {
        curr->toStr();
        curr = curr->next;
    } while(curr->getPlayerId() != 0);
}

/**
 * Adds all player socket descriptors to the specified fd_set
*/
void buildSelectLists(fd_set * set, int master_fd, int * max_fd) {
    FD_ZERO(&(*set));
    FD_SET(master_fd, &(*set));
    *max_fd = master_fd;
    Player * curr = head; //head should always be Player0
    if (curr == NULL) {
        return;
    }
    do {
        
        FD_SET(curr->getSocket(), &(*set));
        *max_fd = std::max(*max_fd, curr->getSocket());
        curr = curr->next;
    } while (curr->getPlayerId() != 0);
}

/**
 * Sets up socket connections for all player processes
*/
void setUpPlayerRing(int master_fd, int max_fd, int num_players) {
    for (int i = 0; i < num_players; i++) {
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL); //listens for a connection; blocking call
        errorListener("select socket connection", status);
         //adds a new connection to the list of players
         //reading from the master socket means there's a new socket connection to it; 
         if (FD_ISSET(master_fd, &readfds)) {
            listenForNewPlayers(master_fd, i, &max_fd, num_players); 

            char buffer[50];
            recv(curr_player->getSocket(), buffer, 5, 0);
            buffer[4] = 0;

            char * port = strdup(buffer);
            curr_player->setPort(port);  //set player port

            std::cout << "Ringmaster received: " << buffer << " from Player " << curr_player->getPlayerId() << std::endl;
         }
         FD_ZERO(&readfds);
         FD_SET(master_fd, &readfds);
    }
}

/*
 * Send neighbour info to players and listen for confirmation
 */
void sendNeighbourInfoToPlayers(int master_fd, int * max_fd) {
    //Data to send: neightbour ids, host, & port num
        //data sent as a char * and received in a char [].
        //data format: char *, <player_id>:<player_host>:<player_port>
        //Process:
            //-build the select lists 
            //-iterate through player list
                //-if player socket is in writefds: use a loop to send data of neighbours
    buildSelectLists(&writefds, master_fd, max_fd);
    int status = select(*max_fd+1, NULL, &writefds, NULL, NULL);
    errorListener("select socket connection", status);

    Player * curr = head;
    if (curr == NULL) {
        return;
    }
    do {
        if (FD_ISSET(curr->getSocket(), &writefds)) {
            char * r_neighbour = curr->next->getPlayerData();
            char * l_neighbour = curr->prev->getPlayerData();
            std::cout << "r_neighbour: " << r_neighbour << std::endl;
            ssize_t res = send(curr->getSocket(), r_neighbour, strlen(r_neighbour), 0);
            errorListener("Error sending neighbour data", res);

            std::cout << "l_neighbour: " << l_neighbour << std::endl;
            res = send(curr->getSocket(), l_neighbour, strlen(l_neighbour), 0);
            errorListener("Error sending neighbour data", res);
        }
        curr = curr->next;
    } while(curr->getPlayerId() != 0);
}

int main(int argc, char ** argv) {
    if (argc < 4) {
        std::cerr << "Usage ./ringmaster <port_num> <num_players> <num_hops>\n";
        return EXIT_FAILURE;
    }
    int master_fd;
    int status;
    const char * port = argv[1];
    const char * hostname = NULL;
    struct addrinfo hostaddr;
    struct addrinfo *hostaddr_list;
    int num_players = atoi(argv[2]);
    int num_hops = atoi(argv[3]);
    int max_fd;

    //1.Open a socket to listen for incoming player connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    memset(&hostaddr, 0, sizeof(hostaddr));
    setAddressOptions(&hostaddr);

    status = getaddrinfo(hostname, port, &hostaddr, &hostaddr_list);
    errorListener("getaddrinfo", status);

    master_fd = socket(hostaddr_list->ai_family, 
                    hostaddr_list->ai_socktype, 
                    hostaddr_list->ai_protocol);
    errorListener("create socket", master_fd);

    int reuse = 1;
    status = setsockopt(master_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    errorListener("set socket options", status);

    status = bind(master_fd, hostaddr_list->ai_addr, hostaddr_list->ai_addrlen);
    perror("Error binding to socket");
    errorListener("bind to socket", status);

    //std::cout << "master address: " << hostaddr_list->ai_addr << std::endl;
    //2.Set up ring of player processes create a list of N socket connections
        //-use select() to check the queue for an incoming connection
        //-accept() the connection to create a new connection socket for the player
        //-create a Player object for the new connection
        //-add Player obj to the list of player sockets 
    //std::cout << "Ringmaster socket: " << master_fd << std::endl;
    status = listen(master_fd, num_players);
    perror("ringmaster listen failure?");
    errorListener("listening to socket", status);
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(master_fd, &readfds);
    max_fd = master_fd;
    std::cout << "Waiting for connection on port " << port << std::endl;

    setUpPlayerRing(master_fd, max_fd, num_players);
    printPlayers();

    sendNeighbourInfoToPlayers(master_fd, &max_fd);
    
        
    
    //3.Start the game while the num of hops is > 0
        //-select a random player socket to pass the potato to
                //generate random number using rand(): seeding the generator: srand((unsigned int)time(NULL)+player_id); getting the rand number: int random = rand() % N;
                //connect to the IP address of the player socket (i.e player.sock_addr)
    
}