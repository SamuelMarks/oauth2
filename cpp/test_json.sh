#!/bin/bash

echo "Compiling..."
g++ -g -DTEST_JSON=1 json_parser.cpp -o json -std=c++2a
echo "Running..."
./json
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi