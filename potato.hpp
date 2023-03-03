#ifndef __POTATO_HPP__
#define __POTATO_HPP__
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

using std::string;
using std::vector;

class Potato {
    private:
        int currentPlayer;
        int numOfHops;
        vector<int> player_trace;
    public:
        Potato() : currentPlayer(-1), numOfHops(0), player_trace(vector<int>()) {};
        Potato(int hops) : currentPlayer(-1), numOfHops(hops), player_trace(vector<int>()) {};
        Potato(int playerId, int hops) : currentPlayer(playerId), numOfHops(hops), player_trace(vector<int>()) {};
        void decrementHops() {
            numOfHops -= 1;
        }
        void updatePlayer(int id) {
            currentPlayer = id;
            player_trace.push_back(id);
        }
        int & getHops() {return numOfHops;}

        int & getPlayer() {return currentPlayer;}

        void addPlayerToTrace(int id) {
            player_trace.push_back(id);
        }

        string getPotatoTrace() {
            size_t i = 0;
            std::stringstream trace;
            for (; i < player_trace.size() - 1; i++) {
                trace << player_trace.at(i) << ",";
            }
            trace << player_trace.at(i);
            return trace.str();
        }

        /*
         * Potato data format: <player>:<hops>&
         */
        string getPotatoData() {
            std::stringstream potato;
            potato << currentPlayer << ":" << numOfHops;
            return potato.str();
        }
};

#endif
