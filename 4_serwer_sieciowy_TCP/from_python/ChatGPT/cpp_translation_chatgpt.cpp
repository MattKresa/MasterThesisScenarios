#include <iostream>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <vector>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#define HOST "127.0.0.1"
#define PORT 5000

std::map<SOCKET, std::string> clients;
std::mutex clients_mutex;

void broadcast(const std::string& message, SOCKET sender = INVALID_SOCKET) {
    std::lock_guard<std::mutex> guard(clients_mutex);
    for (const auto& pair : clients) {
        SOCKET sock = pair.first;
        if (sock != sender) {
            send(sock, message.c_str(), (int)message.size(), 0);
        }
    }
}

std::string process_command(const std::string& command, SOCKET conn) {
    std::istringstream iss(command);
    std::vector<std::string> parts;
    std::string token;
    while (iss >> token) parts.push_back(token);

    if (parts.empty()) return "Error: empty command\n";

    std::string cmd = parts[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "TIME") {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << "Current time: " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "\n";
        return oss.str();
    }
    else if (cmd == "ECHO") {
        if (parts.size() > 1) {
            std::ostringstream oss;
            for (size_t i = 1; i < parts.size(); ++i) {
                if (i > 1) oss << " ";
                oss << parts[i];
            }
            oss << "\n";
            return oss.str();
        }
        else {
            return "Error: no text to echo\n";
        }
    }
    else if (cmd == "ADD") {
        if (parts.size() != 3) return "Usage: /ADD <a> <b>\n";
        try {
            double a = std::stod(parts[1]);
            double b = std::stod(parts[2]);
            std::ostringstream oss;
            oss << "Result: " << (a + b) << "\n";
            return oss.str();
        }
        catch (...) {
            return "Error: please provide numbers\n";
        }
    }
    else if (cmd == "WHO") {
        std::lock_guard<std::mutex> guard(clients_mutex);
        std::ostringstream oss;
        oss << "Active users: ";
        bool first = true;
        for (const auto& pair : clients) {
            if (!first) oss << ", ";
            oss << pair.second;
            first = false;
        }
        oss << "\n";
        return oss.str();
    }
    else if (cmd == "EXIT") {
        return "Disconnecting...\n";
    }

    return "Unknown command\n";
}

void handle_client(SOCKET client_sock, sockaddr_in client_addr) {
    char buffer[1024];
    send(client_sock, "Enter your nickname: ", 22, 0);
    int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) { closesocket(client_sock); return; }
    buffer[bytes] = '\0';
    std::string nickname(buffer);
    nickname.erase(nickname.find_last_not_of("\r\n") + 1);

    if (nickname.empty()) {
        nickname = "User_" + std::to_string(ntohs(client_addr.sin_port));
    }

    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        clients[client_sock] = nickname;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));

    std::cout << "[+] " << nickname << " joined from " << ip_str
        << ":" << ntohs(client_addr.sin_port) << "\n";

    std::string welcome = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
    send(client_sock, welcome.c_str(), (int)welcome.size(), 0);
    broadcast("*** " + nickname + " joined the chat ***\n", client_sock);

    while (true) {
        bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        std::string data(buffer);
        data.erase(data.find_last_not_of("\r\n") + 1);

        if (data.empty()) break;

        if (data[0] == '/') {
            std::string cmd = data.substr(1);
            std::string response = process_command(cmd, client_sock);
            send(client_sock, response.c_str(), (int)response.size(), 0);

            std::string cmd_upper = cmd;
            std::transform(cmd_upper.begin(), cmd_upper.end(), cmd_upper.begin(), ::toupper);
            if (cmd_upper.rfind("EXIT", 0) == 0) break;
        }
        else {
            std::cout << "[" << nickname << "] " << data << "\n";
            broadcast("[" + nickname + "] " + data + "\n", client_sock);
        }
    }

    {
        std::lock_guard<std::mutex> guard(clients_mutex);
        clients.erase(client_sock);
    }
    broadcast("*** " + nickname + " left the chat ***\n", client_sock);
    std::cout << "[-] " << nickname << " disconnected\n";
    closesocket(client_sock);
}

void start_server() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        WSACleanup();
        return;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &server_addr.sin_addr);

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed\n";
        closesocket(server_sock);
        WSACleanup();
        return;
    }

    std::cout << "Chat server running on " << HOST << ":" << PORT << "\n";

    while (true) {
        sockaddr_in client_addr{};
        int addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_sock, (sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            std::cerr << "Accept failed\n";
            continue;
        }
        std::thread(handle_client, client_sock, client_addr).detach();
    }

    closesocket(server_sock);
    WSACleanup();
}

int main() {
    start_server();
    return 0;
}
