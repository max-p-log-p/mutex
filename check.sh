#!/bin/sh
NUM_FILES=5
NUM_SERVERS=3
ROOT_DIR="$HOME/p2"

for ((i = 0; i < $NUM_FILES; ++i)); do less "$ROOT_DIR/0/$i"; done
for ((i = 1; i < $NUM_SERVERS; ++i)); do diff -s "$ROOT_DIR/0" "$ROOT_DIR/$i"; done
