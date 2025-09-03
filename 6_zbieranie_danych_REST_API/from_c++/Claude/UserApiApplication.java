// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserApiApplication.java) ---
package org.example;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.io.File;
import java.io.IOException;
import java.util.*;

@SpringBootApplication
@RestController
public class UserApiApplication {

    private static final String DATA_FILE = "users.json";
    private final ObjectMapper objectMapper = new ObjectMapper();

    public static void main(String[] args) {
        SpringApplication.run(UserApiApplication.class, args);
    }

    // Load users from JSON file
    private List<User> loadUsers() {
        File file = new File(DATA_FILE);
        if (!file.exists()) {
            return new ArrayList<>();
        }

        try {
            TypeReference<List<User>> typeRef = new TypeReference<List<User>>() {};
            List<User> users = objectMapper.readValue(file, typeRef);
            return users != null ? users : new ArrayList<>();
        } catch (IOException e) {
            return new ArrayList<>();
        }
    }

    // Save users to JSON file
    private void saveUsers(List<User> users) {
        try {
            objectMapper.writerWithDefaultPrettyPrinter()
                    .writeValue(new File(DATA_FILE), users);
        } catch (IOException e) {
            throw new RuntimeException("Failed to save users", e);
        }
    }

    // Find user by ID
    private User findUserById(List<User> users, int userId) {
        return users.stream()
                .filter(user -> user.getId() == userId)
                .findFirst()
                .orElse(null);
    }

    // GET /users – list of users
    @GetMapping("/users")
    public ResponseEntity<List<User>> getUsers() {
        List<User> users = loadUsers();
        return ResponseEntity.ok(users);
    }

    // GET /users/{id} – single user
    @GetMapping("/users/{userId}")
    public ResponseEntity<?> getUser(@PathVariable int userId) {
        List<User> users = loadUsers();
        User user = findUserById(users, userId);

        if (user != null) {
            return ResponseEntity.ok(user);
        } else {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                    .body(Collections.singletonMap("error", "User not found"));
        }
    }

    // POST /users – add user
    @PostMapping("/users")
    public ResponseEntity<?> createUser(@RequestBody Map<String, Object> requestData) {
        // Validate required fields
        if (requestData == null ||
                !requestData.containsKey("name") ||
                !requestData.containsKey("email")) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Collections.singletonMap("error", "Missing name or email"));
        }

        List<User> users = loadUsers();

        // Find the highest ID and increment
        int newId = users.stream()
                .mapToInt(User::getId)
                .max()
                .orElse(0) + 1;

        User newUser = new User(
                newId,
                requestData.get("name").toString(),
                requestData.get("email").toString()
        );

        users.add(newUser);
        saveUsers(users);

        return ResponseEntity.status(HttpStatus.CREATED).body(newUser);
    }

    // PUT /users/{id} – update user
    @PutMapping("/users/{userId}")
    public ResponseEntity<?> updateUser(@PathVariable int userId,
                                        @RequestBody Map<String, Object> requestData) {
        if (requestData == null) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Collections.singletonMap("error", "Invalid JSON"));
        }

        List<User> users = loadUsers();
        User user = findUserById(users, userId);

        if (user == null) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                    .body(Collections.singletonMap("error", "User not found"));
        }

        // Update user fields if provided
        if (requestData.containsKey("name")) {
            user.setName(requestData.get("name").toString());
        }
        if (requestData.containsKey("email")) {
            user.setEmail(requestData.get("email").toString());
        }

        saveUsers(users);
        return ResponseEntity.ok(user);
    }

    // DELETE /users/{id} – delete user
    @DeleteMapping("/users/{userId}")
    public ResponseEntity<?> deleteUser(@PathVariable int userId) {
        List<User> users = loadUsers();
        int initialSize = users.size();

        // Remove user with matching ID
        users.removeIf(user -> user.getId() == userId);

        if (users.size() == initialSize) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                    .body(Collections.singletonMap("error", "User not found"));
        }

        saveUsers(users);
        return ResponseEntity.ok(Collections.singletonMap("message", "User deleted"));
    }

    // User data class
    public static class User {
        private int id;
        private String name;
        private String email;

        public User() {}

        public User(int id, String name, String email) {
            this.id = id;
            this.name = name;
            this.email = email;
        }

        public int getId() {
            return id;
        }

        public void setId(int id) {
            this.id = id;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public String getEmail() {
            return email;
        }

        public void setEmail(String email) {
            this.email = email;
        }
    }
}