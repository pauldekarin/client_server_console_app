GCC=g++ -std=c++17

all:build

.PHONY:build
build:build_server build_client

.PHONY: run_server
run_server:clear build_server
	./server.out
.PHONY: build_server
build_server:
	$(GCC) globals.cpp server.cpp -o server.out

.PHONY: run_client
run_client:clear build_client
	./client.out
.PHONY: build_client
build_client:
	$(GCC) globals.cpp client.cpp -o client.out

.PHONY: clear
clear:
	rm -rf *.out *.o