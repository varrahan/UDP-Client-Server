#ifndef DATAGRAM_H
#define DATAGRAM_H

#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

/**
 * @class Datagram
 * Provides utility functions for creating and handling datagrams.
 */
class Datagram {
public:
    /**
     * Creates a Datagram packet.
     * @param filename The name of the file to read.
     * @param mode The mode of transfer.
     * @param isRead The request type, true if read request false if write request
     * @return A vector containing the read reqeust packet data.
     */
    static std::vector<uint8_t> createRequest(const std::string& filename, const std::string& mode, bool isRead) {
        std::vector<uint8_t> packet = {0, static_cast<uint8_t>(isRead ? 1 : 2)};
        packet.insert(packet.end(), filename.begin(), filename.end());
        packet.push_back(0);  // Zero byte after filename
        packet.insert(packet.end(), mode.begin(), mode.end());
        packet.push_back(0);  // Zero byte after mode
        return packet;
    }

    /**
     * Prints the packet content as both raw bytes and a human-readable string.
     * @param packet The packet to print.
     */
    static void printPacket(const std::vector<uint8_t>& packet) {
        std::cout << "Packet as bytes: ";
        for(uint8_t byte : packet) {
            std::cout << static_cast<int>(byte) << " ";
        }
        std::cout << "\nPacket as string: ";
        for(uint8_t byte : packet) {
            if(isprint(byte)) {
                std::cout << static_cast<char>(byte);
            } else {
                std::cout << "[" << static_cast<int>(byte) << "]";
            }
        }
        std::cout << std::endl;
    }
    
    /**
     * Validates whether the given packet follows the expected format of a request.
     * @param packet The packet to validate.
     * @return True if the packet is a valid request, false otherwise.
     */
    static bool isValidRequest(const std::vector<uint8_t>& packet) {
        if(packet.size() < 4) return false;
        if(packet[0] != 0) return false;
        if(packet[1] != 1 && packet[1] != 2) return false;
        
        // Find first zero after filename
        size_t firstZero = 2;
        while(firstZero < packet.size() && packet[firstZero] != 0) firstZero++;
        if(firstZero >= packet.size()) return false;
        
        // Find second zero after mode
        size_t secondZero = firstZero + 1;
        while(secondZero < packet.size() && packet[secondZero] != 0) secondZero++;
        if(secondZero >= packet.size()) return false;
        
        // Ensure nothing after second zero
        return secondZero == packet.size() - 1;
    }
};

/**
 * @class Socket
 * A base class for handling UDP socket communication.
 */
class Socket {
protected:
    int sockfd; // The socket file descriptor.
    struct sockaddr_in addr; // The socket address structure.

    /**
     * Constructs a Socket and initializes the socket.
     * @throws std::runtime_error if socket creation fails.
     */
    Socket() : sockfd(-1) { // Constructor is a protected method to prevent programs from directly instintating it
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0) {
            perror("Socket creation failed");
            throw std::runtime_error("Error creating socket");
        }
        
        // Enable address reuse
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            perror("setsockopt failed");
            throw std::runtime_error("Error setting socket option");
        }
    }

    /**
     * Destroys the ocket and closes the socket if open.
     */
    ~Socket() {
        if(sockfd >= 0) {
            close(sockfd);
        }
    }

public:
    /**
     * Binds the socket to the specified port.
     * @param port The port number to bind to.
     * @throws std::runtime_error if binding fails.
     */
    void bind(uint16_t port) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        // Validate whether or not socket was successfully bound to object
        if(::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Bind failed");
            throw std::runtime_error("Error binding socket");
        }
    }
};

#endif // DATAGRAM_H