all: create

create:
	g++ -std=c++17 -O2 -Wall src/hash.cpp src/main.cpp src/blockchain.cpp -o blockchain

run: create
	./blockchain

clean:
	rm -f blockchain