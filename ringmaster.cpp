#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include "player.hpp"
#include "potato.hpp"

using std::vector;
using std::string;

Player * head = NULL;
Player * curr_player = NULL;
fd_set readfds;
fd_set writefds;


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

void listenForNewPlayers(int sock_fd, int id, int * max_fd, int num_players) {
    struct sockaddr player_addr;
    socklen_t addr_sz = sizeof(player_addr);
    int player_fd = accept(sock_fd, &player_addr, &addr_sz);
    if (player_fd > *max_fd) {
        *max_fd = player_fd;
    };
    Player * n_player = new Player(id, player_fd, player_addr);
    if (head == NULL) {
        head = n_player;
    } else {
        insertNewPlayer(n_player);
    }
    id = id + 1;
    //At the end of the list, close the circle
    if (id == num_players) {
        n_player->next = head;
        head->prev = n_player;
    }
    curr_player = n_player;
}

void updateSocketSets(fd_set * set, int master_fd) {
    FD_ZERO(&(*set));
    FD_SET(master_fd, &(*set));
    Player * curr = head; //head should always be Player0
    do {
        FD_SET(curr->getSocket(), &(*set));
        curr = curr->next;
    } while (curr != NULL && curr->getPlayerId() != 0);
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
    vector<Player> players;
    int num_players = atoi(argv[2]);
    int num_hops = atoi(argv[3]);
    int max_fd;
    struct timeval timeout;
    int player_id = 0;

    //1.Open a socket to listen for incoming player connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    memset(&hostaddr, 0, sizeof(hostaddr));
    hostaddr.ai_family = AF_UNSPEC;
    hostaddr.ai_socktype = SOCK_STREAM;
    hostaddr.ai_flags = AI_PASSIVE;
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
    errorListener("bind to socket", status);
    //2.Start listening on the socket; create a list of N socket connections
        //-use select() to check the queue for an incoming connection
        //-accept() the connection to create a new connection socket for the player
        //-create a Player object for the new connection
        //-add Player obj to the list of player sockets 
    status = listen(master_fd, num_players);
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
            char buffer[512];
            recv(curr_player->getSocket(), buffer, 9, 0);
            buffer[9] = 0;

            std::cout << "Ringmaster received: " << buffer << "from Player" << curr_player->getPlayerId() << std::endl;
         }
         updateSocketSets(&readfds, master_fd);
    }



    //3.Start the game while the num of hops is > 0
        //-select a random player socket to pass the potato to
                //generate random number using rand(): seeding the generator: srand((unsigned int)time(NULL)+player_id); getting the rand number: int random = rand() % N;
                //connect to the IP address of the player socket (i.e player.sock_addr)
    
}