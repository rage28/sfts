Simple File Transfer Server (SFTS)
==================================

SFTS is a simple server which used sockets and lists a set of files it hosts to any client that connects to it. The client can then request the server to send the file by typing in the filename. The entire program was written in C


The application consists of two modules:

Server
------

* It starts and listens to any clients on the port __50000__
* Sends a list of files to any client that connects to it from the '__files__' folder
* It receive from the client the filename that needs to be send and then sends it from stated folder if it is found
* After sending it successfully it closes the connection with its client
* This is a __concurrent__ server capable of serving multiple clients at the same time

Client
------

* It takes one argument and is mandatory. The parameter should a valid IP address where the server is hosted. The usage syntax is '__./client <IP Address\>__'
* The port cannot be changed and is always __50000__
* When the client connects to the server it receives a list of files the server is hosting and the user can then enter a valid filename from the list for the server to send
* The files successfully received will be stored in the '__recv__' directory
* The client then disconnects from the server

Issues Faced
------------
* The main issue faced was making the server concurrent to server large files (>10Mb)
* The second most annoying issue faced was freeing resources in a concurrent environment. For example after opening the fopen() on the target file, if it was not closed using the fclose() function then the server would get stuck without closing the client connection and would refuse any subsequent connections
* Making the server a little more robust by creating a working directory relative to the server binary and the client binary as well
