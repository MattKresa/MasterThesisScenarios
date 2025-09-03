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
public class UserApiApplication {
    public static void main(String[] args) {
        SpringApplication.run(UserApiApplication.class, args);
    }
}

// File: UserController.java
@RestController
@RequestMapping("/users")
class UserController {

    private static final String DATA_FILE = "users.json";
    private final ObjectMapper objectMapper = new ObjectMapper();

    // Load users from file
    private List<User> loadUsers() {
        File file = new File(DATA_FILE);
        if (!file.exists()) return new ArrayList<>();
        try {
            return objectMapper.readValue(file, new TypeReference<List<User>>() {});
        } catch (IOException e) {
            e.printStackTrace();
            return new ArrayList<>();
        }
    }

    // Save users to file
    private void saveUsers(List<User> users) {
        try {
            objectMapper.writerWithDefaultPrettyPrinter().writeValue(new File(DATA_FILE), users);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // GET /users – list all users
    @GetMapping
    public List<User> getUsers() {
        return loadUsers();
    }

    // GET /users/{id} – get single user
    @GetMapping("/{id}")
    public ResponseEntity<?> getUser(@PathVariable int id) {
        List<User> users = loadUsers();
        return users.stream()
                .filter(u -> u.getId() == id)
                .findFirst()
                .<ResponseEntity<?>>map(ResponseEntity::ok)
                .orElse(ResponseEntity.status(HttpStatus.NOT_FOUND)
                        .body(Map.of("error", "User not found")));
    }

    // POST /users – add user
    @PostMapping
    public ResponseEntity<?> createUser(@RequestBody Map<String, Object> data) {
        if (!data.containsKey("name") || !data.containsKey("email")) {
            return ResponseEntity.status(HttpStatus.BAD_REQUEST)
                    .body(Map.of("error", "Missing name or email"));
        }

        List<User> users = loadUsers();
        int newId = users.stream().mapToInt(User::getId).max().orElse(0) + 1;

        User newUser = new User(newId, data.get("name").toString(), data.get("email").toString());
        users.add(newUser);
        saveUsers(users);

        return ResponseEntity.status(HttpStatus.CREATED).body(newUser);
    }

    // PUT /users/{id} – update user
    @PutMapping("/{id}")
    public ResponseEntity<?> updateUser(@PathVariable int id, @RequestBody Map<String, Object> data) {
        List<User> users = loadUsers();
        for (User user : users) {
            if (user.getId() == id) {
                if (data.containsKey("name")) user.setName(data.get("name").toString());
                if (data.containsKey("email")) user.setEmail(data.get("email").toString());
                saveUsers(users);
                return ResponseEntity.ok(user);
            }
        }
        return ResponseEntity.status(HttpStatus.NOT_FOUND)
                .body(Map.of("error", "User not found"));
    }

    // DELETE /users/{id} – delete user
    @DeleteMapping("/{id}")
    public ResponseEntity<?> deleteUser(@PathVariable int id) {
        List<User> users = loadUsers();
        int before = users.size();
        users.removeIf(u -> u.getId() == id);
        if (users.size() == before) {
            return ResponseEntity.status(HttpStatus.NOT_FOUND)
                    .body(Map.of("error", "User not found"));
        }
        saveUsers(users);
        return ResponseEntity.ok(Map.of("message", "User deleted"));
    }
}

// File: User.java
class User {
    private int id;
    private String name;
    private String email;

    public User() {} // Required for JSON deserialization

    public User(int id, String name, String email) {
        this.id = id;
        this.name = name;
        this.email = email;
    }

    public int getId() { return id; }
    public void setId(int id) { this.id = id; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public String getEmail() { return email; }
    public void setEmail(String email) { this.email = email; }
}
