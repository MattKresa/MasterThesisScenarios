#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const string HOST = "127.0.0.1";
const int PORT = 5000;

map<SOCKET, string> clients; // socket fd -> nickname
mutex clients_mutex;

void broadcast(const string& message, SOCKET sender_fd = INVALID_SOCKET) {
    lock_guard<mutex> lock(clients_mutex);
    for (const auto& client : clients) {
        if (client.first != sender_fd) {
            send(client.first, message.c_str(), (int)message.length(), 0);
        }
    }
}

string process_command(const string& command, SOCKET conn) {
    istringstream iss(command);
    vector<string> parts;
    string part;

    while (iss >> part) {
        parts.push_back(part);
    }

    if (parts.empty()) {
        return "Error: empty command\n";
    }

    string cmd = parts[0];
    for (char& c : cmd) {
        c = toupper(c);
    }

    if (cmd == "TIME") {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        return string("Current time: ") + ctime(&now_time);
    }
    else if (cmd == "ECHO") {
        if (parts.size() > 1) {
            string echo_text;
            for (size_t i = 1; i < parts.size(); i++) {
                if (i > 1) echo_text += " ";
                echo_text += parts[i];
            }
            return echo_text + "\n";
        }
        else {
            return "Error: no text to echo\n";
        }
    }
    else if (cmd == "ADD") {
        if (parts.size() != 3) {
            return "Usage: /ADD <a> <b>\n";
        }
        try {
            double a = stod(parts[1]);
            double b = stod(parts[2]);
            return "Result: " + to_string(a + b) + "\n";
        }
        catch (...) {
            return "Error: please provide numbers\n";
        }
    }
    else if (cmd == "WHO") {
        lock_guard<mutex> lock(clients_mutex);
        string user_list = "Active users: ";
        bool first = true;
        for (const auto& client : clients) {
            if (!first) user_list += ", ";
            first = false;
            user_list += client.second;
        }
        return user_list + "\n";
    }
    else if (cmd == "EXIT") {
        return "Disconnecting...\n";
    }

    return "Unknown command\n";
}

void handle_client(SOCKET conn, sockaddr_in addr) {
    char buffer[1024];
    string nickname;

    // Get nickname
    string prompt = "Enter your nickname: ";
    send(conn, prompt.c_str(), (int)prompt.length(), 0);

    int bytes_received = recv(conn, buffer, sizeof(buffer), 0);
    if (bytes_received > 0) {
        nickname = string(buffer, bytes_received);
        // Remove trailing whitespace
        nickname.erase(nickname.find_last_not_of(" \n\r\t") + 1);
    }

    if (nickname.empty()) {
        nickname = "User_" + to_string(ntohs(addr.sin_port));
    }

    {
        lock_guard<mutex> lock(clients_mutex);
        clients[conn] = nickname;
    }

    cout << "[+] " << nickname << " joined from "
        << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << endl;

    string welcome_msg = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
    send(conn, welcome_msg.c_str(), (int)welcome_msg.length(), 0);

    broadcast("*** " + nickname + " joined the chat ***\n", conn);

    try {
        while (true) {
            bytes_received = recv(conn, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                break;
            }

            string data(buffer, bytes_received);
            // Remove trailing whitespace
            data.erase(data.find_last_not_of(" \n\r\t") + 1);

            if (data.empty()) {
                continue;
            }

            if (data[0] == '/') { // commands start with /
                string cmd = data.substr(1);
                string response = process_command(cmd, conn);
                send(conn, response.c_str(), (int)response.length(), 0);

                if (cmd.size() >= 4 && toupper(cmd[0]) == 'E' && toupper(cmd[1]) == 'X' &&
                    toupper(cmd[2]) == 'I' && toupper(cmd[3]) == 'T') {
                    break;
                }
            }
            else {
                cout << "[" << nickname << "] " << data << endl;
                broadcast("[" + nickname + "] " + data + "\n", conn);
            }
        }
    }
    catch (...) {
        // Handle any exceptions
    }

    {
        lock_guard<mutex> lock(clients_mutex);
        clients.erase(conn);
    }

    broadcast("*** " + nickname + " left the chat ***\n", conn);
    cout << "[-] " << nickname << " disconnected" << endl;
    closesocket(conn);
}

void start_server() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return;
    }

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Failed to create socket" << endl;
        WSACleanup();
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST.c_str(), &server_addr.sin_addr);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        cerr << "Listen failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    cout << "Chat server running on " << HOST << ":" << PORT << endl;

    while (true) {
        sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);

        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed" << endl;
            continue;
        }

        thread(handle_client, client_socket, client_addr).detach();
    }

    closesocket(server_socket);
    WSACleanup();
}

int main() {
    start_server();
    return 0;
}