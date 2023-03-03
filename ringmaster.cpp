#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>

#include "player.hpp"
#include "potato.hpp"
#include "utils.hpp"

using std::vector;
using std::string;

Player * head = NULL;
Player * curr_player = NULL;
int num_players;
fd_set readfds;
fd_set writefds;
//map of player id to socket
std::map<int, int> socketMap;
Potato * potato = new Potato();


/**
 * Inserts a new player object into the linked list of player objects and the socket map
*/
void insertNewPlayer(Player * player) {
    Player * curr = head;
    if (curr == NULL) {
        head = player;
        socketMap[player->getPlayerId()] = player->getSocket();
        return;
    }
    while (true) {
        if (curr->getPlayerId() == player->getPlayerId() - 1) {
            curr->next = player;
            player->prev = curr;
            break;
        }
        curr = curr->next;
    }
    socketMap[player->getPlayerId()] = player->getSocket();
}

/**
 * Accepts a new player connection and creates a Player object. 
 * Inserts the player into the Player list
 * inet_ntoa(player_addr->sin_addr)
*/
void listenForNewPlayers(int sock_fd, int id, int * max_fd, int num_players) {
    struct sockaddr player_addr;
    socklen_t addr_sz = sizeof(player_addr);
    
    int player_fd = accept(sock_fd, &player_addr, &addr_sz);
    struct sockaddr_in * in_addr = (struct sockaddr_in *)&player_addr;

    *max_fd = std::max(*max_fd, player_fd);
    Player * n_player = new Player(id, player_fd, inet_ntoa(in_addr->sin_addr));

    insertNewPlayer(n_player);

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
void buildSelectLists(int master_fd, int * max_fd) {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(master_fd, &readfds);
    *max_fd = master_fd;
    Player * curr = head; //head should always be Player0
    if (curr == NULL) {
        return;
    }
    do {
        FD_SET(curr->getSocket(), &readfds);
        FD_SET(curr->getSocket(), &writefds);
        *max_fd = std::max(*max_fd, curr->getSocket());
        curr = curr->next;
    } while (curr->getPlayerId() != 0);
}

/**
 * Sets up socket connections for all player processes
*/
void setUpPlayerRing(int master_fd, int max_fd) {
    buildSelectLists(master_fd, &max_fd);

    for (int i = 0; i < num_players; i++) {
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL); //listens for a connection; blocking call
        errorListener("select socket connection", status);
         //adds a new connection to the list of players
         //reading from the master socket means there's a new socket connection to it; 
         //or data is being written to it
         if (FD_ISSET(master_fd, &readfds)) {
            listenForNewPlayers(master_fd, i, &max_fd, num_players); 

            char buffer[100];
            ssize_t bytes = recv(curr_player->getSocket(), buffer, 99, 0);
            errorListener("Error receiving data from player", bytes);
            buffer[bytes] = 0;

            //receive player port from player
            char * port = strdup(buffer);
            *(curr_player->getPort()) = port;
         }

         FD_ZERO(&readfds);
         FD_SET(master_fd, &readfds);
    }
}

/*
 * Send neighbour info to players
 */
void sendNeighbourInfoToPlayers(int master_fd, int * max_fd) {
    buildSelectLists(master_fd, max_fd);
    int status = select(*max_fd+1, NULL, &writefds, NULL, NULL);
    errorListener("select socket connection", status);

    Player * curr = head;
    if (curr == NULL) {
        return;
    }
    do {
        if (FD_ISSET(curr->getSocket(), &writefds)) {

            string r_neighbour = curr->next->getPlayerData();
            string l_neighbour = curr->prev->getPlayerData();

            string curr_id = std::to_string(curr->getPlayerId()) + "&";
            string total_players = std::to_string(num_players) + "&";

            string neighbour_data = total_players + curr_id + r_neighbour + l_neighbour;
            const char * neighbours = neighbour_data.c_str();
            
            ssize_t res = send(curr->getSocket(), neighbours, strlen(neighbours), 0);
            errorListener("Error sending neighbour data", res);
            
        }
        curr = curr->next;
    } while(curr->getPlayerId() != 0);
}

/*
 * Alert players to close connection and end the game 
 */
void endGame() {
    std::map<int, int>::iterator map_it;
    for (map_it = socketMap.begin(); map_it != socketMap.end(); ++map_it) {
        const char * msg = END_GAME;
        ssize_t len = send(map_it->second, msg, strlen(msg), 0);
        errorListener("Error sending data to player", len);
        
    }
}

/*
 * Receives ready-to-play alerts from players 
 * If alert count is equal to player count, exit loop
 */
void processPlayerAlerts(int master_fd, int & max_fd, int & alert_count) {
    while (true) {
        buildSelectLists(master_fd, &max_fd);
        
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL);
        errorListener("select socket connection", status);
        
        std::map<int, int>::iterator map_it;
        for (map_it = socketMap.begin(); map_it != socketMap.end(); ++map_it) {
            if (FD_ISSET(map_it->second, &readfds)) {
                char buffer[100];
                
                ssize_t len = recv(map_it->second, buffer, 99, 0);
                errorListener("Error receiving data from player", len);
                buffer[len] = 0;

                if (strlen(buffer) > 0) {
                    alert_count++;
                } else {
                    std::cerr << "Error: invalid alert from Player: " << map_it->first << std::endl;
                    endGame();
                    close(master_fd);
                    exit(EXIT_FAILURE);

                }
                std::cout << buffer << std::endl;
            }
        }
        if (alert_count == num_players) {
            break;
        } 
    
    }
}

/*
 * Starts the game: selects a random player and sends the hot potato to the player
 */
void startGame(int numHops) {
    srand((unsigned int)time(NULL));
    int init_player = rand() % num_players;

    potato->getPlayer() = init_player;
    potato->getHops() = numHops;
    const char * potatoString = potato->getPotatoData().c_str();

    std::cout << "Ready to start the game, sending potato to player " <<  init_player << std::endl;
    send(socketMap.at(init_player), potatoString, strlen(potatoString), 0);
}

/*
 * Handle data from players during the game
 * Data is either:
 *  - player id of the current potato holder to add to the potato trace OR
 *  - potato data whose hops should be equal to 0
 */
void handlePlayerData(char * data, bool & isGameOver, string & trace) {
    string input = data;
    vector<string> potato_fields = parse(data, "&");
    
    vector<string> tokens = parse(data, "&");
    if(tokens.size() >= 1) {
        vector<string> pot_fields = parse(strdup(tokens.at(0).c_str()), ":");
        if (pot_fields.size() == 3 && pot_fields.at(1).compare("0") == 0) {
            trace = pot_fields.at(2);
            isGameOver = true;
        }
    }
}



int main(int argc, char ** argv) {
    if (argc < 4) {
        std::cerr << "Usage ./ringmaster <port_num> <num_players> <num_hops>\n";
        return EXIT_FAILURE;
    } 
    num_players = atoi(argv[2]);
    int num_hops = atoi(argv[3]);
    if (num_players < 2) {
        std::cerr << "Error: number of players must be greater or equal to 2\n";
        return EXIT_SUCCESS;
    }
    
    if (num_hops > 512) {
        std::cerr << "Error: number of hops must be less or equal to 512\n";
        return EXIT_SUCCESS;
    }
    
    int master_fd;
    int status;
    const char * port = argv[1];
    const char * hostname = NULL;
    struct addrinfo hostaddr;
    struct addrinfo *hostaddr_list;
    int max_fd;
    int alert_count = 0;
    bool isGameOver = false;
    string trace;

    //1.Open a socket to listen for incoming player connections:
        //-resolving the domain name with the port to get an IP address to bind to the socket
        //-creating a socket with the address info and binding the address to the socket
    master_fd = setupServer(
        &hostaddr,
        &hostaddr_list,
        hostname,
        port
    );
    
    status = listen(master_fd, 100);
    errorListener("listening to socket", status);
    std::cout << "Potato Ringmaster\n";
    std::cout << "Players = " << num_players << std::endl;
    std::cout << "Hops = " << num_hops << std::endl;

    setUpPlayerRing(master_fd, max_fd);
    sendNeighbourInfoToPlayers(master_fd, &max_fd);

    processPlayerAlerts(master_fd, max_fd, alert_count);
    if (num_hops == 0) {
        endGame();
        freeaddrinfo(hostaddr_list);
        close(master_fd);
        return EXIT_SUCCESS;
    }

    //std::cout << "num of hops: " << num_hops << std::endl;
    if (alert_count == num_players) {
        startGame(num_hops);
    } else {
        std::cerr << "Error: missing player alerts\n";

        return EXIT_FAILURE;
    }
    
    while (true) {
        buildSelectLists(master_fd, &max_fd);
        
        int status = select(max_fd+1, &readfds, NULL, NULL, NULL);
        errorListener("select socket connection", status);
        
        std::map<int, int>::iterator map_it;
        for (map_it = socketMap.begin(); map_it != socketMap.end(); ++map_it) {
            if (FD_ISSET(map_it->second, &readfds)) {

                char buffer[2000];
                ssize_t len = recv(map_it->second, buffer, 1999, 0);
                errorListener("Error receiving data from player", len);
                buffer[len] = 0;

                handlePlayerData(buffer, isGameOver, trace);

                if (isGameOver) {
                    break;
                }     
            }
        }
        if (isGameOver) {
            break;
        }
    
    }
    endGame();
    std::cout << "Trace of potato: \n";
    std::cout << trace << std::endl;
    freeaddrinfo(hostaddr_list);
    close(master_fd);
    return EXIT_SUCCESS;
    
}