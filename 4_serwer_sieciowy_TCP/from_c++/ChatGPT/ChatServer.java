import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;

public class ChatServer {

    private static final String HOST = "127.0.0.1";
    private static final int PORT = 5000;

    // Thread-safe map for clients
    private static final Map<Socket, String> clients = new ConcurrentHashMap<>();

    public static void main(String[] args) {
        try (ServerSocket serverSocket = new ServerSocket()) {
            serverSocket.bind(new InetSocketAddress(HOST, PORT));
            System.out.println("Chat server running on " + HOST + ":" + PORT);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                new Thread(() -> handleClient(clientSocket)).start();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void broadcast(String message, Socket sender) {
        for (Socket client : clients.keySet()) {
            if (client != sender) {
                try {
                    client.getOutputStream().write(message.getBytes());
                } catch (IOException ignored) {}
            }
        }
    }

    private static String processCommand(String command) {
        String[] parts = command.trim().split("\\s+");
        if (parts.length == 0) return "Error: empty command\n";

        String cmd = parts[0].toUpperCase();

        switch (cmd) {
            case "TIME":
                return "Current time: " + new SimpleDateFormat("EEE MMM dd HH:mm:ss yyyy").format(new Date()) + "\n";

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
        try {
            BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
            OutputStream out = clientSocket.getOutputStream();

            out.write("Enter your nickname: ".getBytes());
            String nickname = in.readLine();
            if (nickname == null || nickname.trim().isEmpty()) {
                nickname = "User_" + clientSocket.getPort();
            }

            clients.put(clientSocket, nickname);
            System.out.println("[+] " + nickname + " joined from " + clientSocket.getInetAddress().getHostAddress() + ":" + clientSocket.getPort());

            String welcome = "Welcome to the server! Available commands: /TIME, /ECHO <text>, /ADD <a> <b>, /EXIT, /WHO. You can also send messages to other users\n";
            out.write(welcome.getBytes());

            broadcast("*** " + nickname + " joined the chat ***\n", clientSocket);

            String msg;
            while ((msg = in.readLine()) != null) {
                msg = msg.trim();
                if (msg.isEmpty()) continue;

                if (msg.startsWith("/")) {
                    String cmd = msg.substring(1);
                    String response = processCommand(cmd);
                    out.write(response.getBytes());

                    if (cmd.equalsIgnoreCase("EXIT")) {
                        break;
                    }
                } else {
                    System.out.println("[" + nickname + "] " + msg);
                    broadcast("[" + nickname + "] " + msg + "\n", clientSocket);
                }
            }
        } catch (IOException ignored) {
        } finally {
            String leftNick = clients.remove(clientSocket);
            if (leftNick != null) {
                broadcast("*** " + leftNick + " left the chat ***\n", clientSocket);
                System.out.println("[-] " + leftNick + " disconnected");
            }
            try {
                clientSocket.close();
            } catch (IOException ignored) {}
        }
    }
}
