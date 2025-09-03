#include "crow.h"         // Crow web framework header
#include "nlohmann/json.hpp"           // nlohmann::json
#include <fstream>
#include <vector>
#include <optional>
#include <algorithm>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

struct User {
    int id;
    std::string name;
    std::string email;
};

// Serialize to JSON
void to_json(json& j, const User& u) {
    j = json{ {"id", u.id}, {"name", u.name}, {"email", u.email} };
}

// Deserialize from JSON
void from_json(const json& j, User& u) {
    j.at("id").get_to(u.id);
    j.at("name").get_to(u.name);
    j.at("email").get_to(u.email);
}

class UserApi {
    const std::string DATA_FILE = "users.json";

    std::vector<User> loadUsers() {
        if (!fs::exists(DATA_FILE)) return {};
        std::ifstream in(DATA_FILE);
        if (!in.is_open()) return {};

        json j;
        in >> j;
        return j.get<std::vector<User>>();
    }

    void saveUsers(const std::vector<User>& users) {
        std::ofstream out(DATA_FILE);
        json j = users;
        out << j.dump(4);
    }

public:
    void setupRoutes(crow::SimpleApp& app) {
        // GET /users
        CROW_ROUTE(app, "/users").methods(crow::HTTPMethod::GET)(
            [this]() {
                auto users = loadUsers();
                return crow::response{ json(users).dump() };
            }
            );

        // GET /users/<id>
        CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::GET)(
            [this](int id) {
                auto users = loadUsers();
                auto it = std::find_if(users.begin(), users.end(),
                    [id](const User& u) { return u.id == id; });
                if (it != users.end()) {
                    return crow::response{ json(*it).dump() };
                }
                return crow::response(404, json{ {"error", "User not found"} }.dump());
            }
            );

        // POST /users
        CROW_ROUTE(app, "/users").methods(crow::HTTPMethod::POST)(
            [this](const crow::request& req) {
                try {
                    json j = json::parse(req.body);
                    if (!j.contains("name") || !j.contains("email")) {
                        return crow::response(400, json{ {"error", "Missing name or email"} }.dump());
                    }
                    auto users = loadUsers();
                    int newId = 0;
                    for (auto& u : users) newId = std::max(newId, u.id);
                    newId++;

                    User newUser{ newId, j["name"], j["email"] };
                    users.push_back(newUser);
                    saveUsers(users);

                    return crow::response(201, json(newUser).dump());
                }
                catch (...) {
                    return crow::response(400, json{ {"error", "Invalid JSON"} }.dump());
                }
            }
            );

        // PUT /users/<id>
        CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::PUT)(
            [this](const crow::request& req, int id) {
                auto users = loadUsers();
                json j;
                try {
                    j = json::parse(req.body);
                }
                catch (...) {
                    return crow::response(400, json{ {"error", "Invalid JSON"} }.dump());
                }

                for (auto& u : users) {
                    if (u.id == id) {
                        if (j.contains("name")) u.name = j["name"];
                        if (j.contains("email")) u.email = j["email"];
                        saveUsers(users);
                        return crow::response(json(u).dump());
                    }
                }
                return crow::response(404, json{ {"error", "User not found"} }.dump());
            }
            );

        // DELETE /users/<id>
        CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::Delete)(
            [this](int id) {
                auto users = loadUsers();
                auto oldSize = users.size();
                users.erase(std::remove_if(users.begin(), users.end(),
                    [id](const User& u) { return u.id == id; }),
                    users.end());

                if (users.size() != oldSize) {
                    saveUsers(users);
                    return crow::response(json{ {"message", "User deleted"} }.dump());
                }
                return crow::response(404, json{ {"error", "User not found"} }.dump());
            }
            );
    }
};

int main() {
    crow::SimpleApp app;
    UserApi api;
    api.setupRoutes(app);

    app.port(8080).multithreaded().run();
}
