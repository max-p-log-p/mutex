#!/bin/sh
NUM_SERVERS=2
NUM_CLIENTS=5
ROOT_DIR="./p2"

if [ "$1" = "-c" ]; then
    for ((i = 1; i < $NUM_SERVERS; ++i)) do diff -s "$ROOT_DIR/0" "$ROOT_DIR/$i"; done
else
    mkdir -p "$ROOT_DIR"

    killall p2s p2c 2>/dev/null

    for ((i = 0; i < $NUM_SERVERS; ++i)); do ./p2s "$i" "$ROOT_DIR/$i/" ips & done

    sleep 1

    for ((i = 0; i < $NUM_CLIENTS; ++i)); do ./p2c "$i" ips & sleep $i; done
fi
