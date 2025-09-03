import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

public class ChatServer {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Thread-safe map to store client connections and nicknames
    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();
    private static final Object lock = new Object();

    /**
     * Broadcast a message to all connected clients except the sender
     */
    public static void broadcast(String message, Socket senderSocket) {
        synchronized (lock) {
            for (Socket clientSocket : clients.keySet()) {
                if (clientSocket != senderSocket) {
                    try {
                        PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);
                        out.print(message);
                        out.flush();
                    } catch (IOException e) {
                        // Client disconnected, ignore
                    }
                }
            }
        }
    }

    /**
     * Process commands that start with /
     */
    public static String processCommand(String command, Socket conn) {
        String[] parts = command.trim().split("\\s+");

        if (parts.length == 0 || parts[0].isEmpty()) {
            return "Error: empty command\n";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                LocalDateTime now = LocalDateTime.now();
                DateTimeFormatter formatter = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss.SSS");
                return "Current time: " + now.format(formatter) + "\n";

            case "ECHO":
                if (parts.length > 1) {
                    StringBuilder echo = new StringBuilder();
                    for (int i = 1; i < parts.length; i++) {
                        if (i > 1) echo.append(" ");
                        echo.append(parts[i]);
                    }
                    return echo.toString() + "\n";
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
                synchronized (lock) {
                    if (clients.isEmpty()) {
                        return "Active users: (none)\n";
                    }
                    StringBuilder users = new StringBuilder("Active users: ");
                    boolean first = true;
                    for (String nickname : clients.values()) {
                        if (!first) users.append(", ");
                        users.append(nickname);
                        first = false;
                    }
                    return users.toString() + "\n";
                }

            case "EXIT":
                return "Disconnecting...\n";

            default:
                return "Unknown command\n";
        }
    }

    /**
     * Handle individual client connection
     */
    public static void handleClient(Socket clientSocket, String clientAddr) {
        String nickname = null;
        BufferedReader in = null;
        PrintWriter out = null;

        try {
            in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            out = new PrintWriter(clientSocket.getOutputStream(), true);

            // Get nickname from client
            out.print("Enter your nickname: ");
            out.flush();

            nickname = in.readLine();
            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + clientSocket.getPort();
            } else {
                nickname = nickname.trim();
            }

            // Add client to the map
            synchronized (lock) {
                clients.put(clientSocket, nickname);
            }

            System.out.println("[+] " + nickname + " joined from " + clientAddr);

            // Send welcome message
            out.print("Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n");
            out.flush();

            // Broadcast join message
            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            String inputLine;
            while ((inputLine = in.readLine()) != null) {
                inputLine = inputLine.trim();
                if (inputLine.isEmpty()) {
                    continue;
                }

                if (inputLine.startsWith("/")) {
                    // Process command
                    String cmd = inputLine.substring(1);
                    String response = processCommand(cmd, clientSocket);
                    out.print(response);
                    out.flush();

                    if (cmd.toUpperCase().startsWith("EXIT")) {
                        break;
                    }
                } else {
                    // Regular message - broadcast to other clients
                    System.out.println("[" + nickname + "] " + inputLine);
                    broadcast("[" + nickname + "] " + inputLine + "\n", clientSocket);
                }
            }

        } catch (IOException e) {
            System.err.println("Error handling client " + (nickname != null ? nickname : "unknown") + ": " + e.getMessage());
        } finally {
            // Cleanup
            if (nickname != null) {
                synchronized (lock) {
                    clients.remove(clientSocket);
                }
                broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
                System.out.println("[-] " + nickname + " disconnected");
            }

            try {
                if (in != null) in.close();
                if (out != null) out.close();
                if (clientSocket != null && !clientSocket.isClosed()) {
                    clientSocket.close();
                }
            } catch (IOException e) {
                System.err.println("Error closing client connection: " + e.getMessage());
            }
        }
    }

    /**
     * Start the chat server
     */
    public static void startServer() {
        try (ServerSocket serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST))) {
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                try {
                    Socket clientSocket = serverSocket.accept();
                    String clientAddr = clientSocket.getInetAddress().getHostAddress() + ":" + clientSocket.getPort();

                    // Create a new thread to handle each client
                    Thread clientThread = new Thread(() -> handleClient(clientSocket, clientAddr));
                    clientThread.setDaemon(true);
                    clientThread.start();

                } catch (IOException e) {
                    System.err.println("Error accepting client connection: " + e.getMessage());
                }
            }

        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
        }
    }

    /**
     * Main method to start the server
     */
    public static void main(String[] args) {
        System.out.println("Starting Java Chat Server...");
        startServer();
    }
}