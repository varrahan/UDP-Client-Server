#include "datagram.h"

/**
 * @class Host
 * A UDP-based host that forwards packets between a client and a server.
 * The Host listens on a predefined port for incoming client messages, forwards them to a predefined server address, receives the server's response, and relays it back to the client.
 */
class Host : private Socket {
private:
    struct sockaddr_in serverAddr; // Server address information.
    
    /**
     * Forwards a packet to the server and relays the response back to the client.
     * @param packet The packet received from the client.
     * @param clientAddr The address of the client.
     * @param clientLen The length of the client's address structure.
     */
    void forward(const std::vector<uint8_t>& packet, const struct sockaddr_in& clientAddr, socklen_t clientLen) {
        // Forward to server
        if (sendto(sockfd, packet.data(), packet.size(), 0,
                   (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Forward to server failed");
            return;
        }
        
        // Receive response from server
        char buffer[1024];
        struct sockaddr_in responseAddr;
        socklen_t responseLen = sizeof(responseAddr);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;  // Set 5 second timeout
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity == 0) {  // Timeout occurred
            std::cerr << "Timeout: No response received within 5 seconds" << std::endl;
            exit(1);
        } else if (activity < 0) {
            throw std::runtime_error("Error during select()");
        }

        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&responseAddr, &responseLen);
        if (n < 0) {
            perror("Receive from server failed");
            return;
        }
        
        std::vector<uint8_t> response(buffer, buffer + n);
        std::cout << "Received from server:" << std::endl;
        Datagram::printPacket(response);
        
        // Forward response to client
        if (sendto(sockfd, response.data(), response.size(), 0,
                   (struct sockaddr*)&clientAddr, clientLen) < 0) {
            perror("Forward to client failed");
        }
    }
public:
    /**
     * Initializes the server address structure and binds the socket to port 50023 for communication.
     * Constructs a Host object and binds it to a port.
     */
    Host() {
        bind(50023);  // Non-privileged port
        
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50069);
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        
        std::cout << "Host initialized" << std::endl;
    }

    /**
     * Enters a loop where it receives packets from clients, prints the packet data, and forwards it to the server.
     * Starts the host to listen and forward packets.
     */
    void run() {
        std::cout << "Host running" << std::endl;
        
        while (true) {
            char buffer[1024];
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);

            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&clientAddr, &clientLen);
            if (n < 0) {
                perror("Receive from client failed");
                continue;
            }
            std::vector<uint8_t> packet(buffer, buffer + n);
            std::cout << "\nReceived from client:" << std::endl;
            Datagram::printPacket(packet);
            
            forward(packet, clientAddr, clientLen);
        }
    }

};

/**
 * Main function to start the host. 
 * Creates a Host instance and runs it. 
 * Handles any exceptions that may occur.
 * @return int Exit status code.
 */
int main() {
    try {
        Host host;
        host.run();
    } catch (const std::exception& e) {
        std::cerr << "Host error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
