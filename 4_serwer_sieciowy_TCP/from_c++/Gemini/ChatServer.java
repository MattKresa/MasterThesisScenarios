import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;
import java.text.SimpleDateFormat;

public class ChatServer {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Using ConcurrentHashMap for thread-safe access to clients map
    private static Map<Socket, String> clients = new ConcurrentHashMap<>();
    // ReentrantLock for more explicit control, though ConcurrentHashMap handles most synchronization
    private static final Lock clientsLock = new ReentrantLock();

    public static void main(String[] args) {
        // ExecutorService to manage client threads
        ExecutorService executorService = Executors.newCachedThreadPool();
        ServerSocket serverSocket = null;

        try {
            serverSocket = new ServerSocket(PORT, 50, InetAddress.getByName(HOST));
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                Socket clientSocket = serverSocket.accept(); // Blocks until a client connects
                executorService.submit(new ClientHandler(clientSocket)); // Handle client in a new thread
            }
        } catch (IOException e) {
            System.err.println("Server error: " + e.getMessage());
        } finally {
            if (serverSocket != null) {
                try {
                    serverSocket.close();
                } catch (IOException e) {
                    System.err.println("Error closing server socket: " + e.getMessage());
                }
            }
            executorService.shutdown(); // Shut down the thread pool
            System.out.println("Server closed.");
        }
    }

    /**
     * Sends a message to all connected clients except the sender.
     * @param message The message to broadcast.
     * @param senderSocket The socket of the sender, to exclude from broadcasting. Can be null.
     */
    public static void broadcast(String message, Socket senderSocket) {
        clientsLock.lock();
        try {
            for (Map.Entry<Socket, String> entry : clients.entrySet()) {
                Socket clientSocket = entry.getKey();
                if (!clientSocket.equals(senderSocket)) {
                    try {
                        // Using a BufferedWriter for efficient writing
                        OutputStream os = clientSocket.getOutputStream();
                        PrintWriter writer = new PrintWriter(new OutputStreamWriter(os, "UTF-8"), true); // true for autoFlush
                        writer.println(message); // println adds a newline, similar to C++ behavior
                    } catch (IOException e) {
                        System.err.println("Error broadcasting to " + entry.getValue() + ": " + e.getMessage());
                        // Optional: remove client if communication fails
                    }
                }
            }
        } finally {
            clientsLock.unlock();
        }
    }

    /**
     * Processes commands sent by clients.
     * @param command The command string.
     * @return The response string for the client.
     */
    private static String processCommand(String command) {
        String[] parts = command.split("\\s+"); // Split by one or more spaces
        if (parts.length == 0) {
            return "Error: empty command\n";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
                return "Current time: " + formatter.format(new Date()) + "\n";
            case "ECHO":
                if (parts.length > 1) {
                    StringBuilder sb = new StringBuilder();
                    for (int i = 1; i < parts.length; i++) {
                        sb.append(parts[i]).append(" ");
                    }
                    return sb.toString().trim() + "\n"; // trim to remove trailing space
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
                clientsLock.lock();
                try {
                    if (clients.isEmpty()) {
                        return "Active users: None\n";
                    }
                    return "Active users: " + String.join(", ", clients.values()) + "\n";
                } finally {
                    clientsLock.unlock();
                }
            case "EXIT":
                return "Disconnecting...\n";
            default:
                return "Unknown command\n";
        }
    }

    // Inner class to handle each client connection
    private static class ClientHandler implements Runnable {
        private Socket clientSocket;
        private String nickname;
        private BufferedReader reader;
        private PrintWriter writer;

        public ClientHandler(Socket socket) {
            this.clientSocket = socket;
        }

        @Override
        public void run() {
            try {
                // Input and Output Streams for communication
                reader = new BufferedReader(new InputStreamReader(clientSocket.getInputStream(), "UTF-8"));
                writer = new PrintWriter(new OutputStreamWriter(clientSocket.getOutputStream(), "UTF-8"), true); // autoFlush

                // Get nickname
                writer.print("Enter your nickname: "); // Use print to avoid extra newline
                writer.flush(); // Ensure the prompt is sent immediately

                String rawNickname = reader.readLine();
                if (rawNickname == null) { // Client disconnected
                    throw new IOException("Client disconnected during nickname entry.");
                }
                nickname = rawNickname.trim();

                if (nickname.isEmpty()) {
                    nickname = "User_" + clientSocket.getPort();
                }

                clientsLock.lock();
                try {
                    clients.put(clientSocket, nickname);
                } finally {
                    clientsLock.unlock();
                }

                System.out.println("[+] " + nickname + " joined from " +
                        clientSocket.getInetAddress().getHostAddress() + ":" + clientSocket.getPort());

                String welcomeMessage =
                        "Welcome to the server! Available commands: /TIME, /ECHO <text>, " +
                                "/ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
                writer.println(welcomeMessage);

                broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

                // Main communication loop
                String message;
                while ((message = reader.readLine()) != null) {
                    message = message.trim();
                    if (message.isEmpty()) {
                        continue;
                    }

                    if (message.startsWith("/")) {
                        String command = message.substring(1);
                        String response = processCommand(command);
                        writer.println(response);

                        if (command.toUpperCase().startsWith("EXIT")) {
                            break;
                        }
                    } else {
                        System.out.println("[" + nickname + "] " + message);
                        broadcast("[" + nickname + "] " + message + "\n", clientSocket);
                    }
                }

            } catch (IOException e) {
                System.err.println("Client handler error for " + (nickname != null ? nickname : "unknown user") + ": " + e.getMessage());
            } finally {
                // Clean up client resources
                clientsLock.lock();
                try {
                    if (clients.containsKey(clientSocket)) {
                        String leavingNickname = clients.remove(clientSocket);
                        broadcast("*** " + leavingNickname + " left the chat ***\n", clientSocket);
                        System.out.println("[-] " + leavingNickname + " disconnected");
                    }
                } finally {
                    clientsLock.unlock();
                }

                try {
                    if (reader != null) reader.close();
                    if (writer != null) writer.close();
                    if (clientSocket != null) clientSocket.close();
                } catch (IOException e) {
                    System.err.println("Error closing client resources: " + e.getMessage());
                }
            }
        }
    }
}