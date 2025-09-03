import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;

public class ChatServer {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();
    private static final ExecutorService threadPool = Executors.newCachedThreadPool();

    public static void main(String[] args) {
        try (ServerSocket serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST))) {
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                threadPool.execute(() -> handleClient(clientSocket));
            }
        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
        } finally {
            threadPool.shutdown();
        }
    }

    private static void broadcast(String message, Socket sender) {
        clients.forEach((socket, nickname) -> {
            if (socket != sender) {
                try {
                    socket.getOutputStream().write(message.getBytes());
                } catch (IOException e) {
                    // Client disconnected
                }
            }
        });
    }

    private static String processCommand(String command) {
        String[] parts = command.trim().split("\\s+");
        if (parts.length == 0) return "Error: empty command\n";

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                return "Current time: " + new Date() + "\n";
            case "ECHO":
                if (parts.length > 1) {
                    return String.join(" ", Arrays.copyOfRange(parts, 1, parts.length)) + "\n";
                }
                return "Error: no text to echo\n";
            case "ADD":
                if (parts.length != 3) return "Usage: /ADD <a> <b>\n";
                try {
                    double a = Double.parseDouble(parts[1]);
                    double b = Double.parseDouble(parts[2]);
                    return "Result: " + (a + b) + "\n";
                } catch (NumberFormatException e) {
                    return "Error: please provide numbers\n";
                }
            case "WHO":
                return "Active users: " + String.join(", ", clients.values()) + "\n";
            case "EXIT":
                return "Disconnecting...\n";
            default:
                return "Unknown command\n";
        }
    }

    private static void handleClient(Socket clientSocket) {
        String nickname = "";
        try {
            // Get client address info
            InetSocketAddress clientAddr = (InetSocketAddress) clientSocket.getRemoteSocketAddress();

            // Request nickname
            clientSocket.getOutputStream().write("Enter your nickname: ".getBytes());

            BufferedReader reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            nickname = reader.readLine();

            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + clientAddr.getPort();
            } else {
                nickname = nickname.trim();
            }

            // Add client to map
            clients.put(clientSocket, nickname);

            System.out.printf("[+] %s joined from %s:%d\n",
                    nickname, clientAddr.getAddress().getHostAddress(), clientAddr.getPort());

            // Send welcome message
            String welcome = "Welcome to the server! Available commands: /TIME, /ECHO <text>, " +
                    "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
            clientSocket.getOutputStream().write(welcome.getBytes());

            // Broadcast join message
            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            // Main message loop
            String msg;
            while ((msg = reader.readLine()) != null) {
                if (msg.startsWith("/")) {
                    String cmd = msg.substring(1);
                    String response = processCommand(cmd);
                    clientSocket.getOutputStream().write(response.getBytes());

                    if (cmd.toUpperCase().startsWith("EXIT")) {
                        break;
                    }
                } else {
                    System.out.printf("[%s] %s\n", nickname, msg);
                    broadcast("[" + nickname + "] " + msg + "\n", clientSocket);
                }
            }
        } catch (IOException e) {
            System.err.println("Client error: " + e.getMessage());
        } finally {
            // Remove client and notify others
            if (nickname != null && !nickname.isEmpty()) {
                clients.remove(clientSocket);
                broadcast("*** " + nickname + " left the chat ***\n", clientSocket);
                System.out.println("[-] " + nickname + " disconnected");
            }

            try {
                clientSocket.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }
}