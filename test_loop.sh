#! /bin/bash

set -e

for i in {1..10000}; do 
    echo "Running $i"
    ./loader_test.sh
done
