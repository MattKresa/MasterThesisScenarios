// --- All-in-One Spring Boot Application (src/main/java/com/example/demo/UserApiApplication.java) ---
package org.example;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.web.bind.annotation.*;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.File;
import java.io.IOException;
import java.util.*;

@SpringBootApplication
public class UserApiApplication {

    public static void main(String[] args) {
        SpringApplication.run(UserApiApplication.class, args);
    }

    // User data model
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

        // Getters and Setters
        public int getId() { return id; }
        public void setId(int id) { this.id = id; }

        public String getName() { return name; }
        public void setName(String name) { this.name = name; }

        public String getEmail() { return email; }
        public void setEmail(String email) { this.email = email; }
    }

    // Service layer - Business logic and file operations
    @Service
    public static class UserService {
        private static final String DATA_FILE = "users.json";
        private final ObjectMapper objectMapper = new ObjectMapper();

        public List<User> loadUsers() {
            File file = new File(DATA_FILE);
            if (!file.exists()) {
                return new ArrayList<>();
            }

            try {
                return objectMapper.readValue(file, new TypeReference<List<User>>() {});
            } catch (IOException e) {
                e.printStackTrace();
                return new ArrayList<>();
            }
        }

        public void saveUsers(List<User> users) {
            try {
                objectMapper.writerWithDefaultPrettyPrinter()
                        .writeValue(new File(DATA_FILE), users);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        public List<User> getAllUsers() {
            return loadUsers();
        }

        public Optional<User> getUserById(int userId) {
            List<User> users = loadUsers();
            return users.stream()
                    .filter(user -> user.getId() == userId)
                    .findFirst();
        }

        public User addUser(String name, String email) {
            List<User> users = loadUsers();

            int newId = users.stream()
                    .mapToInt(User::getId)
                    .max()
                    .orElse(0) + 1;

            User newUser = new User(newId, name, email);
            users.add(newUser);
            saveUsers(users);

            return newUser;
        }

        public Optional<User> updateUser(int userId, String name, String email) {
            List<User> users = loadUsers();

            for (User user : users) {
                if (user.getId() == userId) {
                    if (name != null) user.setName(name);
                    if (email != null) user.setEmail(email);
                    saveUsers(users);
                    return Optional.of(user);
                }
            }

            return Optional.empty();
        }

        public boolean deleteUser(int userId) {
            List<User> users = loadUsers();
            int originalSize = users.size();

            users.removeIf(user -> user.getId() == userId);

            if (users.size() < originalSize) {
                saveUsers(users);
                return true;
            }

            return false;
        }
    }

    // REST API Controller
    @RestController
    @RequestMapping("/users")
    @CrossOrigin(origins = "*")
    public static class UserController {

        @Autowired
        private UserService userService;

        // GET /users - Get all users
        @GetMapping
        public ResponseEntity<List<User>> getUsers() {
            List<User> users = userService.getAllUsers();
            return ResponseEntity.ok(users);
        }

        // GET /users/{id} - Get single user
        @GetMapping("/{userId}")
        public ResponseEntity<?> getUser(@PathVariable int userId) {
            Optional<User> user = userService.getUserById(userId);

            if (user.isPresent()) {
                return ResponseEntity.ok(user.get());
            } else {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }
        }

        // POST /users - Add new user
        @PostMapping
        public ResponseEntity<?> addUser(@RequestBody Map<String, String> requestData) {
            String name = requestData.get("name");
            String email = requestData.get("email");

            if (name == null || name.trim().isEmpty() ||
                    email == null || email.trim().isEmpty()) {
                Map<String, String> error = new HashMap<>();
                error.put("error", "Missing name or email");
                return ResponseEntity.status(HttpStatus.BAD_REQUEST).body(error);
            }

            User newUser = userService.addUser(name.trim(), email.trim());
            return ResponseEntity.status(HttpStatus.CREATED).body(newUser);
        }

        // PUT /users/{id} - Update user
        @PutMapping("/{userId}")
        public ResponseEntity<?> updateUser(@PathVariable int userId,
                                            @RequestBody Map<String, String> requestData) {
            String name = requestData.get("name");
            String email = requestData.get("email");

            Optional<User> updatedUser = userService.updateUser(userId, name, email);

            if (updatedUser.isPresent()) {
                return ResponseEntity.ok(updatedUser.get());
            } else {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }
        }

        // DELETE /users/{id} - Delete user
        @DeleteMapping("/{userId}")
        public ResponseEntity<?> deleteUser(@PathVariable int userId) {
            boolean deleted = userService.deleteUser(userId);

            if (deleted) {
                Map<String, String> message = new HashMap<>();
                message.put("message", "User deleted");
                return ResponseEntity.ok(message);
            } else {
                Map<String, String> error = new HashMap<>();
                error.put("error", "User not found");
                return ResponseEntity.status(HttpStatus.NOT_FOUND).body(error);
            }
        }
    }
}

/*
pom.xml - Maven dependencies:

<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0
         http://maven.apache.org/xsd/maven-4.0.0.xsd">
    <modelVersion>4.0.0</modelVersion>

    <parent>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-parent</artifactId>
        <version>3.2.0</version>
        <relativePath/>
    </parent>

    <groupId>com.example</groupId>
    <artifactId>user-api</artifactId>
    <version>0.0.1-SNAPSHOT</version>
    <name>user-api</name>
    <description>User Management API</description>

    <properties>
        <java.version>17</java.version>
    </properties>

    <dependencies>
        <dependency>
            <groupId>org.springframework.boot</groupId>
            <artifactId>spring-boot-starter-web</artifactId>
        </dependency>

        <dependency>
            <groupId>com.fasterxml.jackson.core</groupId>
            <artifactId>jackson-databind</artifactId>
        </dependency>

        <dependency>
            <groupId>org.springframework.boot</groupId>
            <artifactId>spring-boot-starter-test</artifactId>
            <scope>test</scope>
        </dependency>
    </dependencies>

    <build>
        <plugins>
            <plugin>
                <groupId>org.springframework.boot</groupId>
                <artifactId>spring-boot-maven-plugin</artifactId>
            </plugin>
        </plugins>
    </build>
</project>
*/

/*
application.properties (optional - place in src/main/resources/):

server.port=8080
logging.level.com.example=DEBUG
*/