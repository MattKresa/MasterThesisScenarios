// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserApiApplication.java) ---
package org.example;

import org.springframework.boot.*;
import org.springframework.boot.autoconfigure.*;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.core.type.TypeReference;

import java.io.File;
import java.util.*;

@SpringBootApplication
@RestController
@RequestMapping("/users")
public class UserApiApplication {

    private final String DATA_FILE = "users.json";
    private final ObjectMapper mapper = new ObjectMapper();

    public static void main(String[] args) {
        SpringApplication.run(UserApiApplication.class, args);
    }

    // User model
    static class User {
        private int id;
        private String name;
        private String email;

        public User() {}

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

    // Load users from file
    private List<User> loadUsers() {
        try {
            File file = new File(DATA_FILE);
            if (!file.exists()) return new ArrayList<>();
            return mapper.readValue(file, new TypeReference<List<User>>() {});
        } catch (Exception e) {
            e.printStackTrace();
            return new ArrayList<>();
        }
    }

    // Save users to file
    private void saveUsers(List<User> users) {
        try {
            mapper.writerWithDefaultPrettyPrinter().writeValue(new File(DATA_FILE), users);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    // GET /users – list
    @GetMapping
    public List<User> getUsers() {
        return loadUsers();
    }

    // GET /users/{id} – single user
    @GetMapping("/{id}")
    public ResponseEntity<Object> getUser(@PathVariable int id) {
        Optional<User> user = loadUsers().stream()
                .filter(u -> u.getId() == id)
                .findFirst();

        if (user.isPresent()) {
            return ResponseEntity.ok((Object) user.get());
        } else {
            return ResponseEntity.status(404).body((Object) Map.of("error", "User not found"));
        }
    }

    // POST /users – add
    @PostMapping
    public ResponseEntity<Object> addUser(@RequestBody User user) {
        if (user.getName() == null || user.getEmail() == null) {
            return ResponseEntity.badRequest().body(Map.of("error", "Missing name or email"));
        }

        List<User> users = loadUsers();
        int newId = users.stream().mapToInt(User::getId).max().orElse(0) + 1;
        user.setId(newId);
        users.add(user);
        saveUsers(users);
        return ResponseEntity.status(201).body(user);
    }

    // PUT /users/{id} – edit
    @PutMapping("/{id}")
    public ResponseEntity<Object> updateUser(@PathVariable int id, @RequestBody User userUpdate) {
        List<User> users = loadUsers();
        for (User user : users) {
            if (user.getId() == id) {
                if (userUpdate.getName() != null) user.setName(userUpdate.getName());
                if (userUpdate.getEmail() != null) user.setEmail(userUpdate.getEmail());
                saveUsers(users);
                return ResponseEntity.ok(user);
            }
        }
        return ResponseEntity.status(404).body(Map.of("error", "User not found"));
    }

    // DELETE /users/{id} – delete
    @DeleteMapping("/{id}")
    public ResponseEntity<Object> deleteUser(@PathVariable int id) {
        List<User> users = loadUsers();
        boolean removed = users.removeIf(u -> u.getId() == id);
        if (removed) {
            saveUsers(users);
            return ResponseEntity.ok(Map.of("message", "User deleted"));
        } else {
            return ResponseEntity.status(404).body(Map.of("error", "User not found"));
        }
    }
}
