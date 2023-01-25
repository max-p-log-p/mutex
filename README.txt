Building
--------
$ make


Testing
-------
Create a file with NUM_SERVERS hostnames. 

NUM_SERVERS is set by default to 2 in proc.h and 2 in test.sh.

// example ips
10.159.191.196
127.0.0.1

Run `./test.sh` for testing and `./test.sh -c` to check for correctness. 

test.sh will create a subdirectory in ROOT_DIR.

The files in the subdirectories of ROOT_DIR should be identical after the program halts.
