CC = clang++
STD = -std=c++2a
CFLAGS = $(STD)

ifeq ($(debug), 1)
	CFLAGS += -gdwarf-4 -Wall -mpopcnt -mbmi2 -m64
else
	CFLAGS += -O3 -pipe -march=native -Wall -flto -DNDEBUG -mpopcnt -m64 -mbmi2
endif

O = main.o analyzer.o ChaosMagic.o MoveMake.o Zobrist.o Backtrack.o Rollout.o

cc0: $(O)
	$(CC) $(CFLAGS) -lpthread -o $@ $(O)

main.o: main.cpp Rollout.h Fen.h analyzer.h
	$(CC) $(CFLAGS) -c main.cpp

ChaosMagic.o: ChaosMagic.cpp ChaosMagic.h
	$(CC) $(CFLAGS) -c ChaosMagic.cpp

MoveMake.o: MoveMake.cpp MoveMake.h Zobrist.h ChaosMagic.h
	$(CC) $(CFLAGS) -c MoveMake.cpp

Zobrist.o: Zobrist.cpp Zobrist.h 
	$(CC) $(CFLAGS) -c Zobrist.cpp

Backtrack.o: Backtrack.cpp Backtrack.h 
	$(CC) $(CFLAGS) -c Backtrack.cpp

Rollout.o: Rollout.cpp Rollout.h Backtrack.h
	$(CC) $(CFLAGS) -c Rollout.cpp

analyzer.o: analyzer.cpp analyzer.h
	$(CC) $(CFLAGS) -c analyzer.cpp

clean:
	rm *.o cc0
