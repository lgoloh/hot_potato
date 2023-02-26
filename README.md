# Hot Potato
An inter-process game of hot potato to learn about socket programming and inter-process communication

Game play:
There are some number of players who quickly toss a potato from one player to another until, at a random point, the game ends
and the player holding the potato is “it”. The objective is to not be the one holding the potato at the
end of the game.

In this assignment, you will create a ring of “player” processes that will pass
the potato around. Thus, each player process has a left neighbor and a right neighbor. Also,
there will be a “ringmaster” process that will start each game, report the results, and shut down
the game.

Learning objectives:
- Practice creating a multi-process application
- Processing command line arguments
- Setting up and monitoring network communication channels between the processes (using TCP sockets)
- Reading/writing information between processes across sockets
