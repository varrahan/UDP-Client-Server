#include "datagram.h"

/**
 * @class Server
 * A simple UDP server that listens on port 50069 and processes requests. 
 * The server binds to a predefined port and continuously listens for incoming datagrams. 
 * It processes read and write requests by validating incoming packets and responding accordingly.
 */
class Server : private Socket {
private:
    bool invalid_flag = false; // For this assignment, since the last request is invalid, force a termination after reading an invalid packet

    /**
     * Processes incoming UDP requests.
     * This method receives datagrams, validates them, and responds accordingly.
     * It supports two types of requests: read (response code {0, 3, 0, 1}) and write (response code {0, 4, 0, 0}).
     */
   void processRequest() {
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        
        // Receive data from the client through the host
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                        (struct sockaddr*)&clientAddr, &clientLen);
        if(n < 0) {
            perror("Receive failed");
            return;
        }
        
        // Convert received data to a packet vector
        std::vector<uint8_t> packet(buffer, buffer + n);
        std::cout << "\nReceived request:" << std::endl;
        Datagram::printPacket(packet);
        
        // Validate the received packet
        if(!Datagram::isValidRequest(packet)) {
            std::cerr << "Invalid packet format" << std::endl;
            invalid_flag = true;
            return;
        }
        
        // Prepare response based on request type
        std::vector<uint8_t> response;
        if(packet[1] == 1) {  // Read request
            response = {0, 3, 0, 1};
        } else {  // Write request
            response = {0, 4, 0, 0};
        }
        
        std::cout << "Sending response:" << std::endl;
        Datagram::printPacket(response);
        
        // Send response back to the client
        if(sendto(sockfd, response.data(), response.size(), 0,
                 (struct sockaddr*)&clientAddr, clientLen) < 0) {
            perror("Send failed");
        }
    }
public:
    /**
     * Constructs a Server instance and binds it to port 50069. 
     * Initializes the server and binds it to a non-privileged port for communication.
     */
    Server() {
        bind(50069);  // Non-privileged port
        std::cout << "Server initialized on port 50069" << std::endl;
    }

    /**
     * Runs the server in an infinite loop to process incoming requests. This method continuously listens for and processes client requests.
     */
    void run() {
        std::cout << "Server running" << std::endl;
        while(true) {
            if (invalid_flag) {
                return;
            }
            processRequest();
        }
    }
};

/**
 * The entry point of the program. Initializes and runs the server.
 * If an exception occurs, it prints an error message.
 * @return int Returns 0 on success, 1 on error.
 */
int main() {
    try {
        Server server;
        server.run();
    } catch(const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
