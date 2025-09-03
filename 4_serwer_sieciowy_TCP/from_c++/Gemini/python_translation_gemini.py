import socket
import threading
import datetime
import sys

# Server configuration
HOST = '127.0.0.1'
PORT = 5000

# Global dictionary to store connected clients: {socket_object: nickname}
clients = {}
clients_lock = threading.Lock()


def broadcast(message, sender_socket=None):
    """
    Sends a message to all connected clients except the sender.
    """
    with clients_lock:
        for client_socket, nickname in clients.items():
            if client_socket != sender_socket:
                try:
                    client_socket.sendall(message.encode('utf-8'))
                except Exception as e:
                    print(f"Error broadcasting to {nickname}: {e}")
                    # Consider removing the client if send fails consistently


def process_command(command):
    """
    Processes commands sent by clients.
    """
    parts = command.split()
    if not parts:
        return "Error: empty command\n"

    cmd = parts[0].upper()

    if cmd == "TIME":
        return f"Current time: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n"
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
            if not clients:
                return "Active users: None\n"
            return "Active users: " + ", ".join(clients.values()) + "\n"
    elif cmd == "EXIT":
        return "Disconnecting...\n"
    else:
        return "Unknown command\n"


def handle_client(client_socket, client_address):
    """
    Handles a single client connection.
    """
    ip_address, port = client_address
    print(f"[+] New connection from {ip_address}:{port}")

    # Get nickname
    try:
        client_socket.sendall(b"Enter your nickname: ")
        nickname_data = client_socket.recv(1024)
        if not nickname_data:
            raise Exception("Client disconnected during nickname entry.")

        nickname = nickname_data.decode('utf-8').strip()
        if not nickname:
            nickname = f"User_{port}"

        with clients_lock:
            clients[client_socket] = nickname

        print(f"[+] {nickname} joined from {ip_address}:{port}")

        welcome_message = (
            "Welcome to the server! Available commands: /TIME, /ECHO <text>, "
            "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n"
        )
        client_socket.sendall(welcome_message.encode('utf-8'))

        broadcast(f"*** {nickname} joined the chat ***\n", client_socket)

        # Main communication loop
        while True:
            data = client_socket.recv(1024)
            if not data:
                break

            message = data.decode('utf-8').strip()
            if not message:
                continue

            if message.startswith('/'):
                command = message[1:]
                response = process_command(command)
                client_socket.sendall(response.encode('utf-8'))

                if command.upper().startswith("EXIT"):
                    break
            else:
                print(f"[{nickname}] {message}")
                broadcast(f"[{nickname}] {message}\n", client_socket)

    except ConnectionResetError:
        print(f"[-] {nickname if 'nickname' in locals() else f'User_{port}'} disconnected unexpectedly.")
    except Exception as e:
        print(f"Error handling client {ip_address}:{port}: {e}")
    finally:
        # Clean up client
        with clients_lock:
            if client_socket in clients:
                nickname_leaving = clients[client_socket]
                del clients[client_socket]
                broadcast(f"*** {nickname_leaving} left the chat ***\n", client_socket)
                print(f"[-] {nickname_leaving} disconnected")
        client_socket.close()


def main():
    """
    Starts and runs the chat server.
    """
    server_socket = None
    try:
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Allows immediate reuse of address

        server_socket.bind((HOST, PORT))
        server_socket.listen(5)  # Max 5 queued connections
        print(f"Chat server running on {HOST}:{PORT}")

        while True:
            client_socket, client_address = server_socket.accept()
            # Start a new thread to handle the client
            client_handler = threading.Thread(target=handle_client, args=(client_socket, client_address))
            client_handler.daemon = True  # Allows the main program to exit even if threads are still running
            client_handler.start()

    except OSError as e:
        print(f"Server error: {e}")
        if server_socket:
            server_socket.close()
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nServer is shutting down.")
    finally:
        if server_socket:
            server_socket.close()
        print("Server closed.")


if __name__ == "__main__":
    main()