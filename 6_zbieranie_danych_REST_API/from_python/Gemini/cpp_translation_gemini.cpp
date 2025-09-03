// main.cpp
#include "crow.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

// To use nlohmann/json, you can download the single header file from:
// https://github.com/nlohmann/json/releases
// To use Crow, you can download it from:
// https://github.com/ipkn/crow/releases

// Data file name
const std::string DATA_FILE = "users.json";

// Struct to represent a user, similar to the Python dictionary
struct User {
    int id;
    std::string name;
    std::string email;
};

// Helper function to convert a User struct to a nlohmann::json object
void to_json(nlohmann::json& j, const User& u) {
    j = nlohmann::json{{"id", u.id}, {"name", u.name}, {"email", u.email}};
}

// Helper function to convert a nlohmann::json object to a User struct
void from_json(const nlohmann::json& j, User& u) {
    j.at("id").get_to(u.id);
    j.at("name").get_to(u.name);
    j.at("email").get_to(u.email);
}

// Helper function to load users from the JSON file
std::vector<User> load_users() {
    std::vector<User> users;
    std::ifstream f(DATA_FILE);
    if (f.is_open()) {
        try {
            nlohmann::json data = nlohmann::json::parse(f);
            if (data.is_array()) {
                for (const auto& user_json : data) {
                    users.push_back(user_json.get<User>());
                }
            }
        } catch (nlohmann::json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    }
    return users;
}

// Helper function to save users to the JSON file
void save_users(const std::vector<User>& users) {
    std::ofstream f(DATA_FILE);
    if (f.is_open()) {
        nlohmann::json data = nlohmann::json::array();
        for (const auto& user : users) {
            data.push_back(user);
        }
        f << data.dump(4);
    }
}

int main() {
    crow::SimpleApp app;

    // Endpoint GET - user list
    CROW_ROUTE(app, "/users")
        .methods("GET"_method)
        ([](const crow::request& req, crow::response& res){
            std::vector<User> users = load_users();
            nlohmann::json response_json = nlohmann::json::array();
            for (const auto& user : users) {
                response_json.push_back(user);
            }
            res.set_header("Content-Type", "application/json");
            res.code = 200;
            res.write(response_json.dump());
            res.end();
        });

    // Endpoint GET - single user
    CROW_ROUTE(app, "/users/<int>")
        .methods("GET"_method)
        ([](const crow::request& req, crow::response& res, int user_id){
            std::vector<User> users = load_users();
            auto it = std::find_if(users.begin(), users.end(),
                [user_id](const User& u) { return u.id == user_id; });
            
            res.set_header("Content-Type", "application/json");
            if (it != users.end()) {
                nlohmann::json user_json = *it;
                res.code = 200;
                res.write(user_json.dump());
            } else {
                nlohmann::json error_json = {{"error", "User not found"}};
                res.code = 404;
                res.write(error_json.dump());
            }
            res.end();
        });

    // Endpoint POST - add user
    CROW_ROUTE(app, "/users")
        .methods("POST"_method)
        ([](const crow::request& req, crow::response& res){
            res.set_header("Content-Type", "application/json");
            try {
                auto body = nlohmann::json::parse(req.body);
                if (!body.contains("name") || !body.contains("email")) {
                    res.code = 400;
                    res.write(nlohmann::json({{"error", "Missing name or email"}}).dump());
                    return res.end();
                }

                std::vector<User> users = load_users();
                int new_id = 1;
                if (!users.empty()) {
                    new_id = users.back().id + 1;
                }
                
                User new_user = {new_id, body["name"], body["email"]};
                users.push_back(new_user);
                save_users(users);

                nlohmann::json new_user_json = new_user;
                res.code = 201;
                res.write(new_user_json.dump());
            } catch (const std::exception& e) {
                res.code = 400;
                res.write(nlohmann::json({{"error", "Invalid JSON"}}).dump());
            }
            res.end();
        });
    
    // Endpoint PUT - edit user
    CROW_ROUTE(app, "/users/<int>")
        .methods("PUT"_method)
        ([](const crow::request& req, crow::response& res, int user_id){
            res.set_header("Content-Type", "application/json");
            try {
                auto body = nlohmann::json::parse(req.body);
                std::vector<User> users = load_users();
                auto it = std::find_if(users.begin(), users.end(),
                    [user_id](const User& u) { return u.id == user_id; });

                if (it != users.end()) {
                    if (body.contains("name")) {
                        it->name = body["name"];
                    }
                    if (body.contains("email")) {
                        it->email = body["email"];
                    }
                    save_users(users);
                    nlohmann::json updated_user_json = *it;
                    res.code = 200;
                    res.write(updated_user_json.dump());
                } else {
                    nlohmann::json error_json = {{"error", "User not found"}};
                    res.code = 404;
                    res.write(error_json.dump());
                }
            } catch (const std::exception& e) {
                res.code = 400;
                res.write(nlohmann::json({{"error", "Invalid JSON"}}).dump());
            }
            res.end();
        });

    // Endpoint DELETE - delete user
    CROW_ROUTE(app, "/users/<int>")
        .methods("DELETE"_method)
        ([](const crow::request& req, crow::response& res, int user_id){
            std::vector<User> users = load_users();
            size_t initial_size = users.size();
            auto it = std::remove_if(users.begin(), users.end(),
                [user_id](const User& u) { return u.id == user_id; });

            res.set_header("Content-Type", "application/json");
            if (it != users.end()) {
                users.erase(it, users.end());
                save_users(users);
                nlohmann::json success_json = {{"message", "User deleted"}};
                res.code = 200;
                res.write(success_json.dump());
            } else {
                nlohmann::json error_json = {{"error", "User not found"}};
                res.code = 404;
                res.write(error_json.dump());
            }
            res.end();
        });

    app.port(18080).multithreaded().run();
}
