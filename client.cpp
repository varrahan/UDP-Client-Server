#include "datagram.h"

/**
 * @class Client
 * UDP client that communicates with a server using an intermediate host service.
 */
class Client : private Socket {
private:
    struct sockaddr_in serverAddr;  // Server address structure 
    std::string filename;           // Filename for requests 

    /**
     * Sends a read or write request to the server based on the request number. 
     * Request types alternate between read and write, except for the 10th request, which is invalid.
     * @param requestNum The request number to determine request type.
     */
    void sendRequest(int requestNum) {
        const std::string mode = "netascii";
        
        std::vector<uint8_t> packet;
        if (requestNum == 10) {
            packet = {0, 3, 'i', 'n', 'v', 'a', 'l', 'i', 'd'};
        } else if (requestNum % 2 == 0) {
            packet = Datagram::createRequest(filename, mode, true);
        } else {
            packet = Datagram::createRequest(filename, mode, false);
        }
        
        std::cout << "\nSending request #" << (requestNum + 1) << std::endl;
        Datagram::printPacket(packet);
        
        int n = sendto(sockfd, packet.data(), packet.size(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        
        if (n < 0) {
            throw std::runtime_error("Error sending packet");
        }
    }

    /**
     * Receives a response from the server through the intermediate host and prints the received packet.
     * If no response is received within 3 seconds, the program exits.
     * @throws std::runtime_error if receiving fails.
     */
    void receiveResponse() {
        char buffer[1024];
        struct sockaddr_in responseAddr;
        socklen_t addrLen = sizeof(responseAddr);

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
                         (struct sockaddr*)&responseAddr, &addrLen);
        if (n < 0) {
            throw std::runtime_error("Error receiving response");
        }

        std::vector<uint8_t> response(buffer, buffer + n);
        std::cout << "Received response:" << std::endl;
        Datagram::printPacket(response);
    }
public:
    /**
     * Constructs a Client object and initializes the server address.
     * @param filename The name of the file to be requested from the server.
     */
    Client(std::string filename) : filename(filename) {
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50023);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        std::cout << "Client initialized" << std::endl;
    }

    /**
     * Runs the client by sending multiple requests and receiving responses.
     */
    void run() {
        for (int i = 0; i < 11; i++) {
            sendRequest(i);
            receiveResponse();
        }
    }
};

/**
 * Main function to start the UDP client.
 * @param argc Argument count.
 * @param argv Argument vector, expecting a filename as an argument.
 * @return Exit status code.
 */
int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <filename.txt>" << std::endl;
            return 1;
        }

        // Get filename
        std::string filename = argv[1];
        Client client(filename);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
