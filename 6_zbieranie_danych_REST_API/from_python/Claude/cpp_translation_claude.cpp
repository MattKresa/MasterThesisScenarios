#include <httplib.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <filesystem>

using json = nlohmann::json;
using namespace httplib;

const std::string DATA_FILE = "users.json";

// Helper functions
std::vector<json> load_users() {
    if (!std::filesystem::exists(DATA_FILE)) {
        return {};
    }

    std::ifstream file(DATA_FILE);
    if (!file.is_open()) {
        return {};
    }

    json j;
    file >> j;
    file.close();

    return j.get<std::vector<json>>();
}

void save_users(const std::vector<json>& users) {
    std::ofstream file(DATA_FILE);
    if (file.is_open()) {
        json j = users;
        file << j.dump(4);
        file.close();
    }
}

int main() {
    Server server;

    // Enable CORS for all routes
    server.set_pre_routing_handler([](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        return Server::HandlerResponse::Unhandled;
        });

    // Handle OPTIONS requests (for CORS preflight)
    server.Options(".*", [](const Request&, Response& res) {
        return;
        });

    // Endpoint GET - user list
    server.Get("/users", [](const Request& req, Response& res) {
        auto users = load_users();
        json response = users;
        res.set_content(response.dump(), "application/json");
        res.status = 200;
        });

    // Endpoint GET - single user
    server.Get(R"(/users/(\d+))", [](const Request& req, Response& res) {
        int user_id = std::stoi(req.matches[1]);
        auto users = load_users();

        auto it = std::find_if(users.begin(), users.end(),
            [user_id](const json& user) {
                return user["id"] == user_id;
            });

        if (it != users.end()) {
            res.set_content(it->dump(), "application/json");
            res.status = 200;
        }
        else {
            json error = { {"error", "User not found"} };
            res.set_content(error.dump(), "application/json");
            res.status = 404;
        }
        });

    // Endpoint POST - add user
    server.Post("/users", [](const Request& req, Response& res) {
        try {
            json data = json::parse(req.body);

            if (!data.contains("name") || !data.contains("email")) {
                json error = { {"error", "Missing name or email"} };
                res.set_content(error.dump(), "application/json");
                res.status = 400;
                return;
            }

            auto users = load_users();

            // Find max ID
            int new_id = 1;
            if (!users.empty()) {
                auto max_it = std::max_element(users.begin(), users.end(),
                    [](const json& a, const json& b) {
                        return a["id"] < b["id"];
                    });
                new_id = (*max_it)["id"].get<int>() + 1;
            }

            json new_user = {
                {"id", new_id},
                {"name", data["name"]},
                {"email", data["email"]}
            };

            users.push_back(new_user);
            save_users(users);

            res.set_content(new_user.dump(), "application/json");
            res.status = 201;

        }
        catch (const json::exception& e) {
            json error = { {"error", "Invalid JSON"} };
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
        });

    // Endpoint PUT - edit user
    server.Put(R"(/users/(\d+))", [](const Request& req, Response& res) {
        try {
            int user_id = std::stoi(req.matches[1]);
            json data = json::parse(req.body);
            auto users = load_users();

            auto it = std::find_if(users.begin(), users.end(),
                [user_id](json& user) {
                    return user["id"] == user_id;
                });

            if (it != users.end()) {
                if (data.contains("name")) {
                    (*it)["name"] = data["name"];
                }
                if (data.contains("email")) {
                    (*it)["email"] = data["email"];
                }

                save_users(users);
                res.set_content(it->dump(), "application/json");
                res.status = 200;
            }
            else {
                json error = { {"error", "User not found"} };
                res.set_content(error.dump(), "application/json");
                res.status = 404;
            }

        }
        catch (const json::exception& e) {
            json error = { {"error", "Invalid JSON"} };
            res.set_content(error.dump(), "application/json");
            res.status = 400;
        }
        });

    // Endpoint DELETE - delete user
    server.Delete(R"(/users/(\d+))", [](const Request& req, Response& res) {
        int user_id = std::stoi(req.matches[1]);
        auto users = load_users();

        size_t original_size = users.size();
        users.erase(
            std::remove_if(users.begin(), users.end(),
                [user_id](const json& user) {
                    return user["id"] == user_id;
                }),
            users.end()
        );

        if (users.size() == original_size) {
            json error = { {"error", "User not found"} };
            res.set_content(error.dump(), "application/json");
            res.status = 404;
        }
        else {
            save_users(users);
            json message = { {"message", "User deleted"} };
            res.set_content(message.dump(), "application/json");
            res.status = 200;
        }
        });

    std::cout << "Server starting on http://localhost:8080" << std::endl;
    server.listen("localhost", 8080);

    return 0;
}

// CMakeLists.txt content:
/*
cmake_minimum_required(VERSION 3.14)
project(UserAPI)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Add executable
add_executable(user_api main.cpp)

# Link libraries
target_link_libraries(user_api
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# If using vcpkg or manual installation:
# target_include_directories(user_api PRIVATE path/to/httplib)
*/