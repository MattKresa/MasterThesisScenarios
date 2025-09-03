import java.io.*;
import java.net.*;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.locks.ReentrantLock;

public class ChatServer {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();
    private static final ReentrantLock lock = new ReentrantLock();

    public static void main(String[] args) {
        startServer();
    }

    private static void startServer() {
        try (ServerSocket serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST))) {
            System.out.printf("Chat server running on %s:%d%n", HOST, PORT);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                new Thread(() -> handleClient(clientSocket)).start();
            }
        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
        }
    }

    private static void handleClient(Socket clientSocket) {
        try (BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
             PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)) {

            // Get nickname
            out.println("Enter your nickname: ");
            String nickname = in.readLine();
            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + clientSocket.getPort();
            } else {
                nickname = nickname.trim();
            }

            // Add client to map
            lock.lock();
            try {
                clients.put(clientSocket, nickname);
            } finally {
                lock.unlock();
            }

            System.out.printf("[+] %s joined from %s%n", nickname, clientSocket.getRemoteSocketAddress());
            out.println("Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users");
            broadcast("*** " + nickname + " joined the chat ***", clientSocket);

            String inputLine;
            while ((inputLine = in.readLine()) != null) {
                inputLine = inputLine.trim();

                if (inputLine.startsWith("/")) {  // Command processing
                    String cmd = inputLine.substring(1);
                    String response = processCommand(cmd, clientSocket);
                    out.println(response);

                    if (cmd.toUpperCase().startsWith("EXIT")) {
                        break;
                    }
                } else {  // Regular message
                    System.out.printf("[%s] %s%n", nickname, inputLine);
                    broadcast("[" + nickname + "] " + inputLine, clientSocket);
                }
            }
        } catch (IOException e) {
            System.err.println("Client handling error: " + e.getMessage());
        } finally {
            // Remove client and notify others
            String nickname = clients.get(clientSocket);
            lock.lock();
            try {
                clients.remove(clientSocket);
            } finally {
                lock.unlock();
            }

            if (nickname != null) {
                broadcast("*** " + nickname + " left the chat ***", clientSocket);
                System.out.printf("[-] %s disconnected%n", nickname);
            }

            try {
                clientSocket.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }

    private static void broadcast(String message, Socket senderSocket) {
        lock.lock();
        try {
            for (Socket client : clients.keySet()) {
                if (client != senderSocket) {
                    try {
                        PrintWriter out = new PrintWriter(client.getOutputStream(), true);
                        out.println(message);
                    } catch (IOException e) {
                        // Ignore disconnected clients
                    }
                }
            }
        } finally {
            lock.unlock();
        }
    }

    private static String processCommand(String command, Socket clientSocket) {
        String[] parts = command.split("\\s+");

        if (parts.length == 0 || parts[0].isEmpty()) {
            return "Error: empty command";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                return "Current time: " + LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));

            case "ECHO":
                if (parts.length > 1) {
                    return String.join(" ", Arrays.copyOfRange(parts, 1, parts.length));
                } else {
                    return "Error: no text to echo";
                }

            case "ADD":
                if (parts.length != 3) {
                    return "Usage: /ADD <a> <b>";
                }
                try {
                    double a = Double.parseDouble(parts[1]);
                    double b = Double.parseDouble(parts[2]);
                    return "Result: " + (a + b);
                } catch (NumberFormatException e) {
                    return "Error: please provide numbers";
                }

            case "WHO":
                lock.lock();
                try {
                    return "Active users: " + String.join(", ", clients.values());
                } finally {
                    lock.unlock();
                }

            case "EXIT":
                return "Disconnecting...";

            default:
                return "Unknown command";
        }
    }
}