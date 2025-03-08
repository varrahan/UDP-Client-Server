#include "datagram.h"

/**
 * A UDP-based intermediate host for RPC communication between client and server
 */
class Host : private Socket {
private:
    struct sockaddr_in serverAddr;      // Server address
    std::mutex mtx;                     // Mutex
    std::condition_variable cv;         // Condition variable
    std::queue<std::vector<uint8_t>> clientToServerQueue;  // Queue for client to server 
    std::queue<std::vector<uint8_t>> serverToClientQueue;  // Queue for server to client
    std::atomic<bool> running;          // Flag for thread execution
    std::thread clientToServerThread; // Thread from client to server
    std::thread serverToClientThread; // Thread from server to client
    
    /**
     * Thread function to handle client requests and forward them to the server.
     */
    void clientToServerWorker() {
        while (running) {
            char buffer[1024];
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            
            // Initialize timeout for socket
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            
            if (activity == 0) continue;
            if (activity < 0) {
                if (errno == EINTR) continue;
                perror("Select error in client-server thread");
                break;
            }
            // Receive client message
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                            (struct sockaddr*)&clientAddr, &clientLen);
            if (n < 0) {
                perror("Receive from client failed");
                continue;
            }
            std::vector<uint8_t> clientPacket(buffer, buffer + n);
            std::cout << "\nReceived from client:" << std::endl;
            Datagram::printPacket(clientPacket);
            std::vector<uint8_t> serverResponse;
            // Check if successful communication with server
            bool success = rpcSend(clientPacket, serverAddr, sizeof(serverAddr), serverResponse);
            if (!success) {
                std::cout << "Failed to communicate with server, sending error to client" << std::endl;
                serverResponse = {0, static_cast<uint8_t>(Datagram::Type::ERROR), 0, 1, 'S', 'e', 'r', 'v', 'e', 'r', ' ', 'e', 'r', 'r', 'o', 'r', 0};
            } else {
                std::cout << "Received from server:" << std::endl;
                Datagram::printPacket(serverResponse);
            }
            // Check if successful communication with client
            if (sendto(sockfd, serverResponse.data(), serverResponse.size(), 0,
                      (struct sockaddr*)&clientAddr, clientLen) < 0) {
                perror("Forward to client failed");
            } else {
                std::cout << "Forwarded server response to client" << std::endl;
            }
        }
    }
    
public:
    /**
     * Initializes the host for RPC communication.
     */
    Host() : running(true) {
        bind(50023);
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(50069);
        serverAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        std::cout << "Host initialized on port 50023, server on port 50069" << std::endl;
        clientToServerThread = std::thread(&Host::clientToServerWorker, this);
    }
    
    /**
     * Destructor for Host
     */
    ~Host() {
        running = false;
        if (clientToServerThread.joinable()) {
            clientToServerThread.join();
        }
        std::cout << "Host shut down" << std::endl;
    }

    /**
     * Run method to run threads
     */
    void run() {
        std::cout << "Host running" << std::endl;
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

/**
 * Main function to run the intermediate host
 * @return Exit
 */
int main() {
    Host host;
    host.run();
    return 0;
}