#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <sstream>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

const char* HOST = "127.0.0.1";
const int PORT = 5000;

std::map<SOCKET, std::string> clients;
std::mutex clients_mutex;

void broadcast(const std::string& message, SOCKET sender = INVALID_SOCKET) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& pair : clients) {
        if (pair.first != sender) {
            send(pair.first, message.c_str(), (int)message.size(), 0);
        }
    }
}

std::string process_command(const std::string& command) {
    std::istringstream iss(command);
    std::vector<std::string> parts;
    std::string token;
    while (iss >> token) parts.push_back(token);

    if (parts.empty()) return "Error: empty command\n";

    std::string cmd = parts[0];
    for (auto& c : cmd) c = toupper(c);

    if (cmd == "TIME") {
        time_t now = time(nullptr);
        return "Current time: " + std::string(ctime(&now));
    }
    else if (cmd == "ECHO") {
        if (parts.size() > 1) {
            std::ostringstream oss;
            for (size_t i = 1; i < parts.size(); ++i) oss << parts[i] << " ";
            oss << "\n";
            return oss.str();
        }
        return "Error: no text to echo\n";
    }
    else if (cmd == "ADD") {
        if (parts.size() != 3) return "Usage: /ADD <a> <b>\n";
        try {
            double a = std::stod(parts[1]);
            double b = std::stod(parts[2]);
            return "Result: " + std::to_string(a + b) + "\n";
        }
        catch (...) {
            return "Error: please provide numbers\n";
        }
    }
    else if (cmd == "WHO") {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string result = "Active users: ";
        bool first = true;
        for (auto& pair : clients) {
            if (!first) result += ", ";
            result += pair.second;
            first = false;
        }
        return result + "\n";
    }
    else if (cmd == "EXIT") {
        return "Disconnecting...\n";
    }

    return "Unknown command\n";
}

void handle_client(SOCKET clientSocket, sockaddr_in clientAddr) {
    char buffer[1024];

    send(clientSocket, "Enter your nickname: ", 21, 0);
    int len = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[len] = '\0';
    std::string nickname(buffer);
    nickname.erase(nickname.find_last_not_of("\r\n") + 1);

    if (nickname.empty()) {
        nickname = "User_" + std::to_string(ntohs(clientAddr.sin_port));
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[clientSocket] = nickname;
    }

    std::cout << "[+] " << nickname << " joined from "
        << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << "\n";

    std::string welcome =
        "Welcome to the server! Available commands: /TIME, /ECHO <text>, "
        "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
    send(clientSocket, welcome.c_str(), (int)welcome.size(), 0);

    broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

    while (true) {
        len = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;

        buffer[len] = '\0';
        std::string msg(buffer);
        msg.erase(msg.find_last_not_of("\r\n") + 1);

        if (msg.empty()) continue;

        if (msg[0] == '/') {
            std::string cmd = msg.substr(1);
            std::string response = process_command(cmd);
            send(clientSocket, response.c_str(), (int)response.size(), 0);

            if (cmd.size() >= 4 && _strnicmp(cmd.c_str(), "EXIT", 4) == 0)
                break;
        }
        else {
            std::cout << "[" << nickname << "] " << msg << "\n";
            broadcast("[" + nickname + "] " + msg + "\n", clientSocket);
        }
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(clientSocket);
    }

    broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
    std::cout << "[-] " << nickname << " disconnected\n";

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(HOST);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed!\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed!\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Chat server running on " << HOST << ":" << PORT << "\n";

    while (true) {
        sockaddr_in clientAddr{};
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed!\n";
            continue;
        }

        std::thread(handle_client, clientSocket, clientAddr).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
