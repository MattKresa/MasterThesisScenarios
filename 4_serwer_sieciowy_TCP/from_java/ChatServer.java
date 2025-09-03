import java.io.*;
import java.net.*;
import java.time.LocalDateTime;
import java.util.*;
import java.util.concurrent.*;

public class ChatServer {
    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();

    public static void main(String[] args) {
        try (ServerSocket serverSocket = new ServerSocket()) {
            serverSocket.bind(new InetSocketAddress(HOST, PORT));
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                Socket socket = serverSocket.accept();
                new Thread(new ClientHandler(socket)).start();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // Sends the message to all clients
    public static void broadcast(String message, Socket sender) {
        for (Socket client : clients.keySet()) {
            if (client != sender) {
                try {
                    OutputStream out = client.getOutputStream();
                    out.write(message.getBytes());
                    out.flush();
                } catch (IOException ignored) {}
            }
        }
    }

    // Adds client
    public static void addClient(Socket socket, String nickname) {
        clients.put(socket, nickname);
    }

    // Removes client
    public static void removeClient(Socket socket) {
        clients.remove(socket);
    }

    // List of active users
    public static String getActiveUsers() {
        return String.join(", ", clients.values());
    }


    static class ClientHandler implements Runnable {
        private final Socket socket;

        ClientHandler(Socket socket) {
            this.socket = socket;
        }

        @Override
        public void run() {
            String nickname = null;
            try (BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                 OutputStream out = socket.getOutputStream()) {

                out.write("Enter your nickname: ".getBytes());
                out.flush();

                nickname = in.readLine();
                if (nickname == null || nickname.isBlank()) {
                    nickname = "User_" + socket.getPort();
                }

                ChatServer.addClient(socket, nickname);
                System.out.println("[+] " + nickname + " joined from " + socket.getRemoteSocketAddress());

                out.write(("Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO\n").getBytes());
                out.flush();

                ChatServer.broadcast("*** " + nickname + " joined the chat ***\n", socket);

                String line;
                while ((line = in.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty()) continue;

                    if (line.startsWith("/")) {
                        String cmd = line.substring(1);
                        String response = processCommand(cmd);
                        out.write(response.getBytes());
                        out.flush();

                        if (cmd.toUpperCase().startsWith("EXIT")) break;
                    } else {
                        System.out.println("[" + nickname + "] " + line);
                        ChatServer.broadcast("[" + nickname + "] " + line + "\n", socket);
                    }
                }

            } catch (IOException ignored) {
            } finally {
                ChatServer.removeClient(socket);
                ChatServer.broadcast("*** " + nickname + " left the chat ***\n", socket);
                System.out.println("[-] " + nickname + " disconnected");
                try { socket.close(); } catch (IOException ignored) {}
            }
        }

        private String processCommand(String cmd) {
            String[] parts = cmd.split("\\s+");
            if (parts.length == 0) return "Error: empty command\n";

            return switch (parts[0].toUpperCase()) {
                case "TIME" -> "Current time: " + LocalDateTime.now() + "\n";

                case "ECHO" -> parts.length > 1
                        ? String.join(" ", Arrays.copyOfRange(parts, 1, parts.length)) + "\n"
                        : "Error: no text to echo\n";

                case "ADD" -> {
                    if (parts.length != 3) yield "Usage: /ADD <a> <b>\n";
                    try {
                        double a = Double.parseDouble(parts[1]);
                        double b = Double.parseDouble(parts[2]);
                        yield "Result: " + (a + b) + "\n";
                    } catch (NumberFormatException e) {
                        yield "Error: please provide numbers\n";
                    }
                }

                case "WHO" -> "Active users: " + ChatServer.getActiveUsers() + "\n";

                case "EXIT" -> "Disconnecting...\n";

                default -> "Unknown command\n";
            };
        }
    }
}
