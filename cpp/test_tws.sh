#!/bin/bash

echo "Compiling..."
g++ -g -DTEST_TINY_WEB_SERVER=1 -lssl -lcrypto tiny_web_server.cpp -o tiny_web_server -std=c++2a
echo "Running..."
./tiny_web_server
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi