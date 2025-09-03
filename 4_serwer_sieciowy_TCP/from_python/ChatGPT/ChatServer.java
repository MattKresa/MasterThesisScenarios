import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.*;

public class ChatServer {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Map of connected clients: Socket -> nickname
    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();
    private static final Object lock = new Object();

    public static void main(String[] args) {
        startServer();
    }

    private static void broadcast(String message, Socket sender) {
        synchronized (lock) {
            for (Socket client : clients.keySet()) {
                if (client != sender) {
                    try {
                        OutputStream out = client.getOutputStream();
                        out.write(message.getBytes());
                        out.flush();
                    } catch (IOException e) {
                        // Ignore send errors
                    }
                }
            }
        }
    }

    private static String processCommand(String command, Socket conn) {
        String[] parts = command.trim().split("\\s+");

        if (parts.length == 0 || parts[0].isEmpty()) {
            return "Error: empty command\n";
        }

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                String timeStr = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").format(new Date());
                return "Current time: " + timeStr + "\n";

            case "ECHO":
                if (parts.length > 1) {
                    return String.join(" ", Arrays.copyOfRange(parts, 1, parts.length)) + "\n";
                } else {
                    return "Error: no text to echo\n";
                }

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
                    return "Active users: " + String.join(", ", clients.values()) + "\n";
                }

            case "EXIT":
                return "Disconnecting...\n";

            default:
                return "Unknown command\n";
        }
    }

    private static void handleClient(Socket conn) {
        try (BufferedReader in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
             OutputStream out = conn.getOutputStream()) {

            out.write("Enter your nickname: ".getBytes());
            out.flush();

            String nickname = in.readLine();
            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + conn.getPort();
            }

            synchronized (lock) {
                clients.put(conn, nickname);
            }

            System.out.println("[+] " + nickname + " joined from " + conn.getRemoteSocketAddress());
            String welcomeMsg = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to the other users\n";
            out.write(welcomeMsg.getBytes());
            out.flush();

            broadcast("*** " + nickname + " joined the chat ***\n", conn);

            String line;
            while ((line = in.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty()) {
                    break;
                }

                if (line.startsWith("/")) {
                    String cmd = line.substring(1);
                    String response = processCommand(cmd, conn);
                    out.write(response.getBytes());
                    out.flush();

                    if (cmd.toUpperCase().startsWith("EXIT")) {
                        break;
                    }
                } else {
                    System.out.println("[" + nickname + "] " + line);
                    broadcast("[" + nickname + "] " + line + "\n", conn);
                }
            }

        } catch (IOException e) {
            // Client disconnected
        } finally {
            synchronized (lock) {
                String nickname = clients.remove(conn);
                if (nickname != null) {
                    broadcast("*** " + nickname + " left the chat ***\n", conn);
                    System.out.println("[-] " + nickname + " disconnected");
                }
            }
            try {
                conn.close();
            } catch (IOException ignored) {}
        }
    }

    private static void startServer() {
        try (ServerSocket serverSocket = new ServerSocket()) {
            serverSocket.bind(new InetSocketAddress(HOST, PORT));
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                Socket conn = serverSocket.accept();
                Thread t = new Thread(() -> handleClient(conn));
                t.setDaemon(true);
                t.start();
            }

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
