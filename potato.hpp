#ifndef __POTATO_HPP__
#define __POTATO_HPP__

class Potato {
    private:
        int currentPlayer;
        int numOfHops;
    public:
        Potato() : currentPlayer(-1), numOfHops(0) {};
        Potato(int playerId, int hops) : currentPlayer(playerId), numOfHops(hops) {};
        void decrementHops() {
            numOfHops -= 1;
        }
        void updatePlayer(int id) {
            currentPlayer = id;
        }
        int getHops() {return numOfHops;}
        int getPlayer() {return currentPlayer;}
};

#endif
