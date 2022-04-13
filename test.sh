#!/bin/sh
NUM_SERVERS=3
NUM_CLIENTS=5
ROOT_DIR="$HOME/p2"

killall p2s p2c 2>/dev/null

for ((i = 0; i < $NUM_SERVERS; ++i)); do ./p2s "$i" "$ROOT_DIR/$i/" ips & done

sleep 1

for ((i = 0; i < $NUM_CLIENTS; ++i)); do ./p2c "$i" ips & sleep $i; done
