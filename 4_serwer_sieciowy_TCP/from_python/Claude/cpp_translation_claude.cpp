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
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

const std::string HOST = "127.0.0.1";
const int PORT = 5000;

std::map<SOCKET, std::string> clients; // socket -> nickname
std::mutex clients_mutex;

// Initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;
}

// Cleanup Winsock
void cleanupWinsock() {
    WSACleanup();
}

// Safe socket send
bool safeSend(SOCKET sock, const std::string& message) {
    int result = send(sock, message.c_str(), static_cast<int>(message.length()), 0);
    return result != SOCKET_ERROR;
}

// Safe socket receive
std::string safeReceive(SOCKET sock, int bufferSize = 1024) {
    std::vector<char> buffer(bufferSize);
    int result = recv(sock, buffer.data(), bufferSize - 1, 0);
    if (result > 0) {
        buffer[result] = '\0';
        std::string received(buffer.data());
        // Remove trailing whitespace
        received.erase(received.find_last_not_of(" \t\n\r\f\v") + 1);
        return received;
    }
    return "";
}

// Broadcast message to all clients except sender
void broadcast(const std::string& message, SOCKET senderSocket = INVALID_SOCKET) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& client : clients) {
        if (client.first != senderSocket) {
            safeSend(client.first, message);
        }
    }
}

// Get current timestamp as string
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Split string by whitespace
std::vector<std::string> split(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Convert string to uppercase
std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Process commands
std::string processCommand(const std::string& command, SOCKET conn) {
    std::vector<std::string> parts = split(command);

    if (parts.empty()) {
        return "Error: empty command\n";
    }

    std::string cmd = toUpper(parts[0]);

    if (cmd == "TIME") {
        return "Current time: " + getCurrentTime() + "\n";
    }
    else if (cmd == "ECHO") {
        if (parts.size() > 1) {
            std::string result;
            for (size_t i = 1; i < parts.size(); ++i) {
                if (i > 1) result += " ";
                result += parts[i];
            }
            return result + "\n";
        }
        return "Error: no text to echo\n";
    }
    else if (cmd == "ADD") {
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
    else if (cmd == "WHO") {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string result = "Active users: ";
        bool first = true;
        for (const auto& client : clients) {
            if (!first) result += ", ";
            result += client.second;
            first = false;
        }
        return result + "\n";
    }
    else if (cmd == "EXIT") {
        return "Disconnecting...\n";
    }

    return "Unknown command\n";
}

// Handle individual client connection
void handleClient(SOCKET clientSocket, const std::string& clientAddr) {
    // Get nickname
    safeSend(clientSocket, "Enter your nickname: ");
    std::string nickname = safeReceive(clientSocket);

    if (nickname.empty()) {
        nickname = "User_" + clientAddr;
    }

    // Add client to map
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[clientSocket] = nickname;
    }

    std::cout << "[+] " << nickname << " joined from " << clientAddr << std::endl;

    safeSend(clientSocket, "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n");
    broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

    try {
        while (true) {
            std::string data = safeReceive(clientSocket);
            if (data.empty()) {
                break;
            }

            if (data[0] == '/') { // Command processing
                std::string cmd = data.substr(1);
                std::string response = processCommand(cmd, clientSocket);
                safeSend(clientSocket, response);

                if (toUpper(cmd.substr(0, 4)) == "EXIT") {
                    break;
                }
            }
            else { // Regular message
                std::cout << "[" << nickname << "] " << data << std::endl;
                broadcast("[" + nickname + "] " + data + "\n", clientSocket);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client " << nickname << ": " << e.what() << std::endl;
    }

    // Cleanup
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(clientSocket);
    }

    broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
    std::cout << "[-] " << nickname << " disconnected" << std::endl;

    closesocket(clientSocket);
}

// Start the server
void startServer() {
    if (!initializeWinsock()) {
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        cleanupWinsock();
        return;
    }

    // Set socket options
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &serverAddr.sin_addr);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        cleanupWinsock();
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        cleanupWinsock();
        return;
    }

    std::cout << "Chat server running on " << HOST << ":" << PORT << std::endl;

    std::vector<std::thread> clientThreads;

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);

        SOCKET clientSocket = accept(serverSocket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrSize);

        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::string clientAddrStr = std::string(clientIP) + ":" +
            std::to_string(ntohs(clientAddr.sin_port));

        // Create thread to handle client
        clientThreads.emplace_back(handleClient, clientSocket, clientAddrStr);
        clientThreads.back().detach(); // Detach thread to run independently
    }

    closesocket(serverSocket);
    cleanupWinsock();
}

int main() {
    try {
        startServer();
    }
    catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}