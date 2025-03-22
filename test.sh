#! /bin/bash

loop_count=${1:-1}
log_level=0
if [[ loop_count -gt 1 ]]; then
    log_level=5
    export CFLAGS="-DLOG_LEVEL=5"
fi

echo "cleaning"
make clean
echo "building w/ LOG_LEVEL $log_level"
make
if [[ $? -ne 0 ]]; then
    exit
fi

for ((i = 0; i < loop_count; i++)); do
    if [[ loop_count -gt 1 ]]; then
        echo "Testing round $((i + 1))"
    fi
    ./test_loader.sh > /dev/null
    if [[ $? -ne 0 ]]; then
        echo "Linux loader testing round $((i + 1)) failed"
        exit
    fi
    ./test_winloader.sh > /dev/null
    if [[ $? -ne 0 ]]; then
        echo "Windows loader testing round $((i + 1)) failed"
        exit
    fi
done

echo "All tests passed"
