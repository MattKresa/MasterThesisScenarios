import socket
import threading
import datetime

HOST = "127.0.0.1"
PORT = 5000

clients = {}  # conn -> nickname
lock = threading.Lock()


def broadcast(message, sender_conn=None):
    with lock:
        for conn in clients:
            if conn != sender_conn:
                try:
                    conn.sendall(message.encode())
                except:
                    pass


def process_command(command: str, conn) -> str:
    parts = command.split()

    if not parts:
        return "Error: empty command\n"

    cmd = parts[0].upper()

    if cmd == "TIME":
        return f"Current time: {datetime.datetime.now()}\n"

    elif cmd == "ECHO":
        return f"{' '.join(parts[1:])}\n" if len(parts) > 1 else "Error: no text to echo\n"

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
        with lock:
            nicks = list(clients.values())
        return "Active users: " + ", ".join(nicks) + "\n"

    elif cmd == "EXIT":
        return "Disconnecting...\n"

    return "Unknown command\n"


def handle_client(conn, addr):
    with conn:
        conn.sendall(b"Enter your nickname: ")
        nickname = conn.recv(1024).decode().strip()
        if not nickname:
            nickname = f"User_{addr[1]}"

        with lock:
            clients[conn] = nickname

        print(f"[+] {nickname} joined from {addr}")
        conn.sendall(b"Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n")
        broadcast(f"*** {nickname} joined the chat ***\n", conn)

        try:
            while True:
                data = conn.recv(1024).decode().strip()
                if not data:
                    break

                if data.startswith("/"):  # commands start with /
                    cmd = data[1:]
                    response = process_command(cmd, conn)
                    conn.sendall(response.encode())

                    if cmd.upper().startswith("EXIT"):
                        break
                else:
                    print(f"[{nickname}] {data}")
                    broadcast(f"[{nickname}] {data}\n", conn)

        finally:
            with lock:
                del clients[conn]
            broadcast(f"*** {nickname} left the chat ***\n", conn)
            print(f"[-] {nickname} disconnected")


def start_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((HOST, PORT))
        server_socket.listen()
        print(f"Chat server running on {HOST}:{PORT}")

        while True:
            conn, addr = server_socket.accept()
            threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()


if __name__ == "__main__":
    start_server()
