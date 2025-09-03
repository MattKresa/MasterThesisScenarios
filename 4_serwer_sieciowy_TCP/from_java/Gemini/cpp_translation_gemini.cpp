#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <winsock2.h> // For Winsock functions
#include <ws2tcpip.h> // For additional Winsock types like sockaddr_in
#include <chrono>     // For std::chrono
#include <sstream>    // For stringstream
#include <algorithm>  // For std::transform

#pragma comment(lib, "Ws2_32.lib") // Link with Ws2_32.lib

// Global constants
const std::string HOST = "127.0.0.1";
const int PORT = 5000;
const int BUFFER_SIZE = 1024;

// Global map to store connected clients: {socket_fd: nickname}
// Using std::map as a concurrent map equivalent with a mutex
std::map<SOCKET, std::string> clients;
std::mutex clients_mutex; // Mutex to protect the clients map

// Forward declarations
class ClientHandler;
class ChatServer;

class ChatServer {
public:
    // Adds a client to the map
    static void addClient(SOCKET socket, const std::string& nickname) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[socket] = nickname;
    }

    // Removes a client from the map and returns their nickname
    static std::string removeClient(SOCKET socket) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        auto it = clients.find(socket);
        if (it != clients.end()) {
            std::string nickname = it->second;
            clients.erase(it);
            return nickname;
        }
        return ""; // Return empty string if not found
    }

    // Returns a comma-separated string of active user nicknames
    static std::string getActiveUsers() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string activeUsers;
        bool first = true;
        for (const auto& pair : clients) {
            if (!first) {
                activeUsers += ", ";
            }
            activeUsers += pair.second;
            first = false;
        }
        return activeUsers;
    }

    // Sends a message to all clients (excluding the sender)
    static void broadcast(const std::string& message, SOCKET sender = INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& pair : clients) {
            SOCKET clientSocket = pair.first;
            if (clientSocket != sender) {
                // Ensure the message is null-terminated for send (though send can take size)
                // Also, convert string to const char*
                send(clientSocket, message.c_str(), message.length(), 0);
            }
        }
    }

    void start() {
        WSADATA wsaData;
        int iResult;

        // Initialize Winsock
        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
            return;
        }

        SOCKET listenSocket = INVALID_SOCKET;
        struct addrinfo* result = NULL, hints;

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;       // IPv4
        hints.ai_socktype = SOCK_STREAM; // Stream socket (TCP)
        hints.ai_protocol = IPPROTO_TCP; // TCP protocol
        hints.ai_flags = AI_PASSIVE;     // For binding

        // Resolve the server address and port
        iResult = getaddrinfo(HOST.c_str(), std::to_string(PORT).c_str(), &hints, &result);
        if (iResult != 0) {
            std::cerr << "getaddrinfo failed: " << iResult << std::endl;
            WSACleanup();
            return;
        }

        // Create a SOCKET for the server to listen for client connections
        listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        // Setup the TCP listening socket
        iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            std::cerr << "bind failed with error: " << WSAGetLastError() << std::endl;
            freeaddrinfo(result);
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        freeaddrinfo(result); // No longer needed

        iResult = listen(listenSocket, SOMAXCONN); // Listen for incoming connection
        if (iResult == SOCKET_ERROR) {
            std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        std::cout << "Chat server running on " << HOST << ":" << PORT << std::endl;

        while (true) {
            SOCKET clientSocket = accept(listenSocket, NULL, NULL); // Accept a client socket
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "accept failed with error: " << WSAGetLastError() << std::endl;
                closesocket(listenSocket);
                WSACleanup();
                return;
            }

            // Create a new thread for each client
            std::thread clientThread(&ClientHandler::run, ClientHandler(clientSocket));
            clientThread.detach(); // Detach the thread so it runs independently
        }

        // Cleanup (this part is unreachable in the current infinite loop,
        // but important for proper shutdown if loop conditions change)
        closesocket(listenSocket);
        WSACleanup();
    }
};

class ClientHandler {
private:
    SOCKET clientSocket;

    // Helper to read a line from the socket (like Java's readLine)
    std::string readLineFromSocket() {
        std::string line;
        char buffer[1]; // Read character by character
        int bytesReceived;

        while ((bytesReceived = recv(clientSocket, buffer, 1, 0)) > 0) {
            if (buffer[0] == '\n') {
                // If it's Windows line ending, consume '\r' if present
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back(); 
                }
                break; // End of line
            }
            line += buffer[0];
        }
        return line;
    }

    // Helper to send a string
    void sendToSocket(const std::string& message) {
        send(clientSocket, message.c_str(), message.length(), 0);
    }

    std::string processCommand(const std::string& cmd) {
        std::vector<std::string> parts;
        std::stringstream ss(cmd);
        std::string segment;
        while (ss >> segment) {
            parts.push_back(segment);
        }

        if (parts.empty()) return "Error: empty command\n";

        // Convert command to uppercase for comparison
        std::string command_upper = parts[0];
        std::transform(command_upper.begin(), command_upper.end(), command_upper.begin(), ::toupper);

        if (command_upper == "TIME") {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            return "Current time: " + std::string(std::ctime(&now_c)); // ctime adds a newline already
        } else if (command_upper == "ECHO") {
            if (parts.size() > 1) {
                std::string echoedText;
                for (size_t i = 1; i < parts.size(); ++i) {
                    echoedText += parts[i];
                    if (i < parts.size() - 1) {
                        echoedText += " ";
                    }
                }
                return echoedText + "\n";
            } else {
                return "Error: no text to echo\n";
            }
        } else if (command_upper == "ADD") {
            if (parts.size() != 3) return "Usage: /ADD <a> <b>\n";
            try {
                double a = std::stod(parts[1]);
                double b = std::stod(parts[2]);
                return "Result: " + std::to_string(a + b) + "\n";
            } catch (const std::invalid_argument& e) {
                return "Error: please provide numbers\n";
            } catch (const std::out_of_range& e) {
                return "Error: number out of range\n";
            }
        } else if (command_upper == "WHO") {
            return "Active users: " + ChatServer::getActiveUsers() + "\n";
        } else if (command_upper == "EXIT") {
            return "Disconnecting...\n";
        } else {
            return "Unknown command\n";
        }
    }

public:
    ClientHandler(SOCKET sock) : clientSocket(sock) {}

    void run() {
        std::string nickname = "";
        char buffer[BUFFER_SIZE];
        int bytesReceived;

        try {
            // Prompt for nickname
            sendToSocket("Enter your nickname: ");
            nickname = readLineFromSocket();
            if (nickname.empty() || nickname.find_first_not_of(" \t\n\r") == std::string::npos) {
                // Get remote port for default nickname
                sockaddr_in addr;
                int addrLen = sizeof(addr);
                getpeername(clientSocket, (sockaddr*)&addr, &addrLen);
                nickname = "User_" + std::to_string(ntohs(addr.sin_port));
            }

            ChatServer::addClient(clientSocket, nickname);

            sockaddr_in clientAddr;
            int clientAddrLen = sizeof(clientAddr);
            getpeername(clientSocket, (sockaddr*)&clientAddr, &clientAddrLen);
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            std::cout << "[+] " << nickname << " joined from " << ipStr << ":" << ntohs(clientAddr.sin_port) << std::endl;

            sendToSocket("Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n");
            ChatServer::broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            std::string line;
            while (true) {
                line = readLineFromSocket();
                if (line.empty() && WSAGetLastError() != 0) break; // Client disconnected or error

                line.erase(0, line.find_first_not_of(" \t\n\r")); // Trim leading whitespace
                line.erase(line.find_last_not_of(" \t\n\r") + 1); // Trim trailing whitespace

                if (line.empty()) continue;

                if (line.rfind("/", 0) == 0) { // Check if it starts with "/"
                    std::string cmd = line.substr(1);
                    std::string response = processCommand(cmd);
                    sendToSocket(response);
                    if (cmd.length() >= 4 && cmd.substr(0, 4) == "EXIT") break; // Check for EXIT command
                } else {
                    std::cout << "[" << nickname << "] " << line << std::endl;
                    ChatServer::broadcast("[" + nickname + "] " + line + "\n", clientSocket);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in ClientHandler for " << nickname << ": " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error in ClientHandler for " << nickname << std::endl;
        }
        finally {
            std::string disconnectedNickname = ChatServer::removeClient(clientSocket);
            if (!disconnectedNickname.empty()) {
                ChatServer::broadcast("*** " + disconnectedNickname + " left the chat ***\n", clientSocket);
                std::cout << "[-] " << disconnectedNickname << " disconnected" << std::endl;
            } else {
                std::cout << "[-] Unknown client disconnected" << std::endl;
            }
            closesocket(clientSocket);
        }
    }
};

int main() {
    ChatServer server;
    server.start();
    return 0;
}