// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserApi.java) ---
package org.example;

// Main application class for the REST API
// To run this code, you'll need the following dependencies in your project:
// - Spark Java: https://sparkjava.com/
// - Google Gson: https://github.com/google/gson
//
// If you are using Maven, add these to your pom.xml:
// <dependency>
//     <groupId>com.sparkjava</groupId>
//     <artifactId>spark-core</artifactId>
//     <version>2.9.3</version>
// </dependency>
// <dependency>
//     <groupId>com.google.code.gson</groupId>
//     <artifactId>gson</artifactId>
//     <version>2.8.6</version>
// </dependency>

import com.google.gson.Gson;
import com.google.gson.JsonSyntaxException;
import spark.Spark;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

public class UserApi {

    // The name of the JSON file for user data.
    private static final String DATA_FILE = "users.json";

    // A Gson instance for handling JSON serialization and deserialization.
    private static final Gson gson = new Gson();

    /**
     * Represents a User object with an ID, name, and email.
     * This is the Java equivalent of the JSON objects in the C++ code.
     */
    static class User {
        int id;
        String name;
        String email;

        // Constructor for creating a new user.
        public User(int id, String name, String email) {
            this.id = id;
            this.name = name;
            this.email = email;
        }
    }

    /**
     * Loads the list of users from the DATA_FILE.
     * If the file does not exist or is invalid, an empty list is returned.
     * This mirrors the C++ code's behavior.
     *
     * @return a List of User objects.
     */
    private static List<User> loadUsers() {
        if (!Files.exists(Paths.get(DATA_FILE))) {
            return new ArrayList<>();
        }
        try (FileReader reader = new FileReader(DATA_FILE)) {
            // Deserialize the JSON array from the file into a List of User objects.
            return gson.fromJson(reader, new com.google.gson.reflect.TypeToken<List<User>>(){}.getType());
        } catch (IOException | JsonSyntaxException e) {
            System.err.println("Error loading users from file: " + e.getMessage());
            return new ArrayList<>();
        }
    }

    /**
     * Saves a list of users to the DATA_FILE.
     *
     * @param users The List of User objects to save.
     */
    private static void saveUsers(List<User> users) {
        try (FileWriter writer = new FileWriter(DATA_FILE)) {
            // Serialize the List of User objects to a formatted JSON string and write to file.
            gson.toJson(users, writer);
        } catch (IOException e) {
            System.err.println("Error saving users to file: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        // Set the server port to 5000, consistent with the C++ example.
        Spark.port(5000);

        // Configure the thread pool for multi-threaded operation.
        // This is the Java equivalent of Crow's multithreaded().
        Spark.threadPool(10);

        // GET /users - Returns a list of all users.
        Spark.get("/users", (req, res) -> {
            res.type("application/json");
            List<User> users = loadUsers();
            return gson.toJson(users);
        });

        // GET /users/:id - Returns a single user by ID.
        Spark.get("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                int userId = Integer.parseInt(req.params(":id"));
                List<User> users = loadUsers();
                // Find the user using a Java Stream.
                User foundUser = users.stream()
                        .filter(u -> u.id == userId)
                        .findFirst()
                        .orElse(null);

                if (foundUser != null) {
                    return gson.toJson(foundUser);
                } else {
                    res.status(404); // Not Found
                    return "{\"error\":\"User not found\"}";
                }
            } catch (NumberFormatException e) {
                res.status(400); // Bad Request
                return "{\"error\":\"Invalid user ID\"}";
            }
        });

        // POST /users - Adds a new user.
        Spark.post("/users", (req, res) -> {
            res.type("application/json");
            try {
                // Parse the request body JSON into a temporary User object.
                User newUser = gson.fromJson(req.body(), User.class);
                if (newUser.name == null || newUser.email == null) {
                    res.status(400); // Bad Request
                    return "{\"error\":\"Missing name or email\"}";
                }

                List<User> users = loadUsers();

                // Generate a new ID based on the highest existing ID.
                int newId = users.stream().mapToInt(u -> u.id).max().orElse(0) + 1;
                newUser.id = newId;

                users.add(newUser);
                saveUsers(users);

                res.status(201); // Created
                return gson.toJson(newUser);
            } catch (JsonSyntaxException e) {
                res.status(400); // Bad Request
                return "{\"error\":\"Invalid JSON\"}";
            }
        });

        // PUT /users/:id - Updates an existing user.
        Spark.put("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                int userId = Integer.parseInt(req.params(":id"));
                // We use a temporary object to hold the fields to update.
                User updatedFields = gson.fromJson(req.body(), User.class);

                List<User> users = loadUsers();
                User foundUser = users.stream()
                        .filter(u -> u.id == userId)
                        .findFirst()
                        .orElse(null);

                if (foundUser != null) {
                    // Update fields only if they are present in the request body.
                    if (updatedFields.name != null) {
                        foundUser.name = updatedFields.name;
                    }
                    if (updatedFields.email != null) {
                        foundUser.email = updatedFields.email;
                    }
                    saveUsers(users);
                    return gson.toJson(foundUser);
                } else {
                    res.status(404); // Not Found
                    return "{\"error\":\"User not found\"}";
                }
            } catch (NumberFormatException | JsonSyntaxException e) {
                res.status(400); // Bad Request
                return "{\"error\":\"Invalid request\"}";
            }
        });

        // DELETE /users/:id - Deletes a user.
        Spark.delete("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                int userId = Integer.parseInt(req.params(":id"));
                List<User> users = loadUsers();

                // Remove the user from the list.
                boolean removed = users.removeIf(u -> u.id == userId);

                if (removed) {
                    saveUsers(users);
                    return "{\"message\":\"User deleted\"}";
                } else {
                    res.status(404); // Not Found
                    return "{\"error\":\"User not found\"}";
                }
            } catch (NumberFormatException e) {
                res.status(400); // Bad Request
                return "{\"error\":\"Invalid user ID\"}";
            }
        });
    }
}
