import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.text.SimpleDateFormat;

public class ChatServer {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Thread-safe map to store client sockets and their nicknames
    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();

    /**
     * Broadcast a message to all connected clients except the sender
     */
    public static void broadcast(String message, Socket sender) {
        List<Socket> disconnectedClients = new ArrayList<>();

        for (Socket clientSocket : clients.keySet()) {
            if (clientSocket != sender) {
                try {
                    PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);
                    out.print(message);
                    out.flush();
                } catch (IOException e) {
                    // Client disconnected, mark for removal
                    disconnectedClients.add(clientSocket);
                }
            }
        }

        // Remove disconnected clients
        for (Socket disconnected : disconnectedClients) {
            clients.remove(disconnected);
            try {
                disconnected.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }

    /**
     * Process server commands
     */
    public static String processCommand(String command) {
        String[] parts = command.trim().split("\\s+");

        if (parts.length == 0) {
            return "Error: empty command\n";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                SimpleDateFormat sdf = new SimpleDateFormat("EEE MMM dd HH:mm:ss yyyy");
                return "Current time: " + sdf.format(new Date()) + "\n";

            case "ECHO":
                if (parts.length > 1) {
                    StringBuilder echo = new StringBuilder();
                    for (int i = 1; i < parts.length; i++) {
                        echo.append(parts[i]);
                        if (i < parts.length - 1) echo.append(" ");
                    }
                    echo.append("\n");
                    return echo.toString();
                }
                return "Error: no text to echo\n";

            case "ADD":
                if (parts.length != 3) {
                    return "Usage: /ADD <a> <b>\n";
                }
                try {
                    double a = Double.parseDouble(parts[1]);
                    double b = Double.parseDouble(parts[2]);
                    return "Result: " + (a + b) + "\n";
                } catch (NumberFormatException e) {
                    return "Error: please provide numbers\n";
                }

            case "WHO":
                if (clients.isEmpty()) {
                    return "Active users: none\n";
                }
                StringBuilder users = new StringBuilder("Active users: ");
                boolean first = true;
                for (String nickname : clients.values()) {
                    if (!first) users.append(", ");
                    users.append(nickname);
                    first = false;
                }
                users.append("\n");
                return users.toString();

            case "EXIT":
                return "Disconnecting...\n";

            default:
                return "Unknown command\n";
        }
    }

    /**
     * Handle individual client connection
     */
    static class ClientHandler implements Runnable {
        private Socket clientSocket;
        private String clientAddress;
        private String nickname;
        private BufferedReader in;
        private PrintWriter out;

        public ClientHandler(Socket socket) {
            this.clientSocket = socket;
            this.clientAddress = socket.getInetAddress().getHostAddress() + ":" + socket.getPort();
        }

        @Override
        public void run() {
            try {
                // Setup input/output streams
                in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                out = new PrintWriter(clientSocket.getOutputStream(), true);

                // Get nickname
                out.print("Enter your nickname: ");
                out.flush();

                String nicknameInput = in.readLine();
                if (nicknameInput == null || nicknameInput.trim().isEmpty()) {
                    nickname = "User_" + clientSocket.getPort();
                } else {
                    nickname = nicknameInput.trim();
                }

                // Add client to clients map
                clients.put(clientSocket, nickname);

                System.out.println("[+] " + nickname + " joined from " + clientAddress);

                // Send welcome message
                String welcome = "Welcome to the server! Available commands: /TIME, /ECHO <text>, " +
                        "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
                out.print(welcome);
                out.flush();

                // Broadcast join message
                broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

                // Main message loop
                String message;
                while ((message = in.readLine()) != null) {
                    message = message.trim();

                    if (message.isEmpty()) {
                        continue;
                    }

                    if (message.startsWith("/")) {
                        // Process command
                        String cmd = message.substring(1);
                        String response = processCommand(cmd);
                        out.print(response);
                        out.flush();

                        // Check for exit command
                        if (cmd.toUpperCase().startsWith("EXIT")) {
                            break;
                        }
                    } else {
                        // Regular chat message
                        System.out.println("[" + nickname + "] " + message);
                        broadcast("[" + nickname + "] " + message + "\n", clientSocket);
                    }
                }

            } catch (IOException e) {
                System.out.println("Error handling client " + nickname + ": " + e.getMessage());
            } finally {
                // Clean up
                cleanup();
            }
        }

        private void cleanup() {
            try {
                // Remove client from map
                clients.remove(clientSocket);

                if (nickname != null) {
                    // Broadcast leave message
                    broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
                    System.out.println("[-] " + nickname + " disconnected");
                }

                // Close streams and socket
                if (in != null) in.close();
                if (out != null) out.close();
                if (clientSocket != null && !clientSocket.isClosed()) {
                    clientSocket.close();
                }
            } catch (IOException e) {
                System.out.println("Error during cleanup: " + e.getMessage());
            }
        }
    }

    public static void main(String[] args) {
        ServerSocket serverSocket = null;

        try {
            // Create server socket
            serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST));
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            // Accept client connections
            while (true) {
                try {
                    Socket clientSocket = serverSocket.accept();

                    // Create new thread for each client
                    Thread clientThread = new Thread(new ClientHandler(clientSocket));
                    clientThread.setDaemon(true);
                    clientThread.start();

                } catch (IOException e) {
                    System.out.println("Error accepting connection: " + e.getMessage());
                }
            }

        } catch (IOException e) {
            System.out.println("Server error: " + e.getMessage());
        } finally {
            // Clean up
            if (serverSocket != null && !serverSocket.isClosed()) {
                try {
                    serverSocket.close();
                } catch (IOException e) {
                    System.out.println("Error closing server socket: " + e.getMessage());
                }
            }
        }
    }
}