#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <pqxx/pqxx>

class DatabaseManager {
private:
    std::unique_ptr<pqxx::connection> conn;

public:
    // Connect to database
    bool connect_to_db() {
        try {
            std::string connection_string =
                "host=localhost "
                "port=5432 "
                "dbname=master_thesis "
                "user=postgres "
                "password=9";

            conn = std::make_unique<pqxx::connection>(connection_string);

            if (!conn->is_open()) {
                std::cerr << "Failed to open database connection" << std::endl;
                return false;
            }

            return true;
        }
        catch (const std::exception& e) {
            std::cerr << "Connection error: " << e.what() << std::endl;
            return false;
        }
    }

    // Setup database schema
    void setup_schema() {
        try {
            pqxx::work txn(*conn);

            // Create users table
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                );
            )");

            // Create orders table
            txn.exec(R"(
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                );
            )");

            txn.commit();
        }
        catch (const std::exception& e) {
            std::cerr << "Schema setup error: " << e.what() << std::endl;
            throw;
        }
    }

    // Insert sample data
    void insert_sample_data() {
        try {
            pqxx::work txn(*conn);

            // Add users
            txn.exec(R"(
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
            )");

            // Get Alice's user ID
            pqxx::result user_result = txn.exec(
                "SELECT id FROM users WHERE email = 'alice@example.com'"
            );

            if (user_result.empty()) {
                std::cerr << "Alice not found in database" << std::endl;
                return;
            }

            int user_id_alice = user_result[0][0].as<int>();

            // Check if Alice already has orders
            pqxx::result count_result = txn.exec_params(
                "SELECT COUNT(*) FROM orders WHERE user_id = $1",
                user_id_alice
            );

            int order_count = count_result[0][0].as<int>();

            if (order_count == 0) {
                // Insert orders for Alice
                std::vector<std::tuple<int, std::string, double>> orders = {
                    {user_id_alice, "Laptop", 3200.00},
                    {user_id_alice, "Mouse", 120.00},
                    {user_id_alice, "Keyboard", 90.00}
                };

                for (const auto& order : orders) {
                    txn.exec_params(
                        "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                        std::get<0>(order),
                        std::get<1>(order),
                        std::get<2>(order)
                    );
                }
            }

            txn.commit();
        }
        catch (const std::exception& e) {
            std::cerr << "Data insertion error: " << e.what() << std::endl;
            throw;
        }
    }

    // Query and process results
    void query_and_process() {
        try {
            pqxx::work txn(*conn);

            pqxx::result results = txn.exec_params(R"(
                SELECT
                    u.name AS user_name,
                    u.email,
                    o.product,
                    o.amount,
                    o.order_date
                FROM orders o
                JOIN users u ON o.user_id = u.id
                WHERE o.amount > $1
                ORDER BY o.amount DESC;
            )", 100.0);

            std::cout << "Orders over 100 zl:\n" << std::endl;

            for (const auto& row : results) {
                std::string user_name = row["user_name"].as<std::string>();
                std::string email = row["email"].as<std::string>();
                std::string product = row["product"].as<std::string>();
                double amount = row["amount"].as<double>();
                std::string order_date = row["order_date"].as<std::string>();

                std::cout << std::fixed << std::setprecision(2)
                    << user_name << " (" << email << ") ordered "
                    << product << " for " << amount << " zł on "
                    << order_date << std::endl;
            }

            txn.commit();
        }
        catch (const std::exception& e) {
            std::cerr << "Query error: " << e.what() << std::endl;
            throw;
        }
    }

    // Close connection
    void close() {
        if (conn && conn->is_open()) {
            conn->close();
        }
    }
};

int main() {
    DatabaseManager db;

    try {
        if (!db.connect_to_db()) {
            return 1;
        }

        db.setup_schema();
        db.insert_sample_data();
        db.query_and_process();

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    db.close();
    return 0;
}

// Compilation instructions:
// Install libpqxx: sudo apt-get install libpqxx-dev (Ubuntu/Debian)
// Compile: g++ -std=c++14 -o postgres_app main.cpp -lpqxx -lpq