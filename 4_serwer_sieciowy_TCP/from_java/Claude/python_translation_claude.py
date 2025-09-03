import socket
import threading
from datetime import datetime
from typing import Dict, Optional
import sys


class ChatServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 5000):
        self.host = host
        self.port = port
        # Dictionary to store socket -> nickname mapping (thread-safe with lock)
        self.clients: Dict[socket.socket, str] = {}
        self.clients_lock = threading.Lock()

    def start(self):
        """Start the chat server"""
        try:
            server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server_socket.bind((self.host, self.port))
            server_socket.listen()

            print(f"Chat server running on {self.host}:{self.port}")

            while True:
                client_socket, address = server_socket.accept()
                client_thread = threading.Thread(
                    target=self.handle_client,
                    args=(client_socket,)
                )
                client_thread.daemon = True
                client_thread.start()

        except KeyboardInterrupt:
            print("\nServer shutting down...")
        except Exception as e:
            print(f"Server error: {e}")
        finally:
            server_socket.close()

    def broadcast(self, message: str, sender: socket.socket):
        """Send message to all clients except sender"""
        with self.clients_lock:
            clients_to_remove = []
            for client_socket in self.clients.keys():
                if client_socket != sender:
                    try:
                        client_socket.send(message.encode('utf-8'))
                    except:
                        # Client disconnected, mark for removal
                        clients_to_remove.append(client_socket)

            # Remove disconnected clients
            for client in clients_to_remove:
                self.remove_client(client)

    def add_client(self, client_socket: socket.socket, nickname: str):
        """Add client to the clients dictionary"""
        with self.clients_lock:
            self.clients[client_socket] = nickname

    def remove_client(self, client_socket: socket.socket):
        """Remove client from the clients dictionary"""
        with self.clients_lock:
            if client_socket in self.clients:
                del self.clients[client_socket]

    def get_active_users(self) -> str:
        """Get list of active users"""
        with self.clients_lock:
            return ", ".join(self.clients.values())

    def handle_client(self, client_socket: socket.socket):
        """Handle individual client connection"""
        nickname = None
        try:
            # Get nickname from client
            client_socket.send(b"Enter your nickname: ")
            nickname = client_socket.recv(1024).decode('utf-8').strip()

            if not nickname:
                nickname = f"User_{client_socket.getpeername()[1]}"

            self.add_client(client_socket, nickname)
            print(f"[+] {nickname} joined from {client_socket.getpeername()}")

            # Send welcome message
            welcome_msg = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n"
            client_socket.send(welcome_msg.encode('utf-8'))

            # Broadcast join message
            self.broadcast(f"*** {nickname} joined the chat ***\n", client_socket)

            # Handle client messages
            while True:
                try:
                    message = client_socket.recv(1024).decode('utf-8').strip()
                    if not message:
                        break

                    if message.startswith('/'):
                        # Process command
                        cmd = message[1:]
                        response = self.process_command(cmd)
                        client_socket.send(response.encode('utf-8'))

                        if cmd.upper().startswith('EXIT'):
                            break
                    else:
                        # Regular chat message
                        print(f"[{nickname}] {message}")
                        self.broadcast(f"[{nickname}] {message}\n", client_socket)

                except socket.timeout:
                    continue
                except:
                    break

        except Exception as e:
            print(f"Error handling client {nickname}: {e}")
        finally:
            # Clean up
            self.remove_client(client_socket)
            if nickname:
                self.broadcast(f"*** {nickname} left the chat ***\n", client_socket)
                print(f"[-] {nickname} disconnected")
            try:
                client_socket.close()
            except:
                pass

    def process_command(self, cmd: str) -> str:
        """Process client commands"""
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


def main():
    server = ChatServer()
    server.start()


if __name__ == "__main__":
    main()