import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 5000

clients = {}  # socket -> nickname
clients_lock = threading.Lock()


def broadcast(message, sender=None):
    with clients_lock:
        for sock in clients:
            if sock != sender:
                try:
                    sock.sendall(message.encode())
                except:
                    pass


def process_command(command):
    parts = command.strip().split()
    if not parts:
        return "Error: empty command\n"

    cmd = parts[0].upper()

    if cmd == "TIME":
        return "Current time: " + time.ctime() + "\n"

    elif cmd == "ECHO":
        if len(parts) > 1:
            return " ".join(parts[1:]) + "\n"
        return "Error: no text to echo\n"

    elif cmd == "ADD":
        if len(parts) != 3:
            return "Usage: /ADD <a> <b>\n"
        try:
            a = float(parts[1])
            b = float(parts[2])
            return f"Result: {a + b}\n"
        except ValueError:
            return "Error: please provide numbers\n"

    elif cmd == "WHO":
        with clients_lock:
            names = ", ".join(clients.values())
        return f"Active users: {names}\n"

    elif cmd == "EXIT":
        return "Disconnecting...\n"

    return "Unknown command\n"


def handle_client(sock, addr):
    try:
        sock.sendall(b"Enter your nickname: ")
        nickname = sock.recv(1024).decode().strip()
        if not nickname:
            nickname = f"User_{addr[1]}"

        with clients_lock:
            clients[sock] = nickname

        print(f"[+] {nickname} joined from {addr[0]}:{addr[1]}")

        welcome = (
            "Welcome to the server! Available commands: /TIME, /ECHO <text>, "
            "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to other users\n"
        )
        sock.sendall(welcome.encode())

        broadcast(f"*** {nickname} joined the chat ***\n", sock)

        while True:
            data = sock.recv(1024)
            if not data:
                break

            msg = data.decode().strip()
            if not msg:
                continue

            if msg.startswith("/"):
                cmd = msg[1:]
                response = process_command(cmd)
                sock.sendall(response.encode())
                if cmd.upper().startswith("EXIT"):
                    break
            else:
                print(f"[{nickname}] {msg}")
                broadcast(f"[{nickname}] {msg}\n", sock)

    except ConnectionResetError:
        pass
    finally:
        with clients_lock:
            if sock in clients:
                left_nick = clients[sock]
                del clients[sock]

        broadcast(f"*** {left_nick} left the chat ***\n", sock)
        print(f"[-] {left_nick} disconnected")
        sock.close()


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()

    print(f"Chat server running on {HOST}:{PORT}")

    while True:
        client_sock, client_addr = server.accept()
        threading.Thread(target=handle_client, args=(client_sock, client_addr), daemon=True).start()


if __name__ == "__main__":
    main()
