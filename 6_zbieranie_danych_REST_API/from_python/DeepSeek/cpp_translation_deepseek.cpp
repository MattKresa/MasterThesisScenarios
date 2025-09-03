#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace httplib;
using json = nlohmann::json;

const std::string DATA_FILE = "users.json";

// Helper functions
std::vector<json> load_users() {
    std::vector<json> users;
    std::ifstream file(DATA_FILE);

    if (file.good()) {
        json data;
        file >> data;
        if (data.is_array()) {
            for (auto& user : data) {
                users.push_back(user);
            }
        }
    }

    return users;
}

void save_users(const std::vector<json>& users) {
    std::ofstream file(DATA_FILE);
    json j = users;
    file << j.dump(4);
}

int main() {
    Server svr;

    // GET /users - list all users
    svr.Get("/users", [](const Request& req, Response& res) {
        auto users = load_users();
        res.set_content(json(users).dump(), "application/json");
        });

    // GET /users/:id - get single user
    svr.Get(R"(/users/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        auto users = load_users();

        auto it = std::find_if(users.begin(), users.end(),
            [id](const json& user) { return user["id"] == id; });

        if (it != users.end()) {
            res.set_content(it->dump(), "application/json");
        }
        else {
            json error = { {"error", "User not found"} };
            res.status = 404;
            res.set_content(error.dump(), "application/json");
        }
        });

    // POST /users - add new user
    svr.Post("/users", [](const Request& req, Response& res) {
        try {
            json data = json::parse(req.body);

            if (!data.contains("name") || !data.contains("email")) {
                json error = { {"error", "Missing name or email"} };
                res.status = 400;
                res.set_content(error.dump(), "application/json");
                return;
            }

            auto users = load_users();
            int new_id = 1;
            if (!users.empty()) {
                auto max_it = std::max_element(users.begin(), users.end(),
                    [](const json& a, const json& b) { return a["id"] < b["id"]; });
                new_id = (*max_it)["id"].get<int>() + 1;
            }

            json new_user = {
                {"id", new_id},
                {"name", data["name"]},
                {"email", data["email"]}
            };

            users.push_back(new_user);
            save_users(users);

            res.status = 201;
            res.set_content(new_user.dump(), "application/json");
        }
        catch (const std::exception& e) {
            json error = { {"error", "Invalid request"} };
            res.status = 400;
            res.set_content(error.dump(), "application/json");
        }
        });

    // PUT /users/:id - update user
    svr.Put(R"(/users/(\d+))", [](const Request& req, Response& res) {
        try {
            int id = std::stoi(req.matches[1]);
            json data = json::parse(req.body);

            auto users = load_users();
            auto it = std::find_if(users.begin(), users.end(),
                [id](const json& user) { return user["id"] == id; });

            if (it != users.end()) {
                if (data.contains("name")) {
                    (*it)["name"] = data["name"];
                }
                if (data.contains("email")) {
                    (*it)["email"] = data["email"];
                }

                save_users(users);
                res.set_content(it->dump(), "application/json");
            }
            else {
                json error = { {"error", "User not found"} };
                res.status = 404;
                res.set_content(error.dump(), "application/json");
            }
        }
        catch (const std::exception& e) {
            json error = { {"error", "Invalid request"} };
            res.status = 400;
            res.set_content(error.dump(), "application/json");
        }
        });

    // DELETE /users/:id - delete user
    svr.Delete(R"(/users/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        auto users = load_users();

        auto new_users_end = std::remove_if(users.begin(), users.end(),
            [id](const json& user) { return user["id"] == id; });

        if (new_users_end == users.end()) {
            json error = { {"error", "User not found"} };
            res.status = 404;
            res.set_content(error.dump(), "application/json");
        }
        else {
            users.erase(new_users_end, users.end());
            save_users(users);
            json message = { {"message", "User deleted"} };
            res.set_content(message.dump(), "application/json");
        }
        });

    std::cout << "Server started at http://localhost:8080\n";
    svr.listen("localhost", 8080);

    return 0;
}