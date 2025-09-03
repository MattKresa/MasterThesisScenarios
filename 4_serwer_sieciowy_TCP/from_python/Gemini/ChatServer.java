import java.io.*;
import java.net.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.*;
import java.util.*;
import java.util.regex.Pattern; // Added for more robust stripping

public class ChatServer {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Using ConcurrentHashMap for thread-safe access to clients
    // Key: Socket, Value: Nickname
    private static Map<Socket, String> clients = new ConcurrentHashMap<>();

    public static void main(String[] args) {
        startServer();
    }

    private static void broadcast(String message, Socket senderSocket) {
        for (Map.Entry<Socket, String> entry : clients.entrySet()) {
            Socket clientSocket = entry.getKey();
            if (clientSocket != senderSocket) {
                try {
                    // Get the output stream for the client and send the message
                    PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);
                    out.println(message);
                } catch (IOException e) {
                    System.err.println("Error broadcasting to client " + entry.getValue() + ": " + e.getMessage());
                    // Consider removing the client if sending fails repeatedly
                }
            }
        }
    }

    private static String processCommand(String command, Socket clientSocket) {
        String[] parts = command.trim().split("\\s+"); // Split by one or more whitespace characters

        if (parts.length == 0 || parts[0].isEmpty()) {
            return "Error: empty command\n";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                return "Current time: " + LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss")) + "\n";
            case "ECHO":
                if (parts.length < 2) {
                    return "Error: no text to echo\n";
                }
                StringBuilder echoText = new StringBuilder();
                for (int i = 1; i < parts.length; i++) {
                    echoText.append(parts[i]);
                    if (i < parts.length - 1) {
                        echoText.append(" ");
                    }
                }
                return echoText.toString() + "\n";
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
                StringJoiner activeUsers = new StringJoiner(", ");
                clients.values().forEach(activeUsers::add); // Add all nicknames
                return "Active users: " + activeUsers.toString() + "\n";
            case "EXIT":
                return "Disconnecting...\n";
            default:
                return "Unknown command\n";
        }
    }

    private static void handleClient(Socket clientSocket) {
        String nickname = "Unknown"; // Default nickname
        String clientAddress = clientSocket.getInetAddress().getHostAddress() + ":" + clientSocket.getPort();

        try (
                BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true) // autoFlush true
        ) {
            out.println("Enter your nickname: ");
            nickname = in.readLine(); // Read the first line as nickname

            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + clientSocket.getPort();
            } else {
                nickname = nickname.trim(); // Trim any whitespace from the nickname
            }

            clients.put(clientSocket, nickname);

            System.out.println("[+] " + nickname + " joined from " + clientAddress);
            out.println("Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users");
            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            String clientInput;
            while ((clientInput = in.readLine()) != null) {
                clientInput = clientInput.trim(); // Trim leading/trailing whitespace

                if (clientInput.isEmpty()) {
                    continue; // Ignore empty lines
                }

                if (clientInput.startsWith("/")) { // Commands start with /
                    String cmd = clientInput.substring(1); // Remove the '/'
                    String response = processCommand(cmd, clientSocket);
                    out.println(response);

                    if (cmd.toUpperCase().startsWith("EXIT")) {
                        break; // Exit loop if client wants to disconnect
                    }
                } else {
                    System.out.println("[" + nickname + "] " + clientInput);
                    broadcast("[" + nickname + "] " + clientInput + "\n", clientSocket);
                }
            }
        } catch (IOException e) {
            System.err.println("Client " + nickname + " (" + clientAddress + ") disconnected unexpectedly: " + e.getMessage());
        } finally {
            clients.remove(clientSocket); // Remove client from the map
            broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
            System.out.println("[-] " + nickname + " disconnected (" + clientAddress + ")");
            try {
                clientSocket.close(); // Ensure the socket is closed
            } catch (IOException e) {
                System.err.println("Error closing client socket for " + nickname + ": " + e.getMessage());
            }
        }
    }

    private static void startServer() {
        try (ServerSocket serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST))) {
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            // Using an ExecutorService for managing threads (better than raw Threads)
            ExecutorService executor = Executors.newCachedThreadPool();

            while (true) {
                Socket clientSocket = serverSocket.accept(); // Blocks until a client connects
                // Submit client handling to a thread pool
                executor.submit(() -> handleClient(clientSocket));
            }
        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
            e.printStackTrace();
        }
    }
}