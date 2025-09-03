import socket
import threading
import time
from datetime import datetime

HOST = "127.0.0.1"
PORT = 5000

clients = {}  # socket -> nickname mapping
clients_lock = threading.Lock()


def broadcast(message, sender=None):
    """Send message to all connected clients except sender"""
    with clients_lock:
        disconnected = []
        for client_socket in clients:
            if client_socket != sender:
                try:
                    client_socket.send(message.encode('utf-8'))
                except:
                    disconnected.append(client_socket)

        # Clean up disconnected clients
        for client in disconnected:
            if client in clients:
                del clients[client]


def process_command(command):
    """Process server commands"""
    parts = command.strip().split()

    if not parts:
        return "Error: empty command\n"

    cmd = parts[0].upper()

    if cmd == "TIME":
        current_time = datetime.now().strftime("%a %b %d %H:%M:%S %Y")
        return f"Current time: {current_time}\n"

    elif cmd == "ECHO":
        if len(parts) > 1:
            echo_text = " ".join(parts[1:])
            return f"{echo_text}\n"
        return "Error: no text to echo\n"

    elif cmd == "ADD":
        if len(parts) != 3:
            return "Usage: /ADD <a> <b>\n"
        try:
            a = float(parts[1])
            b = float(parts[2])
            result = a + b
            return f"Result: {result}\n"
        except ValueError:
            return "Error: please provide numbers\n"

    elif cmd == "WHO":
        with clients_lock:
            if clients:
                nicknames = list(clients.values())
                return f"Active users: {', '.join(nicknames)}\n"
            else:
                return "Active users: none\n"

    elif cmd == "EXIT":
        return "Disconnecting...\n"

    return "Unknown command\n"


def handle_client(client_socket, client_address):
    """Handle individual client connection"""
    nickname = None

    try:
        # Get nickname
        client_socket.send(b"Enter your nickname: ")
        nickname_data = client_socket.recv(1024).decode('utf-8').strip()

        if not nickname_data:
            nickname = f"User_{client_address[1]}"
        else:
            nickname = nickname_data

        # Add client to clients dictionary
        with clients_lock:
            clients[client_socket] = nickname

        print(f"[+] {nickname} joined from {client_address[0]}:{client_address[1]}")

        # Send welcome message
        welcome = ("Welcome to the server! Available commands: /TIME, /ECHO <text>, "
                   "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n")
        client_socket.send(welcome.encode('utf-8'))

        # Broadcast join message
        broadcast(f"*** {nickname} joined the chat ***\n", client_socket)

        # Main message loop
        while True:
            try:
                data = client_socket.recv(1024)
                if not data:
                    break

                message = data.decode('utf-8').strip()
                if not message:
                    continue

                if message.startswith('/'):
                    # Process command
                    cmd = message[1:]
                    response = process_command(cmd)
                    client_socket.send(response.encode('utf-8'))

                    # Check for exit command
                    if cmd.upper().startswith('EXIT'):
                        break
                else:
                    # Regular chat message
                    print(f"[{nickname}] {message}")
                    broadcast(f"[{nickname}] {message}\n", client_socket)

            except ConnectionResetError:
                break
            except Exception as e:
                print(f"Error handling client {nickname}: {e}")
                break

    except Exception as e:
        print(f"Error in client handler: {e}")

    finally:
        # Clean up
        if nickname:
            with clients_lock:
                if client_socket in clients:
                    del clients[client_socket]

            broadcast(f"*** {nickname} left the chat ***\n", client_socket)
            print(f"[-] {nickname} disconnected")

        try:
            client_socket.close()
        except:
            pass


def main():
    """Main server function"""
    # Create server socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        # Bind and listen
        server_socket.bind((HOST, PORT))
        server_socket.listen(5)  # SOMAXCONN equivalent

        print(f"Chat server running on {HOST}:{PORT}")

        while True:
            try:
                # Accept new connections
                client_socket, client_address = server_socket.accept()

                # Create new thread for each client
                client_thread = threading.Thread(
                    target=handle_client,
                    args=(client_socket, client_address)
                )
                client_thread.daemon = True
                client_thread.start()

            except Exception as e:
                print(f"Error accepting connection: {e}")
                continue

    except KeyboardInterrupt:
        print("\nShutting down server...")
    except Exception as e:
        print(f"Server error: {e}")
    finally:
        server_socket.close()


if __name__ == "__main__":
    main()