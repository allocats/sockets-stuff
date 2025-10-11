#!/usr/bin/env bash

rm -rvf build/ > /dev/null
mkdir -p build/bin/ > /dev/null

clang -g -c src/client.c -o build/client.o
clang -g -c src/server.c -o build/server.o

clang -g build/client.o -o build/bin/client
clang -g build/server.o -o build/bin/server
