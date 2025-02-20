#! /bin/bash

set -e

echo "cleaning"
make clean
echo "building linux targets"
make linux
echo "building windows targets"
make windows

loop_count=${1:-1}
for ((i = 0; i < loop_count; i++)); do 
    if [[ loop_count -gt 1 ]]; then
        echo "Testing round $((i + 1))"
    fi
    ./test_loader.sh > /dev/null
    ./test_winloader.sh > /dev/null
done

echo "All tests passed"
