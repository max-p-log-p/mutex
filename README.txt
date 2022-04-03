Building
--------
$ make


Testing
-------
Create a directory in $HOME.

$ mkdir ~/p2/

Create a file with at most 3 server hostnames.

// ips
dc01.utdallas.edu
dc02.utdallas.edu
dc03.utdallas.edu

Execute p2s with the id as the line number of the hostname of the current system.

For dc01.utdallas.edu:

$ ./p2s 0 ~/p2/0 ips

Execute p2c on different machines with the id as any number between [0, 4].
No client should share an id.

$ ./p2c 0 ips
