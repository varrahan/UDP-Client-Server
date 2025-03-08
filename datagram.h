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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

/**
 * Provides utility functions for creating and handling datagrams
 */
class Datagram {
public:
    // Packet type
    enum class Type : uint8_t {
        READ = 1,    // Read
        WRITE = 2,    // Write
        DATA = 3,   // Data
        ACK = 4,    // Ack
        ERROR = 5,  // Error
        INVALID = 6 // Invalid
    };
    
    /**
     * Creates a read or write request packet
     * @param filename The name of the file
     * @param mode The mode
     * @param isRead True if read request false if write request
     * @return A vector containing request packet data
     */
    static std::vector<uint8_t> createRequest(const std::string& filename, const std::string& mode, bool isRead) {
        std::vector<uint8_t> packet = {0, static_cast<uint8_t>(isRead ? static_cast<uint8_t>(Type::READ) : static_cast<uint8_t>(Type::WRITE))};
        packet.insert(packet.end(), filename.begin(), filename.end());
        packet.push_back(0); 
        packet.insert(packet.end(), mode.begin(), mode.end());
        packet.push_back(0);
        return packet;
    }

    /**
     * Creates an invalid packet
     * @return A vector containing an invalid packet
     */
    static std::vector<uint8_t> createInvalidPacket() {
        return {0, static_cast<uint8_t>(Type::INVALID), 'i', 'n', 'v', 'a', 'l', 'i', 'd'};
    }

    /**
     * Creates an ack packet
     * @param blockNum The block number to acknowledge
     * @return A vector containing the ack packet data
     */
    static std::vector<uint8_t> createAck(uint16_t blockNum) {
        std::vector<uint8_t> packet = {0, static_cast<uint8_t>(Type::ACK)};
        packet.push_back(static_cast<uint8_t>(blockNum >> 8)); 
        packet.push_back(static_cast<uint8_t>(blockNum & 0xFF));
        return packet;
    }

    /**
     * Creates a data packet.
     * @param blockNum The block number.
     * @param data The data in the packet.
     * @return A vector containing the data
     */
    static std::vector<uint8_t> createData(uint16_t blockNum, const std::vector<uint8_t>& data) {
        std::vector<uint8_t> packet = {0, static_cast<uint8_t>(Type::DATA)};
        packet.push_back(static_cast<uint8_t>(blockNum >> 8));
        packet.push_back(static_cast<uint8_t>(blockNum & 0xFF));
        packet.insert(packet.end(), data.begin(), data.end());
        return packet;
    }

    /**
     * Extracts the packet type from a packet
     * @param packet The packet to extract from
     * @return Packet type
     */
    static Type getPacketType(const std::vector<uint8_t>& packet) {
        if (packet.size() < 2) return Type::ERROR;
        return static_cast<Type>(packet[1]);
    }

    /**
     * Prints the packet as both string and bytes
     * @param packet The packet to print
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
        if (packet.size() >= 2) {
            std::cout << "Packet type: ";
            switch (static_cast<Type>(packet[1])) {
                case Type::READ: std::cout << "Read Request"; break;
                case Type::WRITE: std::cout << "Write Request"; break;
                case Type::DATA: std::cout << "Data"; break;
                case Type::ACK: std::cout << "Acknowledgment"; break;
                case Type::ERROR: std::cout << "Error"; break;
                case Type::INVALID: std::cout << "Invalid"; break;
                default: std::cout << "Unknown (" << static_cast<int>(packet[1]) << ")"; break;
            }
            std::cout << std::endl;
        }
    }
    
    /**
     * Check if packet is valid
     * @param packet The packet to validate
     * @return True if valid, else false
     */
    static bool isValidRequest(const std::vector<uint8_t>& packet) {
        if(packet.size() < 4) return false;
        if(packet[0] != 0) return false;
        if(packet[1] != static_cast<uint8_t>(Type::READ) && 
           packet[1] != static_cast<uint8_t>(Type::WRITE)) return false;
        size_t firstZero = 2;
        while(firstZero < packet.size() && packet[firstZero] != 0) firstZero++;
        if(firstZero >= packet.size()) return false;
        size_t secondZero = firstZero + 1;
        while(secondZero < packet.size() && packet[secondZero] != 0) secondZero++;
        if(secondZero >= packet.size()) return false;
        return secondZero == packet.size() - 1;
    }
};

/**
 * A base class for handling UDP socket communication
 */
class Socket {
protected:
    int sockfd;
    struct sockaddr_in addr;

    /**
     * Constructs and initializes socket
     * @throws std::runtime_error if socket fails
     */
    Socket() : sockfd(-1) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if(sockfd < 0) {
            perror("Socket creation failed");
            throw std::runtime_error("Error creating socket");
        }
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
            perror("setsockopt failed");
            throw std::runtime_error("Error setting socket option");
        }
    }

    /**
     * Socket destructor
     */
    ~Socket() {
        if(sockfd >= 0) {
            close(sockfd);
        }
    }

public:
    /**
     * Binds socket to the port
     * @param port The port number to bind
     * @throws std::runtime_error if binding fails
     */
    void bind(uint16_t port) {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if(::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Bind failed");
            throw std::runtime_error("Error binding socket");
        }
    }
    
    /**
     * RPC send function that sends a packet and waits for a response
     * @param sendPacket Packet to send
     * @param destAddr Destination address
     * @param destLen Length of destination address
     * @param receivePacket Reference to store received packet
     * @param timeoutSec Timeout (default is 5 seconds)
     * @return True if successful, false if error
     */
    bool rpcSend(const std::vector<uint8_t>& sendPacket, 
                const struct sockaddr_in& destAddr, 
                socklen_t destLen,
                std::vector<uint8_t>& receivePacket,
                int timeoutSec = 5) {
        
        if (sendto(sockfd, sendPacket.data(), sendPacket.size(), 0,
                   (struct sockaddr*)&destAddr, destLen) < 0) {
            perror("Send failed in RPC");
            return false;
        }
        
        char buffer[1024];
        struct sockaddr_in responseAddr;
        socklen_t responseLen = sizeof(responseAddr);
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        struct timeval timeout;
        timeout.tv_sec = timeoutSec;
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

        if (activity <= 0) {
            if (activity == 0) {
                std::cerr << "RPC Timeout: No response received within " << timeoutSec << " seconds" << std::endl;
            } else {
                perror("Select error in RPC");
            }
            return false;
        }

        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                         (struct sockaddr*)&responseAddr, &responseLen);
        if (n < 0) {
            perror("Receive failed in RPC");
            return false;
        }
        receivePacket.assign(buffer, buffer + n);
        return true;
    }
};
#endif // DATAGRAM_H