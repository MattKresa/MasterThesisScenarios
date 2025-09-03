import socket
import threading
import datetime

HOST = "127.0.0.1"
PORT = 5000

# Dictionary: client_socket -> nickname
clients = {}
clients_lock = threading.Lock()  # For thread-safe access


def broadcast(message, sender_socket=None):
    """Send message to all clients except sender."""
    with clients_lock:
        for client in list(clients.keys()):
            if client != sender_socket:
                try:
                    client.sendall(message.encode())
                except:
                    pass  # Ignore broken connections


def add_client(sock, nickname):
    with clients_lock:
        clients[sock] = nickname


def remove_client(sock):
    with clients_lock:
        if sock in clients:
            del clients[sock]


def get_active_users():
    with clients_lock:
        return ", ".join(clients.values())


def handle_client(sock, addr):
    nickname = None
    try:
        sock.sendall(b"Enter your nickname: ")
        nickname = sock.recv(1024).decode().strip()
        if not nickname:
            nickname = f"User_{addr[1]}"

        add_client(sock, nickname)
        print(f"[+] {nickname} joined from {addr}")

        sock.sendall(
            b"Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n"
        )
        broadcast(f"*** {nickname} joined the chat ***\n", sock)

        while True:
            data = sock.recv(1024)
            if not data:
                break

            line = data.decode().strip()
            if not line:
                continue

            if line.startswith("/"):
                cmd = line[1:]
                response = process_command(cmd)
                sock.sendall(response.encode())
                if cmd.upper().startswith("EXIT"):
                    break
            else:
                print(f"[{nickname}] {line}")
                broadcast(f"[{nickname}] {line}\n", sock)

    except:
        pass
    finally:
        remove_client(sock)
        broadcast(f"*** {nickname} left the chat ***\n", sock)
        print(f"[-] {nickname} disconnected")
        try:
            sock.close()
        except:
            pass


def process_command(cmd):
    parts = cmd.split()
    if not parts:
        return "Error: empty command\n"

    command = parts[0].upper()

    if command == "TIME":
        return f"Current time: {datetime.datetime.now()}\n"

    elif command == "ECHO":
        return " ".join(parts[1:]) + "\n" if len(parts) > 1 else "Error: no text to echo\n"

    elif command == "ADD":
        if len(parts) != 3:
            return "Usage: /ADD <a> <b>\n"
        try:
            a = float(parts[1])
            b = float(parts[2])
            return f"Result: {a + b}\n"
        except ValueError:
            return "Error: please provide numbers\n"

    elif command == "WHO":
        return f"Active users: {get_active_users()}\n"

    elif command == "EXIT":
        return "Disconnecting...\n"

    else:
        return "Unknown command\n"


def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((HOST, PORT))
    server_socket.listen()
    print(f"Chat server running on {HOST}:{PORT}")

    try:
        while True:
            client_socket, addr = server_socket.accept()
            threading.Thread(target=handle_client, args=(client_socket, addr), daemon=True).start()
    except KeyboardInterrupt:
        print("\nServer shutting down...")
    finally:
        server_socket.close()


if __name__ == "__main__":
    main()
