import socket
import threading
import datetime
import concurrent.futures

HOST = "127.0.0.1"
PORT = 5000

# Dictionary to store connected clients: {socket: nickname}
clients = {}
clients_lock = threading.Lock() # To ensure thread-safe operations on the clients dictionary

class ChatServer:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.server_socket = None

    def start(self):
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen()
            print(f"Chat server running on {self.host}:{self.port}")

            while True:
                conn, addr = self.server_socket.accept()
                thread = threading.Thread(target=self.handle_client, args=(conn, addr))
                thread.start()

        except Exception as e:
            print(f"Server error: {e}")
        finally:
            self.shutdown()

    def shutdown(self):
        if self.server_socket:
            self.server_socket.close()
            print("Server shut down.")

    def broadcast(self, message, sender_socket=None):
        with clients_lock:
            for client_socket in clients:
                if client_socket != sender_socket:
                    try:
                        client_socket.sendall(message.encode())
                    except Exception as e:
                        print(f"Error broadcasting to a client: {e}")
                        self.remove_client(client_socket)

    def add_client(self, socket, nickname):
        with clients_lock:
            clients[socket] = nickname

    def remove_client(self, socket):
        with clients_lock:
            if socket in clients:
                nickname = clients[socket]
                del clients[socket]
                return nickname
            return None

    def get_active_users(self):
        with clients_lock:
            return ", ".join(clients.values())

    def handle_client(self, conn, addr):
        nickname = None
        try:
            conn.sendall(b"Enter your nickname: ")
            nickname = conn.recv(1024).decode().strip()
            if not nickname:
                nickname = f"User_{conn.getpeername()[1]}"

            self.add_client(conn, nickname)
            print(f"[+] {nickname} joined from {addr}")

            conn.sendall(b"Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n")
            self.broadcast(f"*** {nickname} joined the chat ***\n", conn)

            while True:
                data = conn.recv(1024).decode().strip()
                if not data:
                    break

                if data.startswith("/"):
                    response = self.process_command(data[1:])
                    conn.sendall(response.encode())
                    if data[1:].upper().startswith("EXIT"):
                        break
                else:
                    print(f"[{nickname}] {data}")
                    self.broadcast(f"[{nickname}] {data}\n", conn)

        except Exception as e:
            print(f"Client handler error for {nickname if nickname else 'unknown user'}: {e}")
        finally:
            if nickname:
                removed_nickname = self.remove_client(conn)
                if removed_nickname:
                    self.broadcast(f"*** {removed_nickname} left the chat ***\n", conn)
                    print(f"[-] {removed_nickname} disconnected")
            try:
                conn.close()
            except Exception as e:
                print(f"Error closing client socket: {e}")

    def process_command(self, cmd):
        parts = cmd.split(maxsplit=2)
        command = parts[0].upper()

        if command == "TIME":
            return f"Current time: {datetime.datetime.now()}\n"
        elif command == "ECHO":
            return f"{parts[1] if len(parts) > 1 else 'Error: no text to echo'}\n"
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
    server = ChatServer(HOST, PORT)
    server.start()