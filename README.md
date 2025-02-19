# UDP Client-Host-Server Communication

## Description
This project implements a UDP-based Client-Host-Server architecture where:
1. Client sends read/write requests to a server via an intermediate Host.
2. Host acts as a forwarding proxy, relaying requests from the client to the server and sending responses back.
3. Server processes the requests and sends appropriate responses back to the Host.
4. Host forwards responses to the Client.
5. Client receives and outputs the response.

## Program File Structure
- `client.cpp`: Implements the Client class for sending requests and receiving responses.
- `host.cpp`: Implements the Host class, forwarding messages between the client and server.
- `server.cpp`: Implements the Server class for processing incoming requests.
- `datagram.h`: Contains utility functions for packet creation, validation, and handling UDP sockets.

## Miscellaneous Files
- `Assign2Sequence.png`: Sequence diagram illustrating the program's execution.
- `Assign2UML.png`: UML class diagram representing the program structure.

## Compilation Instructions
Ensure you have `g++` installed. Compile the source files using:
```bash
$ g++ client.cpp -o client
$ g++ host.cpp -o host
$ g++ server.cpp -o server
```

## Execution Instructions
1. Run the Server:
   ```bash
   $ ./server
   ```
2. Run the Host:
   ```bash
   $ ./host
   ```
3. Run the Client:
   ```bash
   $ ./client <filename.txt>
   ```
   Replace `<filename.txt>` with the file you wish to request (In this program package, there is an existing `example.txt` that can be used).

## Features
- Alternating read and write requests.
- Handles invalid requests gracefully.
- Detailed packet printing for debugging.
- Error handling for socket failures.
- Timeout for graceful exit if a response is not received within 5 seconds.

## Dependencies
- **POSIX Sockets:** `<sys/socket.h>`, `<netinet/in.h>`, `<arpa/inet.h>`
- **C++ Standard Library:** `<vector>`, `<iostream>`, `<cstring>`, `<stdexcept>`, `<unistd.h>`, `<errno.h>`

