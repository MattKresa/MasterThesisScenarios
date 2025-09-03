// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserService.java) ---
package org.example;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import spark.Spark;

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Iterator;

public class UserService {
    private static final String DATA_FILE = "users.json";
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    // Load users from file
    private static JsonArray loadUsers() throws IOException {
        if (!Files.exists(Paths.get(DATA_FILE))) {
            return new JsonArray();
        }
        try (FileReader reader = new FileReader(DATA_FILE)) {
            return JsonParser.parseReader(reader).getAsJsonArray();
        }
    }

    // Save users to file
    private static void saveUsers(JsonArray users) throws IOException {
        try (FileWriter writer = new FileWriter(DATA_FILE)) {
            gson.toJson(users, writer);
        }
    }

    public static void main(String[] args) {
        Spark.port(5000);
        // GET /users - list all users
        Spark.get("/users", (req, res) -> {
            res.type("application/json");
            try {
                return loadUsers();
            } catch (IOException e) {
                res.status(500);
                return errorResponse("Internal server error");
            }
        }, gson::toJson);

        // GET /users/:id - get single user
        Spark.get("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                JsonArray users = loadUsers();
                int userId = Integer.parseInt(req.params(":id"));

                for (int i = 0; i < users.size(); i++) {
                    JsonObject user = users.get(i).getAsJsonObject();
                    if (user.get("id").getAsInt() == userId) {
                        return user;
                    }
                }

                res.status(404);
                return errorResponse("User not found");
            } catch (IOException e) {
                res.status(500);
                return errorResponse("Internal server error");
            }
        }, gson::toJson);

        // POST /users - add new user
        Spark.post("/users", (req, res) -> {
            res.type("application/json");
            try {
                JsonObject newUser = JsonParser.parseString(req.body()).getAsJsonObject();

                if (!newUser.has("name") || !newUser.has("email")) {
                    res.status(400);
                    return errorResponse("Missing name or email");
                }

                JsonArray users = loadUsers();
                int newId = 0;
                for (int i = 0; i < users.size(); i++) {
                    JsonObject user = users.get(i).getAsJsonObject();
                    newId = Math.max(newId, user.get("id").getAsInt());
                }
                newId++;

                JsonObject user = new JsonObject();
                user.addProperty("id", newId);
                user.addProperty("name", newUser.get("name").getAsString());
                user.addProperty("email", newUser.get("email").getAsString());

                users.add(user);
                saveUsers(users);

                res.status(201);
                return user;
            } catch (Exception e) {
                res.status(400);
                return errorResponse("Invalid JSON");
            }
        }, gson::toJson);

        // PUT /users/:id - update user
        Spark.put("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                int userId = Integer.parseInt(req.params(":id"));
                JsonObject updateData = JsonParser.parseString(req.body()).getAsJsonObject();

                JsonArray users = loadUsers();
                for (int i = 0; i < users.size(); i++) {
                    JsonObject user = users.get(i).getAsJsonObject();
                    if (user.get("id").getAsInt() == userId) {
                        if (updateData.has("name")) {
                            user.addProperty("name", updateData.get("name").getAsString());
                        }
                        if (updateData.has("email")) {
                            user.addProperty("email", updateData.get("email").getAsString());
                        }
                        saveUsers(users);
                        return user;
                    }
                }

                res.status(404);
                return errorResponse("User not found");
            } catch (Exception e) {
                res.status(400);
                return errorResponse("Invalid JSON");
            }
        }, gson::toJson);

        // DELETE /users/:id - delete user
        Spark.delete("/users/:id", (req, res) -> {
            res.type("application/json");
            try {
                int userId = Integer.parseInt(req.params(":id"));
                JsonArray users = loadUsers();
                int initialSize = users.size();

                Iterator<com.google.gson.JsonElement> iterator = users.iterator();
                while (iterator.hasNext()) {
                    JsonObject user = iterator.next().getAsJsonObject();
                    if (user.get("id").getAsInt() == userId) {
                        iterator.remove();
                        break;
                    }
                }

                if (users.size() == initialSize) {
                    res.status(404);
                    return errorResponse("User not found");
                }

                saveUsers(users);
                return successResponse("User deleted");
            } catch (IOException e) {
                res.status(500);
                return errorResponse("Internal server error");
            }
        }, gson::toJson);

        Spark.init();
    }

    private static JsonObject errorResponse(String message) {
        JsonObject response = new JsonObject();
        response.addProperty("error", message);
        return response;
    }

    private static JsonObject successResponse(String message) {
        JsonObject response = new JsonObject();
        response.addProperty("message", message);
        return response;
    }
}