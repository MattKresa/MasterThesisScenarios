#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include <sstream>
#include <ctime>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

const std::string HOST = "127.0.0.1";
const int PORT = 5000;

std::map<SOCKET, std::string> clients; // socket -> nickname
std::mutex clients_mutex;

void broadcast(const std::string& message, SOCKET sender) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto& [sock, nick] : clients) {
        if (sock != sender) {
            send(sock, message.c_str(), static_cast<int>(message.size()), 0);
        }
    }
}

void add_client(SOCKET sock, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients[sock] = nickname;
}

void remove_client(SOCKET sock) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(sock);
}

std::string get_active_users() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    std::string result;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        result += it->second;
        if (std::next(it) != clients.end()) result += ", ";
    }
    return result;
}

std::string process_command(const std::string& cmd) {
    std::istringstream iss(cmd);
    std::vector<std::string> parts;
    std::string token;
    while (iss >> token) parts.push_back(token);

    if (parts.empty()) return "Error: empty command\n";

    std::string command = parts[0];
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "TIME") {
        std::time_t now = std::time(nullptr);
        return "Current time: " + std::string(std::ctime(&now));
    }
    else if (command == "ECHO") {
        if (parts.size() > 1) {
            std::string msg;
            for (size_t i = 1; i < parts.size(); i++) {
                msg += parts[i] + " ";
            }
            msg += "\n";
            return msg;
        }
        return "Error: no text to echo\n";
    }
    else if (command == "ADD") {
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
    else if (command == "WHO") {
        return "Active users: " + get_active_users() + "\n";
    }
    else if (command == "EXIT") {
        return "Disconnecting...\n";
    }
    else {
        return "Unknown command\n";
    }
}

void handle_client(SOCKET client_sock, sockaddr_in client_addr) {
    char buffer[1024];
    std::string nickname;

    send(client_sock, "Enter your nickname: ", 22, 0);
    int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) { closesocket(client_sock); return; }
    buffer[len] = '\0';
    nickname = buffer;
    nickname.erase(std::remove(nickname.begin(), nickname.end(), '\n'), nickname.end());
    nickname.erase(std::remove(nickname.begin(), nickname.end(), '\r'), nickname.end());
    if (nickname.empty()) nickname = "User_" + std::to_string(ntohs(client_addr.sin_port));

    add_client(client_sock, nickname);
    std::cout << "[+] " << nickname << " joined from " << inet_ntoa(client_addr.sin_addr) << "\n";

    send(client_sock, "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n", 91, 0);

    broadcast("*** " + nickname + " joined the chat ***\n", client_sock);

    while (true) {
        len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;
        buffer[len] = '\0';
        std::string line(buffer);
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        if (line.empty()) continue;

        if (line[0] == '/') {
            std::string cmd = line.substr(1);
            std::string response = process_command(cmd);
            send(client_sock, response.c_str(), static_cast<int>(response.size()), 0);
            if (cmd.size() >= 4 && _strnicmp(cmd.c_str(), "EXIT", 4) == 0) break;
        }
        else {
            std::cout << "[" << nickname << "] " << line << "\n";
            broadcast("[" + nickname + "] " + line + "\n", client_sock);
        }
    }

    remove_client(client_sock);
    broadcast("*** " + nickname + " left the chat ***\n", client_sock);
    std::cout << "[-] " << nickname << " disconnected\n";
    closesocket(client_sock);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &server_addr.sin_addr);

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(server_sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Chat server running on " << HOST << ":" << PORT << "\n";

    while (true) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (sockaddr*)&client_addr, &addr_len);
        if (client_sock != INVALID_SOCKET) {
            std::thread(handle_client, client_sock, client_addr).detach();
        }
    }

    closesocket(server_sock);
    WSACleanup();
    return 0;
}
