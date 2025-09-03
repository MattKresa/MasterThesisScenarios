#include "crow.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <vector>
#include <algorithm>

using json = nlohmann::json;
const std::string DATA_FILE = "users.json";

// Load users from file
json load_users() {
    std::ifstream file(DATA_FILE);
    if (!file.is_open()) return json::array();
    json users;
    file >> users;
    return users;
}

// Save users to file
void save_users(const json& users) {
    std::ofstream file(DATA_FILE);
    file << users.dump(4);
}

int main() {
    crow::SimpleApp app;

    // GET /users – list of users
    CROW_ROUTE(app, "/users").methods(crow::HTTPMethod::GET)
        ([]() {
        json users = load_users();
        return crow::response{ users.dump() };
            });

    // GET /users/<id> – single user
    CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::GET)
        ([](int user_id) {
        json users = load_users();
        auto it = std::find_if(users.begin(), users.end(), [&](const json& u) { return u["id"] == user_id; });
        if (it != users.end()) {
            return crow::response{ it->dump() };
        }
        return crow::response(404, R"({"error":"User not found"})");
            });

    // POST /users – adding user
    CROW_ROUTE(app, "/users").methods(crow::HTTPMethod::POST)
        ([](const crow::request& req) {
        auto data = json::parse(req.body, nullptr, false);
        if (!data.is_object() || !data.contains("name") || !data.contains("email"))
            return crow::response(400, R"({"error":"Missing name or email"})");

        json users = load_users();
        int new_id = 0;
        for (auto& u : users) new_id = std::max(new_id, u["id"].get<int>());
        new_id++;

        json new_user = { {"id", new_id}, {"name", data["name"]}, {"email", data["email"]} };
        users.push_back(new_user);
        save_users(users);

        return crow::response(201, new_user.dump());
            });

    // PUT /users/<id> – update user
    CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::PUT)
        ([](const crow::request& req, int user_id) {
        auto data = json::parse(req.body, nullptr, false);
        if (!data.is_object())
            return crow::response(400, R"({"error":"Invalid JSON"})");

        json users = load_users();
        for (auto& u : users) {
            if (u["id"] == user_id) {
                if (data.contains("name")) u["name"] = data["name"];
                if (data.contains("email")) u["email"] = data["email"];
                save_users(users);
                return crow::response{ u.dump() };
            }
        }
        return crow::response(404, R"({"error":"User not found"})");
            });

    // DELETE /users/<id> – delete user
    CROW_ROUTE(app, "/users/<int>").methods(crow::HTTPMethod::Delete)
        ([](int user_id) {
        json users = load_users();
        size_t before = users.size();
        users.erase(std::remove_if(users.begin(), users.end(),
            [&](const json& u) { return u["id"] == user_id; }),
            users.end());
        if (users.size() == before)
            return crow::response(404, R"({"error":"User not found"})");

        save_users(users);
        return crow::response(R"({"message":"User deleted"})");
            });

    app.port(5000).multithreaded().run();
}
