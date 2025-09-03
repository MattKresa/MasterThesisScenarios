#include <iostream>
#include <string>
#include <memory>
#include <pqxx/pqxx>

class DatabaseManager {
private:
    static std::unique_ptr<pqxx::connection> connectToDb() {
        try {
            std::string connection_string =
                "host=localhost port=5432 dbname=master_thesis user=postgres password=9";

            auto conn = std::make_unique<pqxx::connection>(connection_string);

            if (!conn->is_open()) {
                throw std::runtime_error("Failed to open database connection");
            }

            return conn;
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Database connection failed: " + std::string(e.what()));
        }
    }

    static void setupSchema(pqxx::connection& conn) {
        try {
            pqxx::work txn(conn);

            // Create users table
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                )
            )");

            // Create orders table
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            )");

            txn.commit();
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Schema setup failed: " + std::string(e.what()));
        }
    }

    static void insertSampleData(pqxx::connection& conn) {
        try {
            pqxx::work txn(conn);

            // Insert users
            txn.exec(R"(
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING
            )");

            // Get Alice's user ID
            pqxx::result result = txn.exec("SELECT id FROM users WHERE email = 'alice@example.com'");

            if (result.empty()) {
                txn.commit();
                return;
            }

            int userIdAlice = result[0][0].as<int>();

            // Check if Alice already has orders
            std::string countQuery = "SELECT COUNT(*) FROM orders WHERE user_id = " +
                txn.quote(userIdAlice);
            pqxx::result countResult = txn.exec(countQuery);

            long countOrders = countResult[0][0].as<long>();

            if (countOrders == 0) {
                // Insert sample orders for Alice using prepared statements
                conn.prepare("insert_order",
                    "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)");

                // Insert Laptop
                txn.exec_prepared("insert_order", userIdAlice, "Laptop", "3200.00");

                // Insert Mouse
                txn.exec_prepared("insert_order", userIdAlice, "Mouse", "120.00");

                // Insert Keyboard
                txn.exec_prepared("insert_order", userIdAlice, "Keyboard", "90.00");
            }

            txn.commit();
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Sample data insertion failed: " + std::string(e.what()));
        }
    }

    static void queryAndProcess(pqxx::connection& conn) {
        try {
            pqxx::work txn(conn);

            std::string sql = R"(
                SELECT
                    u.name AS user_name,
                    u.email,
                    o.product,
                    o.amount,
                    o.order_date
                FROM orders o
                JOIN users u ON o.user_id = u.id
                WHERE o.amount > $1
                ORDER BY o.amount DESC
            )";

            // Prepare and execute the query
            conn.prepare("query_orders", sql);
            pqxx::result result = txn.exec_prepared("query_orders", "100.00");

            std::cout << "Orders over 100 zl:\n\n";

            for (const auto& row : result) {
                std::string name = row["user_name"].as<std::string>();
                std::string email = row["email"].as<std::string>();
                std::string product = row["product"].as<std::string>();
                std::string amount = row["amount"].as<std::string>();
                std::string date = row["order_date"].as<std::string>();

                std::cout << name << " (" << email << ") ordered " << product
                    << " for " << amount << " zł on " << date << std::endl;
            }

            txn.commit();
        }
        catch (const std::exception& e) {
            throw std::runtime_error("Query execution failed: " + std::string(e.what()));
        }
    }

public:
    static void run() {
        try {
            auto conn = connectToDb();

            setupSchema(*conn);
            insertSampleData(*conn);
            queryAndProcess(*conn);

        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
};

int main() {
    DatabaseManager::run();
    return 0;
}

// Compilation instructions:
// g++ -std=c++17 -o database_manager database_manager.cpp -lpqxx -lpq
// or with CMake:
/*
cmake_minimum_required(VERSION 3.10)
project(DatabaseManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(database_manager database_manager.cpp)
target_link_libraries(database_manager ${PQXX_LIBRARIES})
target_include_directories(database_manager PRIVATE ${PQXX_INCLUDE_DIRS})
target_compile_options(database_manager PRIVATE ${PQXX_CFLAGS_OTHER})
*/