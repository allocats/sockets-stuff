#!/usr/bin/env bash

for i in {1..1000}; do
    ./build/bin/client "Message $i"
done
