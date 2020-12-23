#!/bin/bash

echo "Compiling..."
g++ -g -DTEST_TINY_WEB_CLIENT=1 -lssl -lcrypto tiny_web_client.cpp -o tiny_web_client -std=c++2a
echo "Running..."
./tiny_web_client
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi