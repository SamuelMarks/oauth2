#!/bin/bash

echo "Compiling..."
g++ -g -DTEST_URL=1 url.cpp -o url -std=c++2a
echo "Running..."
./url
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi