#include "crow.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string DATA_FILE = "users.json";

// Load users from JSON file
json load_users() {
    if (!fs::exists(DATA_FILE)) {
        return json::array();
    }
    std::ifstream file(DATA_FILE);
    json users;
    file >> users;
    return users;
}

// Save users to JSON file
void save_users(const json& users) {
    std::ofstream file(DATA_FILE);
    file << users.dump(4);
}

int main() {
    crow::SimpleApp app;

    // GET - all users
    CROW_ROUTE(app, "/users")
        .methods("GET"_method)
        ([] {
        json users = load_users();
        return crow::response{ users.dump() };
            });

    // GET - single user
    CROW_ROUTE(app, "/users/<int>")
        .methods("GET"_method)
        ([](int user_id) {
        json users = load_users();
        auto it = std::find_if(users.begin(), users.end(), [&](const json& u) {
            return u["id"] == user_id;
            });
        if (it != users.end()) {
            return crow::response{ it->dump() };
        }
        return crow::response(404, R"({"error": "User not found"})");
            });

    // POST - add user
    CROW_ROUTE(app, "/users")
        .methods("POST"_method)
        ([](const crow::request& req) {
        json data = json::parse(req.body, nullptr, false);
        if (data.is_discarded() || !data.contains("name") || !data.contains("email")) {
            return crow::response(400, R"({"error": "Missing name or email"})");
        }

        json users = load_users();
        int new_id = 0;
        for (auto& u : users) {
            new_id = std::max(new_id, u["id"].get<int>());
        }
        new_id++;

        json new_user = {
            {"id", new_id},
            {"name", data["name"]},
            {"email", data["email"]}
        };
        users.push_back(new_user);
        save_users(users);

        return crow::response(201, new_user.dump());
            });

    // PUT - update user
    CROW_ROUTE(app, "/users/<int>")
        .methods("PUT"_method)
        ([](const crow::request& req, int user_id) {
        json data = json::parse(req.body, nullptr, false);
        if (data.is_discarded()) {
            return crow::response(400, R"({"error": "Invalid JSON"})");
        }

        json users = load_users();
        for (auto& u : users) {
            if (u["id"] == user_id) {
                if (data.contains("name")) u["name"] = data["name"];
                if (data.contains("email")) u["email"] = data["email"];
                save_users(users);
                return crow::response(u.dump());
            }
        }
        return crow::response(404, R"({"error": "User not found"})");
            });

    // DELETE - delete user
    CROW_ROUTE(app, "/users/<int>")
        .methods("DELETE"_method)
        ([](int user_id) {
        json users = load_users();
        auto old_size = users.size();
        json new_users = json::array();

        for (auto& u : users) {
            if (u["id"] != user_id) {
                new_users.push_back(u);
            }
        }

        if (new_users.size() == old_size) {
            return crow::response(404, R"({"error": "User not found"})");
        }

        save_users(new_users);
        return crow::response(R"({"message": "User deleted"})");
            });

    app.port(5000).multithreaded().run();
}
