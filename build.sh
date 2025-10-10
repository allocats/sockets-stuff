#!/usr/bin/env bash

rm -rvf build/ > /dev/null
mkdir -p build/bin/ > /dev/null

clang -c src/client.c -o build/client.o
clang -c src/server.c -o build/server.o

clang build/client.o -o build/bin/client
clang build/server.o -o build/bin/server
