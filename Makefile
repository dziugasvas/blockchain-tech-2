all: create

create:
	g++ -std=c++17 -O2 -Wall src/hash.cpp src/main.cpp src/blockchain.cpp -o blockchain

run: create
	./blockchain

clean:
	rm -f blockchain

test:
	g++ -std=c++17 -O2 -Wall -Isrc src/hash.cpp src/blockchain.cpp tests/tests.cpp -o run_tests && ./run_tests

bench:
	g++ -std=c++17 -O2 -Wall -Isrc src/hash.cpp src/blockchain.cpp benchmark/bench.cpp -o run_bench && ./run_bench