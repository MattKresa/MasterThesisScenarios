import socket
import threading
from datetime import datetime
from concurrent.futures import ThreadPoolExecutor


class ChatServer:
    HOST = "127.0.0.1"
    PORT = 5000

    def __init__(self):
        self.clients = {}  # socket: nickname
        self.lock = threading.Lock()

    def start(self):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind((self.HOST, self.PORT))
            server_socket.listen()
            print(f"Chat server running on {self.HOST}:{self.PORT}")

            with ThreadPoolExecutor() as executor:
                while True:
                    client_socket, addr = server_socket.accept()
                    executor.submit(self.client_handler, client_socket)

    def broadcast(self, message, sender_socket=None):
        with self.lock:
            for client in self.clients:
                if client != sender_socket:
                    try:
                        client.send(message.encode())
                    except:
                        pass

    def add_client(self, client_socket, nickname):
        with self.lock:
            self.clients[client_socket] = nickname

    def remove_client(self, client_socket):
        with self.lock:
            if client_socket in self.clients:
                nickname = self.clients[client_socket]
                del self.clients[client_socket]
                return nickname
            return None

    def get_active_users(self):
        with self.lock:
            return ", ".join(self.clients.values())

    def client_handler(self, client_socket):
        nickname = None
        try:
            client_socket.send("Enter your nickname: ".encode())
            nickname = client_socket.recv(1024).decode().strip()

            if not nickname:
                nickname = f"User_{client_socket.getpeername()[1]}"

            self.add_client(client_socket, nickname)
            print(f"[+] {nickname} joined from {client_socket.getpeername()}")

            welcome_msg = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n"
            client_socket.send(welcome_msg.encode())

            self.broadcast(f"*** {nickname} joined the chat ***\n", client_socket)

            while True:
                data = client_socket.recv(1024)
                if not data:
                    break

                line = data.decode().strip()
                if not line:
                    continue

                if line.startswith("/"):
                    cmd = line[1:]
                    response = self.process_command(cmd)
                    client_socket.send(response.encode())

                    if cmd.upper().startswith("EXIT"):
                        break
                else:
                    print(f"[{nickname}] {line}")
                    self.broadcast(f"[{nickname}] {line}\n", client_socket)

        except Exception as e:
            print(f"Error with client {nickname}: {e}")
        finally:
            nickname = self.remove_client(client_socket)
            if nickname:
                self.broadcast(f"*** {nickname} left the chat ***\n", client_socket)
                print(f"[-] {nickname} disconnected")
            client_socket.close()

    def process_command(self, cmd):
        parts = cmd.split()
        if not parts:
            return "Error: empty command\n"

        command = parts[0].upper()

        if command == "TIME":
            return f"Current time: {datetime.now()}\n"

        elif command == "ECHO":
            if len(parts) > 1:
                return " ".join(parts[1:]) + "\n"
            else:
                return "Error: no text to echo\n"

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
            return f"Active users: {self.get_active_users()}\n"

        elif command == "EXIT":
            return "Disconnecting...\n"

        else:
            return "Unknown command\n"


if __name__ == "__main__":
    server = ChatServer()
    server.start()