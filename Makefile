CFLAGS=-std=gnu++98 -pedantic -Wall -Werror -ggdb3
TARGETS=player ringmaster
OBJS=$(patsubst %,%.o,$(TARGETS)) player.o potato.o
all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp player.o potato.o
	g++ -g -o $@ $<

ringmaster: ringmaster.cpp player.o potato.o
	g++ -g -o $@ $<

%.o: %.hpp
	g++ $(CFLAGS) -c $<