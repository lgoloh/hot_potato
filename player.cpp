#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <cassert>

#include "player.hpp"
#include "utils.hpp"

using std::vector;
using std::string;
#define MAX_CONN 2

fd_set readfds;
fd_set writefds;
int player_id;
int num_players;

int player_sock;
int ringmaster_sock;

int max_fd;
int curr_sockfd;
int player_port;
Player * right_neighbor = new Player();
Player * left_neighbor = new Player();
vector<int> neighbours_fds[2];
bool isConnected = false;
bool isGameOver = false;

/*
 * Neighbour connection startegy:
 * Right neighbor: use getaddrinfo() with the host and port of the player process to get right neighbour socket
 * Left neighbor: listen for connection on player_sock and accept() to open new socket connection for left neighbor
 */


void printSocketList() {
    for (int i = 0; i < neighbours_fds->size(); i++) {
        int fd = neighbours_fds->at(i);
        std::cout << "fd at " <<  i << " = " << fd << std::endl;
    }
}

/*
 * Creates a client socket to connect with a server process
 */
int getClientSocket(int protocol) {
    return socket(AF_INET, SOCK_STREAM, protocol);
}

/*
 * Parses neighbour data to set Player fields
 * i.e <player_id>:<player_host>:<player_port>&
 */
void setNeighbourFields(char * data, Player * neighbour) {
    vector<string> neighbour_fields = parse(data, ":");
    if (neighbour_fields.size() != 3) {
        std::cerr << "Error: invalid neighbour data\n";
        exit(EXIT_FAILURE);
    } 
    neighbour->getPlayerId() = atoi(neighbour_fields.at(0).c_str());
    *(neighbour->getHost()) = strdup(neighbour_fields.at(1).c_str());
    *(neighbour->getPort()) = strdup(neighbour_fields.at(2).c_str());
}

/*
 * Parses intial setup data received from ringmaster 
 * i.e <num of players>&<player id>&<right neighbour data>&<left neighbour data>&
 */
void parseInitData(char * data) {
    if (strlen(data) == 0) {
        return;
    }
    vector<string> neighbour_tokens = parse(data, "&");
    if (neighbour_tokens.size() >= 4) {
        num_players = atoi(neighbour_tokens.at(0).c_str());
        player_id = atoi(neighbour_tokens.at(1).c_str());
        setNeighbourFields(strdup(neighbour_tokens.at(2).c_str()), right_neighbor);
        setNeighbourFields(strdup(neighbour_tokens.at(3).c_str()), left_neighbor);
    }
}


/*
 * Accepts a new connection from the left neighbour and adds the connection socket to the list
 */
void handleNewConnection() {
    struct sockaddr neighbor_addr;
    socklen_t sz = sizeof(neighbor_addr);

    int left_sock = accept(player_sock, &neighbor_addr, &sz);
    if (left_sock == -1) {
        if (neighbours_fds->size() == 2) {
            perror("Error: max number of players reached");
        } else {
            perror("Error: cannot accept new player");
        }
        exit(EXIT_FAILURE);
    }
   
    left_neighbor->getSocket() = left_sock;
    neighbours_fds->push_back(left_sock);
    max_fd = std::max(max_fd, left_sock);
    curr_sockfd = left_sock;
}

/*
 * Connects the player to another process with the specified host name and port
 */
void connectToProcess(const char * host_name, const char * host_port, int * client_fd) {
    struct addrinfo hostaddr;
    struct addrinfo *hostaddr_list;

    memset(&hostaddr, 0, sizeof(hostaddr));
    hostaddr.ai_family = AF_INET;
    hostaddr.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(host_name, host_port, &hostaddr, &hostaddr_list);
    errorListener("Error connecting to process", status);

    *client_fd = getClientSocket(hostaddr_list->ai_protocol);
    status = connect(*client_fd, hostaddr_list->ai_addr, hostaddr_list->ai_addrlen);

    errorListener("Error connecting to process", status);

}

/**
 * Build the select lists and update the max socket descriptor
*/
void buildSelectLists() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(player_sock, &readfds);
    FD_SET(ringmaster_sock, &readfds);
    max_fd = std::max(player_sock, ringmaster_sock);
    for (int i = 0; i < neighbours_fds->size(); i++) {
        int curr_fd = neighbours_fds->at(i);
        
        max_fd = std::max(max_fd, curr_fd);
        FD_SET(curr_fd, &readfds);
        FD_SET(curr_fd, &writefds);
    }
}

/*
 * Sets up socket connections with the right neighbour of the player
 */
void connectToNeighbours() {
    //connect to right neighbour process
    int right_sock;

    connectToProcess(*(right_neighbor->getHost()), *(right_neighbor->getPort()), &right_sock);
    neighbours_fds->push_back(right_sock);
    right_neighbor->getSocket() = right_sock;

    //accept connection from left neighbour
    handleNewConnection();
    
    string message = "Player " + std::to_string(player_id) + " is ready to play";
    const char * alert = message.c_str();
    send(ringmaster_sock, alert, strlen(alert), 0);
}

/*
 * - Attach player id
 * - Decrement hops by 1
 * - if hops == 0:
 *      - send potato to ringmaster
 *   else send potato to random neighbour
 */
void handlePotatoData(string data) {
    char * input = strdup(data.c_str());
    vector<string> potato_fields = parse(input, "&");

    if (potato_fields.size() == 1) {
        char * d = strdup(potato_fields.at(0).c_str());
        vector<string> pot_data = parse(d, ":");
        string trace;
        if (pot_data.size() != 3) {
            trace = std::to_string(player_id) + "&";
        } else {
            trace = pot_data.at(2) + "," + std::to_string(player_id) + "&";
        }

        int hops = stoi(pot_data.at(1)) - 1;
        string update = std::to_string(player_id) + ":" + std::to_string(hops) + ":" + trace;

        const char * potato = update.c_str();
        if (hops == 0) {
            std::cout << "I'm it\n";
            send(ringmaster_sock, potato, strlen(potato), 0);
        } else {
            int index = rand() % MAX_CONN; // pick a random index
            int fd = neighbours_fds->at(index);

            if (fd == right_neighbor->getSocket()) {
                std::cout << "Sending potato to " << right_neighbor->getPlayerId() << std::endl;
            } else if (fd == left_neighbor->getSocket()) {
                std::cout << "Sending potato to " << left_neighbor->getPlayerId() << std::endl;
            }

            send(fd, potato, strlen(potato), 0);
            
        }
    }
    
}

/*
 * Handles init setup data from ringmaster
*/
void init(char * input) {

    parseInitData(input);
    std::cout << "Connected as player " 
                        << std::to_string(player_id) 
                        <<" out of " << std::to_string(num_players) << " total players\n";
    
}

/*
 * Processes data from the ringmaster
 * - end game 
 * - potato 
 * - initial setup data
 */
void handleDataFromRingMaster() {

    char buffer[2000];
    ssize_t len = recv(ringmaster_sock, buffer, 1999, 0);
    errorListener("Error receiving neighbour data", len);
    buffer[len] = 0;

    if (strlen(buffer) == 0) {
        return;
    }

    string input = buffer;
    if (input.find("&") == string::npos) { 
        if (input.compare(END_GAME) == 0) {
            isGameOver = true;
        } else {
            handlePotatoData(input);
        }
        
    } else {
        init(buffer);
        connectToNeighbours();
    }
    
}

/*
 * Processes data received from neighbour 
 */
void handleDataFromNeighbours(int neighbour_fd) {

    char buffer[2000];
    ssize_t len = recv(neighbour_fd, buffer, 1999, 0);
    errorListener("Error receiving neighbour data", len);
    buffer[len] = 0;

    string input = buffer;

    if (strchr(buffer, ':') != NULL) {
        handlePotatoData(input);     
    }
}

/*
 * Connect to the ringmaster and send port number
 */
void connectToRingmaster(const char * host_name, const char * host_port) {
    //connect to ringmaster process
    connectToProcess(host_name, host_port, &ringmaster_sock);

    //send port number to ringmaster 
    const char * port = std::to_string(player_port).c_str();
    send(ringmaster_sock, port, strlen(port), 0);
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

    //1.Open a socket to listen for incoming neigbour connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    player_sock = setupServer(
        &playeraddr,
        &playeraddr_list,
        hostname,
        ""
    );
    player_port = getPort(player_sock);
    status = listen(player_sock, 100);

    //2.Connect listener to ringmaster
    connectToRingmaster(master_name, port);
    errorListener("Error listening to connections", status);

    while(true) {
        if (isGameOver) {
            break;
        }
        buildSelectLists();

        status = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        errorListener("Error listening to neighbours", status);

        if (FD_ISSET(ringmaster_sock, &readfds)) {
            handleDataFromRingMaster();
        }

        for (int i = 0; i < neighbours_fds->size(); i++) {
            int fd = neighbours_fds->at(i);
            if (FD_ISSET(fd, &readfds)) {
                handleDataFromNeighbours(fd);
            }
        }

    }

    freeaddrinfo(playeraddr_list);
    close(player_sock);   
    return EXIT_SUCCESS;
    
}