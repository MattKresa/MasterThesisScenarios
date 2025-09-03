import socket
import threading
from threading import Lock
from datetime import datetime
import time

HOST = "127.0.0.1"
PORT = 5000

clients = {}
clients_lock = Lock()


def broadcast(message, sender=None):
    with clients_lock:
        for client_socket in clients:
            if client_socket != sender:
                try:
                    client_socket.send(message.encode('utf-8'))
                except:
                    # Handle client disconnection
                    pass


def process_command(command):
    parts = command.strip().split()
    if not parts:
        return "Error: empty command\n"

    cmd = parts[0].upper()

    if cmd == "TIME":
        return "Current time: " + datetime.now().strftime("%c") + "\n"
    elif cmd == "ECHO":
        if len(parts) > 1:
            return ' '.join(parts[1:]) + "\n"
        return "Error: no text to echo\n"
    elif cmd == "ADD":
        if len(parts) != 3:
            return "Usage: /ADD <a> <b>\n"
        try:
            a = float(parts[1])
            b = float(parts[2])
            return "Result: " + str(a + b) + "\n"
        except ValueError:
            return "Error: please provide numbers\n"
    elif cmd == "WHO":
        with clients_lock:
            if not clients:
                return "No active users\n"
            return "Active users: " + ', '.join(clients.values()) + "\n"
    elif cmd == "EXIT":
        return "Disconnecting...\n"

    return "Unknown command\n"


def handle_client(client_socket, client_addr):
    try:
        client_socket.send("Enter your nickname: ".encode('utf-8'))
        nickname = client_socket.recv(1024).decode('utf-8').strip()

        if not nickname:
            nickname = f"User_{client_addr[1]}"

        with clients_lock:
            clients[client_socket] = nickname

        print(f"[+] {nickname} joined from {client_addr[0]}:{client_addr[1]}")

        welcome = (
            "Welcome to the server! Available commands: /TIME, /ECHO <text>, "
            "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n"
        )
        client_socket.send(welcome.encode('utf-8'))

        broadcast(f"*** {nickname} joined the chat ***\n", client_socket)

        while True:
            try:
                msg = client_socket.recv(1024).decode('utf-8').strip()
                if not msg:
                    break

                if msg.startswith('/'):
                    cmd = msg[1:]
                    response = process_command(cmd)
                    client_socket.send(response.encode('utf-8'))

                    if cmd.upper().startswith("EXIT"):
                        break
                else:
                    print(f"[{nickname}] {msg}")
                    broadcast(f"[{nickname}] {msg}\n", client_socket)
            except:
                break

    finally:
        with clients_lock:
            if client_socket in clients:
                nickname = clients[client_socket]
                del clients[client_socket]

        broadcast(f"*** {nickname} left the chat ***\n", client_socket)
        print(f"[-] {nickname} disconnected")
        client_socket.close()


def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        print(f"Chat server running on {HOST}:{PORT}")

        while True:
            client_socket, client_addr = server_socket.accept()
            threading.Thread(
                target=handle_client,
                args=(client_socket, client_addr),
                daemon=True
            ).start()
    except KeyboardInterrupt:
        print("\nShutting down server...")
    finally:
        with clients_lock:
            for client in clients:
                client.close()
            clients.clear()
        server_socket.close()


if __name__ == "__main__":
    main()