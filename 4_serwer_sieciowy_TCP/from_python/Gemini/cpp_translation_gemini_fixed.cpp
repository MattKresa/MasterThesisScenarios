#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <map>
#include <mutex>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <algorithm> // For std::remove_if and std::transform
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // For inet_pton
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // For close
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

const char* HOST = "127.0.0.1";
const int PORT = 5000;

// Using a map to store client sockets and their nicknames
// The key is the SOCKET (or int on Linux), and the value is the nickname string.
std::map<SOCKET, std::string> clients;
std::mutex clients_mtx; // Mutex to protect access to the clients map

// Helper function to send data to a socket
void send_data(SOCKET sock, const std::string& message) {
    send(sock, message.c_str(), message.length(), 0);
}

// Broadcasts a message to all connected clients except the sender
void broadcast(const std::string& message, SOCKET sender_sock = INVALID_SOCKET) {
    std::lock_guard<std::mutex> guard(clients_mtx);
    for (auto const& [conn_sock, nickname] : clients) {
        if (conn_sock != sender_sock) {
            try {
                send_data(conn_sock, message);
            }
            catch (const std::exception& e) {
                // In a real application, you might want more robust error handling,
                // like trying to remove the client if the send fails consistently.
                std::cerr << "Error sending to client: " << e.what() << std::endl;
            }
        }
    }
}

// Processes commands like /TIME, /ECHO, /ADD, /WHO, /EXIT
std::string process_command(const std::string& command, SOCKET conn_sock) {
    std::istringstream iss(command);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part) {
        parts.push_back(part);
    }

    if (parts.empty()) {
        return "Error: empty command\n";
    }

    std::string cmd = parts[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper); // Convert to uppercase

    if (cmd == "TIME") {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* ptm = std::localtime(&now_c);
        std::ostringstream oss;
        oss << "Current time: " << std::put_time(ptm, "%Y-%m-%d %H:%M:%S") << "\n";
        return oss.str();
    }
    else if (cmd == "ECHO") {
        if (parts.size() < 2) {
            return "Error: no text to echo\n";
        }
        std::string echo_text;
        for (size_t i = 1; i < parts.size(); ++i) {
            echo_text += parts[i] + (i == parts.size() - 1 ? "" : " ");
        }
        return echo_text + "\n";
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
        catch (const std::invalid_argument& e) {
            return "Error: please provide numbers\n";
        }
        catch (const std::out_of_range& e) {
            return "Error: number out of range\n";
        }
    }
    else if (cmd == "WHO") {
        std::lock_guard<std::mutex> guard(clients_mtx);
        std::string active_users = "Active users: ";
        bool first = true;
        for (auto const& [sock, nickname] : clients) {
            if (!first) {
                active_users += ", ";
            }
            active_users += nickname;
            first = false;
        }
        return active_users + "\n";
    }
    else if (cmd == "EXIT") {
        return "Disconnecting...\n";
    }

    return "Unknown command\n";
}

// Handles a single client connection
void handle_client(SOCKET client_sock, sockaddr_in client_addr) {
    std::string nickname;
    char buffer[1024];
    int bytes_received;

    // Get client IP address for logging if needed
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    send_data(client_sock, "Enter your nickname: ");
    bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        nickname = std::string(buffer);
        // Remove leading/trailing whitespace
        nickname.erase(0, nickname.find_first_not_of(" \t\n\r\f\v"));
        nickname.erase(nickname.find_last_not_of(" \t\n\r\f\v") + 1);
    }

    if (nickname.empty()) {
        nickname = "User_" + std::to_string(ntohs(client_addr.sin_port));
    }

    {
        std::lock_guard<std::mutex> guard(clients_mtx);
        clients[client_sock] = nickname;
    }

    std::cout << "[+] " << nickname << " joined from " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;
    send_data(client_sock, "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n");
    broadcast("*** " + nickname + " joined the chat ***\n", client_sock);

    try {
        while (true) {
            bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) { // Client disconnected or error
                break;
            }
            buffer[bytes_received] = '\0';
            std::string data = std::string(buffer);
            // Remove leading/trailing whitespace
            data.erase(0, data.find_first_not_of(" \t\n\r\f\v"));
            data.erase(data.find_last_not_of(" \t\n\r\f\v") + 1);


            if (data.rfind("/", 0) == 0) { // Command starts with /
                std::string cmd = data.substr(1);
                std::string response = process_command(cmd, client_sock);
                send_data(client_sock, response);

                std::string upper_cmd = cmd;
                std::transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);
                if (upper_cmd.rfind("EXIT", 0) == 0) {
                    break;
                }
            }
            else {
                std::cout << "[" << nickname << "] " << data << std::endl;
                broadcast("[" + nickname + "] " + data + "\n", client_sock);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error handling client " << nickname << ": " << e.what() << std::endl;
    }

    {
        std::lock_guard<std::mutex> guard(clients_mtx);
        clients.erase(client_sock);
    }
#ifdef _WIN32
    closesocket(client_sock);
#else
    close(client_sock);
#endif
    broadcast("*** " + nickname + " left the chat ***\n", client_sock);
    std::cout << "[-] " << nickname << " disconnected" << std::endl;

}

// Starts the server
void start_server() {
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return;
    }
#endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " <<
#ifdef _WIN32
            WSAGetLastError()
#else
            "errno"
#endif
            << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
#ifdef _WIN32
    inet_pton(AF_INET, HOST, &(server_addr.sin_addr));
#else
    inet_pton(AF_INET, HOST, &(server_addr.sin_addr));
#endif

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket: " <<
#ifdef _WIN32
            WSAGetLastError()
#else
            "errno"
#endif
            << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#endif
        return;
    }

    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening on socket: " <<
#ifdef _WIN32
            WSAGetLastError()
#else
            "errno"
#endif
            << std::endl;
#ifdef _WIN32
        closesocket(server_socket);
        WSACleanup();
#endif
        return;
    }

    std::cout << "Chat server running on " << HOST << ":" << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        SOCKET client_sock = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_sock == INVALID_SOCKET) {
            std::cerr << "Error accepting connection: " <<
#ifdef _WIN32
                WSAGetLastError()
#else
                "errno"
#endif
                << std::endl;
            continue;
        }

        // Create a new thread to handle the client
        std::thread client_thread(handle_client, client_sock, client_addr);
        client_thread.detach(); // Detach the thread so it runs independently
    }

#ifdef _WIN32
    closesocket(server_socket);
    WSACleanup();
#else
    close(server_socket);
#endif
}

int main() {
    start_server();
    return 0;
}