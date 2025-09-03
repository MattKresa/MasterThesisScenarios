#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <sstream>
#include <ctime>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

class ChatServer {
private:
    static const std::string HOST;
    static const int PORT;
    static std::map<SOCKET, std::string> clients;
    static std::mutex clientsMutex;
    static bool serverRunning;

public:
    static void start() {
        WSADATA wsaData;
        SOCKET serverSocket = INVALID_SOCKET;
        sockaddr_in serverAddr;

        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed" << std::endl;
            return;
        }

        // Create server socket
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        // Bind socket
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = inet_addr(HOST.c_str());
        serverAddr.sin_port = htons(PORT);

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        // Listen for connections
        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::cout << "Chat server running on " << HOST << ":" << PORT << std::endl;
        serverRunning = true;

        // Accept client connections
        while (serverRunning) {
            SOCKET clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                continue;
            }

            std::thread clientThread(handleClient, clientSocket);
            clientThread.detach();
        }

        // Cleanup
        closesocket(serverSocket);
        WSACleanup();
    }

    static void stop() {
        serverRunning = false;
    }

    static void broadcast(const std::string& message, SOCKET sender = INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (const auto& client : clients) {
            if (client.first != sender) {
                send(client.first, message.c_str(), (int)message.length(), 0);
            }
        }
    }

    static void addClient(SOCKET socket, const std::string& nickname) {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients[socket] = nickname;
    }

    static void removeClient(SOCKET socket) {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(socket);
    }

    static std::string getActiveUsers() {
        std::lock_guard<std::mutex> lock(clientsMutex);
        std::string result;
        for (const auto& client : clients) {
            if (!result.empty()) result += ", ";
            result += client.second;
        }
        return result;
    }

private:
    static void handleClient(SOCKET clientSocket) {
        std::string nickname;
        char buffer[1024];
        int bytesReceived;

        try {
            // Get nickname
            std::string prompt = "Enter your nickname: ";
            send(clientSocket, prompt.c_str(), (int)prompt.length(), 0);

            bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) throw std::runtime_error("Connection closed");

            nickname = std::string(buffer, bytesReceived);
            nickname = nickname.substr(0, nickname.find_first_of("\r\n"));

            if (nickname.empty()) {
                nickname = "User_" + std::to_string(clientSocket);
            }

            addClient(clientSocket, nickname);
            std::cout << "[+] " << nickname << " joined" << std::endl;

            // Send welcome message
            std::string welcomeMsg = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n";
            send(clientSocket, welcomeMsg.c_str(), (int)welcomeMsg.length(), 0);

            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            // Handle client messages
            while (true) {
                bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) break;

                std::string message(buffer, bytesReceived);
                message = message.substr(0, message.find_first_of("\r\n"));

                if (message.empty()) continue;

                if (message[0] == '/') {
                    std::string cmd = message.substr(1);
                    std::string response = processCommand(cmd);
                    send(clientSocket, response.c_str(), (int)response.length(), 0);

                    if (cmd.substr(0, 4) == "EXIT") break;
                }
                else {
                    std::cout << "[" << nickname << "] " << message << std::endl;
                    broadcast("[" + nickname + "] " + message + "\n", clientSocket);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error with client " << nickname << ": " << e.what() << std::endl;
        }

        // Cleanup
        removeClient(clientSocket);
        std::string leaveMsg = "*** " + nickname + " left the chat ***\n";
        broadcast(leaveMsg, clientSocket);
        std::cout << "[-] " << nickname << " disconnected" << std::endl;
        closesocket(clientSocket);
    }

    static std::string processCommand(const std::string& cmd) {
        std::vector<std::string> parts;
        std::istringstream iss(cmd);
        std::string part;

        while (iss >> part) {
            parts.push_back(part);
        }

        if (parts.empty()) return "Error: empty command\n";

        std::string command = parts[0];
        std::transform(command.begin(), command.end(), command.begin(), ::toupper);

        if (command == "TIME") {
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "Current time: " << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "\n";
            return ss.str();
        }
        else if (command == "ECHO") {
            if (parts.size() > 1) {
                std::string echoText;
                for (size_t i = 1; i < parts.size(); ++i) {
                    if (i > 1) echoText += " ";
                    echoText += parts[i];
                }
                return echoText + "\n";
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
            catch (...) {
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
};

// Initialize static members
const std::string ChatServer::HOST = "127.0.0.1";
const int ChatServer::PORT = 5000;
std::map<SOCKET, std::string> ChatServer::clients;
std::mutex ChatServer::clientsMutex;
bool ChatServer::serverRunning = true;

int main() {
    ChatServer::start();
    return 0;
}