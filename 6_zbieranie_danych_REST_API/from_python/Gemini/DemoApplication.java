// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/DemoApplication.java) ---
package org.example;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import jakarta.annotation.PostConstruct;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.annotation.Bean;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Service;
import org.springframework.web.bind.annotation.*;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * This is an all-in-one Spring Boot application containing the main class,
 * model, service, and controller in a single file for a self-contained example.
 *
 * It provides a REST API for managing users, with data stored in a 'users.json' file.
 */
@SpringBootApplication
public class DemoApplication {

    public static void main(String[] args) {
        SpringApplication.run(DemoApplication.class, args);
    }

    /**
     * A simple Plain Old Java Object (POJO) representing a User.
     * The @Data, @AllArgsConstructor, and @NoArgsConstructor annotations
     * are from the Lombok library.
     */
    @Data
    @AllArgsConstructor
    @NoArgsConstructor
    public static class User {
        private int id;
        private String name;
        private String email;
    }

    /**
     * Service class for managing user data. It handles the reading and writing
     * to the users.json file.
     */
    @Service
    public static class UserService {

        private final String DATA_FILE = "users.json";
        private final ObjectMapper objectMapper = new ObjectMapper();
        private List<User> users;
        private AtomicInteger idCounter;

        /**
         * Loads the users from the JSON file and initializes the ID counter on startup.
         */
        @PostConstruct
        public void init() {
            loadUsers();
            idCounter = new AtomicInteger(
                    users.stream()
                            .mapToInt(User::getId)
                            .max()
                            .orElse(0)
            );
        }

        /**
         * Loads the list of users from the users.json file.
         */
        private void loadUsers() {
            File file = new File(DATA_FILE);
            if (file.exists()) {
                try {
                    users = objectMapper.readValue(file, new TypeReference<List<User>>() {});
                } catch (IOException e) {
                    System.err.println("Error reading users.json: " + e.getMessage());
                    users = new ArrayList<>();
                }
            } else {
                users = new ArrayList<>();
            }
        }

        /**
         * Saves the current list of users to the users.json file.
         */
        private void saveUsers() {
            try {
                objectMapper.writerWithDefaultPrettyPrinter().writeValue(new File(DATA_FILE), users);
            } catch (IOException e) {
                System.err.println("Error writing to users.json: " + e.getMessage());
            }
        }

        // --- CRUD operations ---

        public List<User> findAll() {
            return users;
        }

        public User findById(int id) {
            return users.stream()
                    .filter(u -> u.getId() == id)
                    .findFirst()
                    .orElse(null);
        }

        public User save(User newUser) {
            newUser.setId(idCounter.incrementAndGet());
            users.add(newUser);
            saveUsers();
            return newUser;
        }

        public User update(int id, User updatedUser) {
            User existingUser = findById(id);
            if (existingUser != null) {
                existingUser.setName(updatedUser.getName());
                existingUser.setEmail(updatedUser.getEmail());
                saveUsers();
                return existingUser;
            }
            return null;
        }

        public boolean delete(int id) {
            boolean removed = users.removeIf(user -> user.getId() == id);
            if (removed) {
                saveUsers();
            }
            return removed;
        }
    }

    /**
     * The REST controller for the User API.
     */
    @RestController
    @RequestMapping("/users")
    public static class UserController {

        private final UserService userService;

        // Dependency injection via constructor
        public UserController(UserService userService) {
            this.userService = userService;
        }

        // Endpoint GET - user list
        @GetMapping
        public ResponseEntity<List<User>> getUsers() {
            List<User> users = userService.findAll();
            return ResponseEntity.ok(users);
        }

        // Endpoint GET - single user
        @GetMapping("/{id}")
        public ResponseEntity<User> getUser(@PathVariable int id) {
            User user = userService.findById(id);
            if (user != null) {
                return ResponseEntity.ok(user);
            }
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
        }

        // Endpoint POST - add user
        @PostMapping
        public ResponseEntity<?> addUser(@RequestBody User newUser) {
            if (newUser.getName() == null || newUser.getEmail() == null) {
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).body("Missing name or email");
            }
            User savedUser = userService.save(newUser);
            return ResponseEntity.status(HttpStatus.CREATED).body(savedUser);
        }

        // Endpoint PUT - edit user
        @PutMapping("/{id}")
        public ResponseEntity<User> updateUser(@PathVariable int id, @RequestBody User updatedUser) {
            User user = userService.update(id, updatedUser);
            if (user != null) {
                return ResponseEntity.ok(user);
            }
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body(null);
        }

        // Endpoint DELETE - delete user
        @DeleteMapping("/{id}")
        public ResponseEntity<String> deleteUser(@PathVariable int id) {
            boolean deleted = userService.delete(id);
            if (deleted) {
                return ResponseEntity.ok("User deleted");
            }
            return ResponseEntity.status(HttpStatus.NOT_FOUND).body("User not found");
        }
    }
}
