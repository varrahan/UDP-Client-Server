#include "datagram.h"

/**
 * UDP server that processes requests in RPC style.
 */
class Server : private Socket {
private:
    bool invalid_flag = false; // Flag to terminate after processing an invalid packet
    
    /**
     * Processes an incoming RPC request and prepares the response
     * @param request The incoming request packet
     * @return The response packet to send back
     */
    std::vector<uint8_t> processRequest(const std::vector<uint8_t>& request) {
        std::cout << "\nProcessing request:" << std::endl;
        Datagram::printPacket(request);
        // Check if request is valid
        if(!Datagram::isValidRequest(request)) {
            std::cerr << "Invalid packet format" << std::endl;
            invalid_flag = true;
            return {0, static_cast<uint8_t>(Datagram::Type::ERROR), 0, 0, 'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 'r', 'e', 'q', 'u', 'e', 's', 't', 0};
        }
        // Response if read request
        std::vector<uint8_t> response;
        if(request[1] == static_cast<uint8_t>(Datagram::Type::READ)) {
            response = Datagram::createData(1, {'D', 'a', 't', 'a', ' ', 'f', 'o', 'r', ' ', 'r', 'e', 'a', 'd', ' ', 'r', 'e', 'q', 'u', 'e', 's', 't'});
        } else {
            // Ack response
            response = Datagram::createAck(0);
        }
        std::cout << "Sending response:" << std::endl;
        Datagram::printPacket(response);
        return response;
    }
    
public:
    /**
     * Creates Server instance and binds it to port 50069.
     */
    Server() {
        bind(50069);
        std::cout << "Server initialized on port 50069" << std::endl;
    }
    
    /**
     * Runs the server to process incoming RPC requests.
     * Processes requests until an invalid packet is received.
     */
    void run() {
        std::cout << "Server running" << std::endl;
        
        while(!invalid_flag) {
            char buffer[1024];
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            // Wait for reqeust
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&clientAddr, &clientLen);
            if(n < 0) {
                perror("Receive failed");
                continue;
            }

            // Process request and create response
            std::vector<uint8_t> request(buffer, buffer + n);
            std::vector<uint8_t> response = processRequest(request);
            
            // If issue with sending packet, print error message
            if(sendto(sockfd, response.data(), response.size(), 0,
                     (struct sockaddr*)&clientAddr, clientLen) < 0) {
                perror("Send failed");
            }
            
            // If packet is invalid, exit run
            if (Datagram::getPacketType(request) == Datagram::Type::INVALID) {
                std::cout << "Processed invalid packet, terminating server" << std::endl;
                break;
            }
        }
    }
};

/**
 * Main function to start the server
 * @return Exit
 */
int main() {
    Server server;
    server.run();
    return 0;
}