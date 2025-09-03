#include <crow.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <algorithm>
#include <optional>

using json = nlohmann::json;

const std::string DATA_FILE = "users.json";

// Model użytkownika
struct User {
    int id;
    std::string name;
    std::string email;

    User() : id(0), name(""), email("") {}
    User(int id, const std::string& name, const std::string& email)
        : id(id), name(name), email(email) {
    }

    json to_json() const {
        return {
            {"id", id},
            {"name", name},
            {"email", email}
        };
    }

    static User from_json(const json& j) {
        User user;
        if (j.contains("id")) user.id = j["id"];
        if (j.contains("name")) user.name = j["name"];
        if (j.contains("email")) user.email = j["email"];
        return user;
    }
};

// Załaduj użytkowników z pliku
std::vector<User> loadUsers() {
    std::vector<User> users;
    try {
        std::ifstream file(DATA_FILE);
        if (!file.good()) return users;

        json j;
        file >> j;
        for (const auto& item : j) {
            users.push_back(User::from_json(item));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading users: " << e.what() << std::endl;
    }
    return users;
}

// Zapisz użytkowników do pliku
void saveUsers(const std::vector<User>& users) {
    try {
        json j;
        for (const auto& user : users) {
            j.push_back(user.to_json());
        }

        std::ofstream file(DATA_FILE);
        file << j.dump(2);  // Pretty print with 2 spaces indent
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving users: " << e.what() << std::endl;
    }
}

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/users")
        .methods("GET"_method)([]() {
        auto users = loadUsers();
        json response;
        for (const auto& user : users) {
            response.push_back(user.to_json());
        }
        return crow::response(200, response.dump());
            });

    CROW_ROUTE(app, "/users/<int>")
        .methods("GET"_method)([](int id) {
        auto users = loadUsers();
        auto it = std::find_if(users.begin(), users.end(),
            [id](const User& u) { return u.id == id; });

        if (it != users.end()) {
            return crow::response(200, it->to_json().dump());
        }
        else {
            return crow::response(404, json{ {"error", "User not found"} }.dump());
        }
            });

    CROW_ROUTE(app, "/users")
        .methods("POST"_method)([](const crow::request& req) {
        try {
            auto body = json::parse(req.body);

            if (!body.contains("name") || !body.contains("email")) {
                return crow::response(400, json{ {"error", "Missing name or email"} }.dump());
            }

            auto users = loadUsers();
            int new_id = 0;
            if (!users.empty()) {
                auto max_it = std::max_element(users.begin(), users.end(),
                    [](const User& a, const User& b) { return a.id < b.id; });
                new_id = max_it->id + 1;
            }

            User new_user(new_id, body["name"], body["email"]);
            users.push_back(new_user);
            saveUsers(users);

            auto response = crow::response(201, new_user.to_json().dump());
            response.add_header("Content-Type", "application/json");
            return response;
        }
        catch (const std::exception& e) {
            return crow::response(400, json{ {"error", "Invalid request"} }.dump());
        }
            });

    CROW_ROUTE(app, "/users/<int>")
        .methods("PUT"_method)([](const crow::request& req, int id) {
        try {
            auto body = json::parse(req.body);
            auto users = loadUsers();

            for (auto& user : users) {
                if (user.id == id) {
                    if (body.contains("name")) user.name = body["name"];
                    if (body.contains("email")) user.email = body["email"];
                    saveUsers(users);
                    return crow::response(200, user.to_json().dump());
                }
            }

            return crow::response(404, json{ {"error", "User not found"} }.dump());
        }
        catch (const std::exception& e) {
            return crow::response(400, json{ {"error", "Invalid request"} }.dump());
        }
            });

    CROW_ROUTE(app, "/users/<int>")
        .methods("DELETE"_method)([](int id) {
        auto users = loadUsers();
        auto new_end = std::remove_if(users.begin(), users.end(),
            [id](const User& u) { return u.id == id; });

        if (new_end != users.end()) {
            users.erase(new_end, users.end());
            saveUsers(users);
            return crow::response(200, json{ {"message", "User deleted"} }.dump());
        }
        else {
            return crow::response(404, json{ {"error", "User not found"} }.dump());
        }
            });

    app.port(8080).multithreaded().run();
    return 0;
}