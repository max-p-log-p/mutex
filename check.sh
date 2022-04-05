#!/bin/sh
NUM_CLIENTS=5
NUM_SERVERS=3
ROOT_DIR="$HOME/p2"

for ((i = 0; i < $NUM_CLIENTS; ++i)); do grep "^$i" $ROOT_DIR/0/* | awk '{print $2}' | sort -n | less; done
for ((i = 1; i < $NUM_SERVERS; ++i)); do diff -s "$ROOT_DIR/0" "$ROOT_DIR/$i"; done