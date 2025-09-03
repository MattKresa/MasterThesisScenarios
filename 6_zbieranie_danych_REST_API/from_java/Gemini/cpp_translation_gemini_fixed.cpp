#include "crow.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

using json = nlohmann::json;

// --- User Model ---
struct User {
    int id;
    std::string name;
    std::string email;

    // Default constructor
    User() : id(0) {}

    // Parameterized constructor
    User(int id, const std::string& name, const std::string& email)
        : id(id), name(name), email(email) {
    }

    // Serialization function to convert User to a JSON object
    json toJson() const {
        json j;
        j["id"] = id;
        j["name"] = name;
        j["email"] = email;
        return j;
    }

    // Deserialization function to convert a JSON object to a User
    static User fromJson(const json& j) {
        User u;
        if (j.count("id")) u.id = j["id"].get<int>();
        if (j.count("name")) u.name = j["name"].get<std::string>();
        if (j.count("email")) u.email = j["email"].get<std::string>();
        return u;
    }
};

// --- Data Persistence Functions ---
const std::string DATA_FILE = "users.json";

std::vector<User> loadUsers() {
    std::vector<User> users;
    std::ifstream file(DATA_FILE);
    if (!file.is_open()) {
        return users;
    }

    try {
        json j;
        file >> j;
        for (const auto& user_json : j) {
            users.push_back(User::fromJson(user_json));
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return {};
    }
    return users;
}

void saveUsers(const std::vector<User>& users) {
    std::ofstream file(DATA_FILE);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << DATA_FILE << " for writing." << std::endl;
        return;
    }

    json j_array = json::array();
    for (const auto& user : users) {
        j_array.push_back(user.toJson());
    }
    file << j_array.dump(4); // Pretty print with 4 spaces
}

// --- Main Application ---
int main() {
    crow::SimpleApp app;

    // GET /users - Get all users
    CROW_ROUTE(app, "/users")
        ([]() {
        std::vector<User> users = loadUsers();
        json j_array = json::array();
        for (const auto& user : users) {
            j_array.push_back(user.toJson());
        }
        return crow::response(j_array.dump());
            });

    // GET /users/{id} - Get a single user
    CROW_ROUTE(app, "/users/<int>")
        ([](int id) {
        std::vector<User> users = loadUsers();
        auto it = std::find_if(users.begin(), users.end(), [id](const User& u) {
            return u.id == id;
            });

        if (it != users.end()) {
            return crow::response(it->toJson().dump());
        }
        else {
            return crow::response(404, "{\"error\":\"User not found\"}");
        }
            });

    // POST /users - Add a new user
    CROW_ROUTE(app, "/users").methods("POST"_method)
        ([](const crow::request& req) {
        json j;
        try {
            j = json::parse(req.body);
        }
        catch (const json::parse_error& e) {
            return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        }

        if (!j.contains("name") || !j.contains("email")) {
            return crow::response(400, "{\"error\":\"Missing name or email\"}");
        }

        std::vector<User> users = loadUsers();
        int newId = 0;
        if (!users.empty()) {
            newId = users.back().id;
            for (const auto& user : users) {
                if (user.id > newId) {
                    newId = user.id;
                }
            }
        }
        newId++;

        User newUser(newId, j["name"].get<std::string>(), j["email"].get<std::string>());
        users.push_back(newUser);
        saveUsers(users);

        return crow::response(201, newUser.toJson().dump());
            });

    // PUT /users/{id} - Update a user
    CROW_ROUTE(app, "/users/<int>").methods("PUT"_method)
        ([](const crow::request& req, int id) {
        json j;
        try {
            j = json::parse(req.body);
        }
        catch (const json::parse_error& e) {
            return crow::response(400, "{\"error\":\"Invalid JSON\"}");
        }

        std::vector<User> users = loadUsers();
        for (auto& user : users) {
            if (user.id == id) {
                if (j.contains("name")) user.name = j["name"].get<std::string>();
                if (j.contains("email")) user.email = j["email"].get<std::string>();
                saveUsers(users);
                return crow::response(200, user.toJson().dump());
            }
        }
        return crow::response(404, "{\"error\":\"User not found\"}");
            });

    // DELETE /users/{id} - Delete a user
    CROW_ROUTE(app, "/users/<int>").methods("DELETE"_method)
        ([](int id) {
        std::vector<User> users = loadUsers();
        auto it = std::remove_if(users.begin(), users.end(), [id](const User& u) {
            return u.id == id;
            });

        if (it != users.end()) {
            users.erase(it, users.end());
            saveUsers(users);
            return crow::response(200, "{\"message\":\"User deleted\"}");
        }
        else {
            return crow::response(404, "{\"error\":\"User not found\"}");
        }
            });

    app.port(18080).run();

    return 0;
}