# Project work for Distributed System course @ University of Oulu

## IRC inspired chat server and client

- Created using Boost 1.72
- Asynchronous server
- Server's starting script broadcasts ip - no need to know it!
- Client's starting script listens for the ip and connects there.
- Little fork needed to support multiple chat rooms (not implemented as we do not think this is necessary.)


## Dependencies
- Boost 1.72
- g3log
- python3

## Usage
To install, use ```make```. 
./server.sh will start the server and ./client.sh <nickname> will start the client.
We assume that the ip to be connected can be found from eth0. 
  
## Start manually


**Start server**
```
./ChatBoxServer <port>
```

**Start client** 
```
./ChatBox <nickname> <host> <port>
```
