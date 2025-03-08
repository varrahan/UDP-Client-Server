#include "datagram.h"

/**
 * UDP client that communicates to server using intermediate host service with RPC communication
 */
class Client : private Socket {
private:
    struct sockaddr_in hostAddr;  // Intermediate host address structure 
    std::string filename;         // Filename for requests 

    /**
     * Sends a read or write request to the server via the host using RPC communication
     * @param requestNum The request number to determine request type
     * @return True if successful call, else false
     */
    bool sendRpcRequest(int requestNum) {
        const std::string mode = "netascii";
        std::vector<uint8_t> packet;
        if (requestNum == 10) {
            packet = Datagram::createInvalidPacket();
        } else if (requestNum % 2 == 0) {
            packet = Datagram::createRequest(filename, mode, true);  // Read request
        } else {
            packet = Datagram::createRequest(filename, mode, false);  // Write request
        }
        
        // Send RPC request
        std::cout << "\nSending RPC request #" << (requestNum + 1) << std::endl;
        Datagram::printPacket(packet);
        
        std::vector<uint8_t> response;
        bool success = rpcSend(packet, hostAddr, sizeof(hostAddr), response);
        
        // Check the success of the request
        if (success) {
            std::cout << "Received RPC response:" << std::endl;
            Datagram::printPacket(response);
            return true;
        } else {
            std::cerr << "RPC call failed for request #" << (requestNum + 1) << std::endl;
            return false;
        }
    }

public:
    /**
     * Constructs Client and initializes the intermediate host address
     * @param filename The name of the file to be requested.
     */
    Client(std::string filename) : filename(filename) {
        memset(&hostAddr, 0, sizeof(hostAddr));
        hostAddr.sin_family = AF_INET;
        hostAddr.sin_port = htons(50023);
        hostAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        bind(0);
        std::cout << "Client initialized" << std::endl;
    }

    /**
     * Runs the client by sending multiple RPC requests.
     */
    void run() {
        for (int i = 0; i < 11; i++) {
            if (!sendRpcRequest(i)) {
                std::cerr << "Failed to complete request #" << (i + 1) << ". Exiting." << std::endl;
                break;
            }
        }
    }
};

/**
 * Main function to start the UDP client
 * @param argc Argument count
 * @param argv Argument vector, expecting a txt filename as an argument
 * @return Exit
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Missing txt file arguement" << std::endl;
        return 1;
    }
    std::string filename = argv[1];
    Client client(filename);
    client.run();
    return 0;
}