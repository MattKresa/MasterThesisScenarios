#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

class ChatServer {
private:
    static const std::string HOST;
    static const int PORT = 5000;

    // Map of sockets to nicknames (thread-safe with mutex)
    static std::map<SOCKET, std::string> clients;
    static std::mutex clients_mutex;

public:
    static void start() {
#ifdef _WIN32
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return;
        }
#endif

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        // Allow socket reuse
        int opt = 1;
        setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        inet_pton(AF_INET, HOST.c_str(), &serverAddr.sin_addr);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            closesocket(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            closesocket(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        std::cout << "Chat server running on " << HOST << ":" << PORT << std::endl;

        while (true) {
            sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);

            if (clientSocket != INVALID_SOCKET) {
                std::thread clientThread(handleClient, clientSocket);
                clientThread.detach();
            }
        }

        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
    }

    // Send message to all clients except sender
    static void broadcast(const std::string& message, SOCKET sender) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::vector<SOCKET> toRemove;

        for (auto& pair : clients) {
            if (pair.first != sender) {
                if (send(pair.first, message.c_str(), static_cast<int>(message.length()), 0) == SOCKET_ERROR) {
                    toRemove.push_back(pair.first);
                }
            }
        }

        // Remove disconnected clients
        for (SOCKET sock : toRemove) {
            clients.erase(sock);
        }
    }

    // Add client
    static void addClient(SOCKET socket, const std::string& nickname) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[socket] = nickname;
    }

    // Remove client
    static void removeClient(SOCKET socket) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(socket);
    }

    // Get list of active users
    static std::string getActiveUsers() {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string result;
        bool first = true;
        for (const auto& pair : clients) {
            if (!first) result += ", ";
            result += pair.second;
            first = false;
        }
        return result;
    }

private:
    // Handle individual client
    static void handleClient(SOCKET clientSocket) {
        std::string nickname;
        char buffer[1024];

        try {
            // Get nickname
            std::string prompt = "Enter your nickname: ";
            send(clientSocket, prompt.c_str(), static_cast<int>(prompt.length()), 0);

            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                nickname = std::string(buffer);
                // Remove newline characters
                nickname.erase(std::remove(nickname.begin(), nickname.end(), '\n'), nickname.end());
                nickname.erase(std::remove(nickname.begin(), nickname.end(), '\r'), nickname.end());

                if (nickname.empty() || std::all_of(nickname.begin(), nickname.end(), ::isspace)) {
                    nickname = "User_" + std::to_string(clientSocket);
                }
            }
            else {
                nickname = "User_" + std::to_string(clientSocket);
            }

            addClient(clientSocket, nickname);
            std::cout << "[+] " << nickname << " joined" << std::endl;

            // Send welcome message
            std::string welcome = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n";
            send(clientSocket, welcome.c_str(), static_cast<int>(welcome.length()), 0);

            // Broadcast join message
            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            // Handle messages
            while (true) {
                bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived <= 0) break;

                buffer[bytesReceived] = '\0';
                std::string line(buffer);

                // Remove newline characters
                line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);

                if (line.empty()) continue;

                if (line[0] == '/') {
                    // Process command
                    std::string cmd = line.substr(1);
                    std::string response = processCommand(cmd);
                    send(clientSocket, response.c_str(), static_cast<int>(response.length()), 0);

                    // Convert to uppercase for comparison
                    std::string cmdUpper = cmd;
                    std::transform(cmdUpper.begin(), cmdUpper.end(), cmdUpper.begin(), ::toupper);
                    if (cmdUpper.substr(0, 4) == "EXIT") break;
                }
                else {
                    // Regular chat message
                    std::cout << "[" << nickname << "] " << line << std::endl;
                    broadcast("[" + nickname + "] " + line + "\n", clientSocket);
                }
            }

        }
        catch (...) {
            // Handle any exceptions
        }

        // Cleanup
        removeClient(clientSocket);
        broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
        std::cout << "[-] " << nickname << " disconnected" << std::endl;
        closesocket(clientSocket);
    }

    // Process commands
    static std::string processCommand(const std::string& cmd) {
        std::vector<std::string> parts = split(cmd, ' ');
        if (parts.empty()) return "Error: empty command\n";

        std::string command = parts[0];
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "TIME") {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "Current time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "\n";
            return ss.str();
        }
        else if (command == "ECHO") {
            if (parts.size() > 1) {
                std::string result;
                for (size_t i = 1; i < parts.size(); ++i) {
                    if (i > 1) result += " ";
                    result += parts[i];
                }
                return result + "\n";
            }
            else {
                return "Error: no text to echo\n";
            }
        }
        else if (command == "ADD") {
            if (parts.size() != 3) {
                return "Usage: /ADD <a> <b>\n";
            }
            try {
                double a = std::stod(parts[1]);
                double b = std::stod(parts[2]);
                return "Result: " + std::to_string(a + b) + "\n";
            }
            catch (const std::exception&) {
                return "Error: please provide numbers\n";
            }
        }
        else if (command == "WHO") {
            return "Active users: " + getActiveUsers() + "\n";
        }
        else if (command == "EXIT") {
            return "Disconnecting...\n";
        }
        else {
            return "Unknown command\n";
        }
    }

    // Utility function to split string
    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter)) {
            // Trim whitespace
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }
};

// Static member definitions
const std::string ChatServer::HOST = "127.0.0.1";
std::map<SOCKET, std::string> ChatServer::clients;
std::mutex ChatServer::clients_mutex;

int main() {
    ChatServer::start();
    return 0;
}