// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserApiApp.java) ---
package org.example;

import org.springframework.boot.*;
import org.springframework.boot.autoconfigure.*;
import org.springframework.web.bind.annotation.*;
import org.springframework.http.ResponseEntity;
import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.File;
import java.io.IOException;
import java.util.*;

@SpringBootApplication
@RestController
@RequestMapping("/users")
public class UserApiApp {

    private static final String DATA_FILE = "users.json";
    private final ObjectMapper mapper = new ObjectMapper();

    public static void main(String[] args) {
        SpringApplication.run(UserApiApp.class, args);
    }

    // ===== Utility functions =====
    private List<Map<String, Object>> loadUsers() {
        File file = new File(DATA_FILE);
        if (!file.exists()) {
            return new ArrayList<>();
        }
        try {
            return mapper.readValue(file, new TypeReference<List<Map<String, Object>>>() {});
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void saveUsers(List<Map<String, Object>> users) {
        try {
            mapper.writerWithDefaultPrettyPrinter().writeValue(new File(DATA_FILE), users);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    // ===== Endpoints =====

    // GET - all users
    @GetMapping
    public ResponseEntity<List<Map<String, Object>>> getUsers() {
        return ResponseEntity.ok(loadUsers());
    }

    // GET - single user
    @GetMapping("/{id}")
    public ResponseEntity<?> getUser(@PathVariable int id) {
        return loadUsers().stream()
                .filter(u -> ((Number) u.get("id")).intValue() == id)
                .findFirst()
                .<ResponseEntity<?>>map(ResponseEntity::ok)
                .orElse(ResponseEntity.status(404).body(Map.of("error", "User not found")));
    }

    // POST - add user
    @PostMapping
    public ResponseEntity<?> addUser(@RequestBody Map<String, Object> data) {
        if (!data.containsKey("name") || !data.containsKey("email")) {
            return ResponseEntity.badRequest().body(Map.of("error", "Missing name or email"));
        }

        List<Map<String, Object>> users = loadUsers();
        int newId = users.stream()
                .mapToInt(u -> ((Number) u.get("id")).intValue())
                .max()
                .orElse(0) + 1;

        Map<String, Object> newUser = new LinkedHashMap<>();
        newUser.put("id", newId);
        newUser.put("name", data.get("name"));
        newUser.put("email", data.get("email"));

        users.add(newUser);
        saveUsers(users);

        return ResponseEntity.status(201).body(newUser);
    }

    // PUT - update user
    @PutMapping("/{id}")
    public ResponseEntity<?> updateUser(@PathVariable int id, @RequestBody Map<String, Object> data) {
        List<Map<String, Object>> users = loadUsers();
        for (Map<String, Object> u : users) {
            if (((Number) u.get("id")).intValue() == id) {
                if (data.containsKey("name")) u.put("name", data.get("name"));
                if (data.containsKey("email")) u.put("email", data.get("email"));
                saveUsers(users);
                return ResponseEntity.ok(u);
            }
        }
        return ResponseEntity.status(404).body(Map.of("error", "User not found"));
    }

    // DELETE - delete user
    @DeleteMapping("/{id}")
    public ResponseEntity<?> deleteUser(@PathVariable int id) {
        List<Map<String, Object>> users = loadUsers();
        List<Map<String, Object>> newUsers = new ArrayList<>();

        for (Map<String, Object> u : users) {
            if (((Number) u.get("id")).intValue() != id) {
                newUsers.add(u);
            }
        }

        if (newUsers.size() == users.size()) {
            return ResponseEntity.status(404).body(Map.of("error", "User not found"));
        }

        saveUsers(newUsers);
        return ResponseEntity.ok(Map.of("message", "User deleted"));
    }
}
