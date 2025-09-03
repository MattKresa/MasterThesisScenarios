#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <sstream>
#include <httplib.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

class User {
private:
    int id;
    string name;
    string email;

public:
    User() : id(0) {}

    User(int id, const string& name, const string& email)
        : id(id), name(name), email(email) {
    }

    // Getters
    int getId() const { return id; }
    string getName() const { return name; }
    string getEmail() const { return email; }

    // Setters
    void setId(int id) { this->id = id; }
    void setName(const string& name) { this->name = name; }
    void setEmail(const string& email) { this->email = email; }

    // Convert to JSON
    json toJson() const {
        return json{
            {"id", id},
            {"name", name},
            {"email", email}
        };
    }

    // Create from JSON
    static User fromJson(const json& j) {
        User user;
        if (j.contains("id")) user.id = j["id"];
        if (j.contains("name")) user.name = j["name"];
        if (j.contains("email")) user.email = j["email"];
        return user;
    }
};

class UserApiApplication {
private:
    const string DATA_FILE = "users.json";
    httplib::Server server;

    // Load users from file
    vector<User> loadUsers() {
        vector<User> users;
        try {
            ifstream file(DATA_FILE);
            if (!file.is_open()) {
                return users;
            }

            json j;
            file >> j;

            for (const auto& userJson : j) {
                users.push_back(User::fromJson(userJson));
            }
        }
        catch (const exception& e) {
            cerr << "Error loading users: " << e.what() << endl;
        }
        return users;
    }

    // Save users to file
    void saveUsers(const vector<User>& users) {
        try {
            json j = json::array();
            for (const auto& user : users) {
                j.push_back(user.toJson());
            }

            ofstream file(DATA_FILE);
            file << j.dump(2); // Pretty print with 2 spaces
        }
        catch (const exception& e) {
            cerr << "Error saving users: " << e.what() << endl;
        }
    }

    // Find user by ID
    optional<User> findUserById(const vector<User>& users, int id) {
        auto it = find_if(users.begin(), users.end(),
            [id](const User& user) { return user.getId() == id; });

        if (it != users.end()) {
            return *it;
        }
        return nullopt;
    }

    // Generate new ID
    int generateNewId(const vector<User>& users) {
        int maxId = 0;
        for (const auto& user : users) {
            if (user.getId() > maxId) {
                maxId = user.getId();
            }
        }
        return maxId + 1;
    }

public:
    UserApiApplication() {
        setupRoutes();
    }

    void setupRoutes() {
        // Enable CORS
        server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
            res.set_header("Access-Control-Allow-Headers", "Content-Type");
            return httplib::Server::HandlerResponse::Unhandled;
            });

        // Handle OPTIONS requests for CORS
        server.Options("/users.*", [](const httplib::Request&, httplib::Response& res) {
            return;
            });

        // GET /users - get all users
        server.Get("/users", [this](const httplib::Request& req, httplib::Response& res) {
            vector<User> users = loadUsers();
            json j = json::array();

            for (const auto& user : users) {
                j.push_back(user.toJson());
            }

            res.set_content(j.dump(), "application/json");
            });

        // GET /users/{id} - get single user
        server.Get(R"(/users/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            int id = stoi(req.matches[1]);
            vector<User> users = loadUsers();

            auto userOpt = findUserById(users, id);
            if (userOpt.has_value()) {
                res.set_content(userOpt->toJson().dump(), "application/json");
            }
            else {
                json error = { {"error", "User not found"} };
                res.status = 404;
                res.set_content(error.dump(), "application/json");
            }
            });

        // POST /users - add new user
        server.Post("/users", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                json j = json::parse(req.body);

                if (!j.contains("name") || !j.contains("email") ||
                    j["name"].is_null() || j["email"].is_null()) {
                    json error = { {"error", "Missing name or email"} };
                    res.status = 400;
                    res.set_content(error.dump(), "application/json");
                    return;
                }

                vector<User> users = loadUsers();
                int newId = generateNewId(users);

                User newUser(newId, j["name"], j["email"]);
                users.push_back(newUser);
                saveUsers(users);

                res.status = 201;
                res.set_content(newUser.toJson().dump(), "application/json");

            }
            catch (const exception& e) {
                json error = { {"error", "Invalid JSON"} };
                res.status = 400;
                res.set_content(error.dump(), "application/json");
            }
            });

        // PUT /users/{id} - update user
        server.Put(R"(/users/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                int id = stoi(req.matches[1]);
                json j = json::parse(req.body);

                vector<User> users = loadUsers();

                for (auto& user : users) {
                    if (user.getId() == id) {
                        if (j.contains("name") && !j["name"].is_null()) {
                            user.setName(j["name"]);
                        }
                        if (j.contains("email") && !j["email"].is_null()) {
                            user.setEmail(j["email"]);
                        }

                        saveUsers(users);
                        res.set_content(user.toJson().dump(), "application/json");
                        return;
                    }
                }

                json error = { {"error", "User not found"} };
                res.status = 404;
                res.set_content(error.dump(), "application/json");

            }
            catch (const exception& e) {
                json error = { {"error", "Invalid JSON or ID"} };
                res.status = 400;
                res.set_content(error.dump(), "application/json");
            }
            });

        // DELETE /users/{id} - delete user
        server.Delete(R"(/users/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                int id = stoi(req.matches[1]);
                vector<User> users = loadUsers();

                auto it = remove_if(users.begin(), users.end(),
                    [id](const User& user) { return user.getId() == id; });

                if (it != users.end()) {
                    users.erase(it, users.end());
                    saveUsers(users);
                    json success = { {"message", "User deleted"} };
                    res.set_content(success.dump(), "application/json");
                }
                else {
                    json error = { {"error", "User not found"} };
                    res.status = 404;
                    res.set_content(error.dump(), "application/json");
                }

            }
            catch (const exception& e) {
                json error = { {"error", "Invalid ID"} };
                res.status = 400;
                res.set_content(error.dump(), "application/json");
            }
            });
    }

    void run(const string& host = "0.0.0.0", int port = 8080) {
        cout << "Starting server at http://" << host << ":" << port << endl;
        cout << "API endpoints:" << endl;
        cout << "  GET    /users" << endl;
        cout << "  GET    /users/{id}" << endl;
        cout << "  POST   /users" << endl;
        cout << "  PUT    /users/{id}" << endl;
        cout << "  DELETE /users/{id}" << endl;

        server.listen(host.c_str(), port);
    }
};

int main() {
    UserApiApplication app;
    app.run();
    return 0;
}

/*
Build instructions:

1. Install dependencies:
   - cpp-httplib: https://github.com/yhirose/cpp-httplib
   - nlohmann/json: https://github.com/nlohmann/json

2. For Ubuntu/Debian:
   sudo apt-get install nlohmann-json3-dev

3. Download httplib.h from the cpp-httplib repository

4. Compile:
   g++ -std=c++17 -I. user_api.cpp -o user_api -pthread

5. Run:
   ./user_api

API will be available at: http://localhost:8080/users
*/