#!/bin/bash

echo "Compiling..."
g++ -g -DTEST_OPEN_BROWSER=1 open_browser.cpp -o open_browser -std=c++2a
echo "Running..."
./open_browser
if [ $? == 0 ]; then
    echo "SUCCESS"
else
    echo "FAILED"
fi