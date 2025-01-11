#! /bin/bash

set -e

loop_count=${1:-1000}
for ((i = 0; i < loop_count; i++)); do 
    echo "Testing round $((i + 1))"
    ./loader_test.sh
    ./winloader_test.sh
done
