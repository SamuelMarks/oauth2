#!/bin/bash

echo "Compiling..."
g++ -g -lssl -lcrypto main.cpp -o main -std=c++2a
echo "Running..."
./main
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi