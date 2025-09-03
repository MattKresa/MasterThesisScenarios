package org.example;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.web.bind.annotation.*;
import org.springframework.http.ResponseEntity;
import org.springframework.http.HttpStatus;

import java.io.*;
import java.util.*;
import java.util.stream.Collectors;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.core.type.TypeReference;

@SpringBootApplication
@RestController
public class UserApiApplication {

    private static final String DATA_FILE = "users.json";
    private static final ObjectMapper objectMapper = new ObjectMapper();

    public static void main(String[] args) {
        SpringApplication.run(UserApiApplication.class, args);
    }

    // Helper methods
    private List<Map<String, Object>> loadUsers() throws IOException {
        File file = new File(DATA_FILE);
        if (!file.exists()) {
            return new ArrayList<>();
        }
        return objectMapper.readValue(file, new TypeReference<List<Map<String, Object>>>() {});
    }

    private void saveUsers(List<Map<String, Object>> users) throws IOException {
        objectMapper.writerWithDefaultPrettyPrinter().writeValue(new File(DATA_FILE), users);
    }

    // Endpoint GET - user list
    @GetMapping("/users")
    public ResponseEntity<List<Map<String, Object>>> getUsers() {
        try {
            return ResponseEntity.ok(loadUsers());
        } catch (IOException e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Endpoint GET – single user
    @GetMapping("/users/{id}")
    public ResponseEntity<Object> getUser(@PathVariable int id) {
        try {
            List<Map<String, Object>> users = loadUsers();
            Optional<Map<String, Object>> user = users.stream()
                    .filter(u -> u.get("id").equals(id))
                    .findFirst();

            if (user.isPresent()) {
                return ResponseEntity.ok(user.get());
            } else {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }
        } catch (IOException e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Endpoint POST – add users
    @PostMapping("/users")
    public ResponseEntity<Object> addUser(@RequestBody Map<String, Object> payload) {
        if (!payload.containsKey("name") || !payload.containsKey("email")) {
            Map<String, String> error = new HashMap<>();
            error.put("error", "Missing name or email");
            return ResponseEntity.badRequest().body(error);
        }

        try {
            List<Map<String, Object>> users = loadUsers();
            int newId = users.stream()
                    .mapToInt(u -> (int) u.get("id"))
                    .max()
                    .orElse(0) + 1;

            Map<String, Object> newUser = new HashMap<>();
            newUser.put("id", newId);
            newUser.put("name", payload.get("name"));
            newUser.put("email", payload.get("email"));

            users.add(newUser);
            saveUsers(users);

            return ResponseEntity.status(HttpStatus.CREATED).body(newUser);
        } catch (IOException e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Endpoint PUT – edit user
    @PutMapping("/users/{id}")
    public ResponseEntity<Object> updateUser(@PathVariable int id, @RequestBody Map<String, Object> payload) {
        try {
            List<Map<String, Object>> users = loadUsers();
            Optional<Map<String, Object>> userOpt = users.stream()
                    .filter(u -> u.get("id").equals(id))
                    .findFirst();

            if (userOpt.isPresent()) {
                Map<String, Object> user = userOpt.get();
                if (payload.containsKey("name")) {
                    user.put("name", payload.get("name"));
                }
                if (payload.containsKey("email")) {
                    user.put("email", payload.get("email"));
                }
                saveUsers(users);
                return ResponseEntity.ok(user);
            } else {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }
        } catch (IOException e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }

    // Endpoint DELETE – delete user
    @DeleteMapping("/users/{id}")
    public ResponseEntity<Object> deleteUser(@PathVariable int id) {
        try {
            List<Map<String, Object>> users = loadUsers();
            List<Map<String, Object>> newUsers = users.stream()
                    .filter(u -> !u.get("id").equals(id))
                    .collect(Collectors.toList());

            if (newUsers.size() == users.size()) {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }

            saveUsers(newUsers);
            Map<String, String> response = new HashMap<>();
            response.put("message", "User deleted");
            return ResponseEntity.ok(response);
        } catch (IOException e) {
            return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR).build();
        }
    }
}