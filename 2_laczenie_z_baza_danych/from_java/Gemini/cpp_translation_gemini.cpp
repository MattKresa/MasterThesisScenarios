#include <iostream>
#include <string>
#include <pqxx/pqxx> // Include the libpqxx header
#include <iomanip>   // For std::fixed, std::setprecision
#include <ctime>     // For std::put_time

// A simple class to manage database operations
class DatabaseManager {
public:
    /**
     * @brief Establishes a connection to the PostgreSQL database.
     * @return A unique pointer to a pqxx::connection object.
     * @throws pqxx::sql_error if a database error occurs during connection.
     */
    static std::unique_ptr<pqxx::connection> connectToDb() {
        std::string conn_info = "dbname=master_thesis user=postgres password=9 host=localhost port=5432";
        try {
            // pqxx::connection constructor attempts to connect
            auto conn = std::make_unique<pqxx::connection>(conn_info);
            std::cout << "Database connection established successfully." << std::endl;
            return conn;
        }
        catch (const pqxx::sql_error& e) {
            std::cerr << "SQL Error connecting to database: " << e.what() << std::endl;
            throw; // Re-throw to be caught in main
        }
        catch (const std::exception& e) {
            std::cerr << "Error connecting to database: " << e.what() << std::endl;
            throw; // Re-throw to be caught in main
        }
    }

    /**
     * @brief Sets up the 'users' and 'orders' tables in the database if they do not exist.
     * @param conn A reference to the pqxx::connection object.
     * @throws pqxx::sql_error if a SQL execution error occurs.
     */
    static void setupSchema(pqxx::connection& conn) {
        try {
            // A 'work' object represents a transaction. It commits on destruction if no error.
            pqxx::work W(conn);

            // Create users table
            W.exec("CREATE TABLE IF NOT EXISTS users ("
                "id SERIAL PRIMARY KEY,"
                "name TEXT NOT NULL,"
                "email TEXT UNIQUE NOT NULL"
                ")");

            // Create orders table
            W.exec("CREATE TABLE IF NOT EXISTS orders ("
                "id SERIAL PRIMARY KEY,"
                "user_id INTEGER REFERENCES users(id),"
                "product TEXT NOT NULL,"
                "amount NUMERIC(10,2),"
                "order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                ")");

            W.commit(); // Explicitly commit the transaction
            std::cout << "Database schema setup complete." << std::endl;
        }
        catch (const pqxx::sql_error& e) {
            std::cerr << "SQL Error setting up schema: " << e.what() << std::endl;
            throw;
        }
    }

    /**
     * @brief Inserts sample user and order data into the database.
     * It checks for existing users and orders to prevent duplicate insertions.
     * @param conn A reference to the pqxx::connection object.
     * @throws pqxx::sql_error if a SQL execution error occurs.
     */
    static void insertSampleData(pqxx::connection& conn) {
        try {
            pqxx::work W(conn);

            // Insert users
            // ON CONFLICT (email) DO NOTHING handles existing users
            W.exec("INSERT INTO users (name, email) VALUES "
                "('Alicja', 'alice@example.com'),"
                "('Bartek', 'bartek@example.com'),"
                "('Celina', 'celina@example.com') "
                "ON CONFLICT (email) DO NOTHING");
            std::cout << "Sample users inserted (or already exist)." << std::endl;

            // Get userId for 'alice@example.com' using prepared statement
            int userIdAlice = -1;
            // Prepare a statement for reusability (optional for single use, but good practice)
            conn.prepare("get_alice_id", "SELECT id FROM users WHERE email = $1");
            pqxx::result r_alice = W.exec_prepared("get_alice_id", "alice@example.com");

            if (!r_alice.empty()) {
                userIdAlice = r_alice[0][0].as<int>();
            }
            else {
                std::cout << "Alice not found in users table, skipping order insertion." << std::endl;
                W.abort(); // Abort the current transaction if Alice is not found
                return;
            }

            // Check if orders for Alice already exist
            long long countOrders = 0;
            conn.prepare("count_alice_orders", "SELECT COUNT(*) FROM orders WHERE user_id = $1");
            pqxx::result r_count = W.exec_prepared("count_alice_orders", userIdAlice);

            if (!r_count.empty()) {
                countOrders = r_count[0][0].as<long long>();
            }

            if (countOrders == 0) {
                // Insert orders for Alice using a prepared statement
                conn.prepare("insert_order", "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)");

                W.exec_prepared("insert_order", userIdAlice, "Laptop", "3200.00");
                W.exec_prepared("insert_order", userIdAlice, "Mouse", "120.00");
                W.exec_prepared("insert_order", userIdAlice, "Keyboard", "90.00");

                std::cout << "Sample orders for Alice inserted." << std::endl;
            }
            else {
                std::cout << "Orders for Alice already exist, skipping insertion." << std::endl;
            }

            W.commit(); // Commit the transaction
        }
        catch (const pqxx::sql_error& e) {
            std::cerr << "SQL Error inserting sample data: " << e.what() << std::endl;
            throw;
        }
    }

    /**
     * @brief Queries the database for orders with an amount greater than a specified value
     * and prints the results.
     * @param conn A reference to the pqxx::connection object.
     * @throws pqxx::sql_error if a SQL execution error occurs.
     */
    static void queryAndProcess(pqxx::connection& conn) {
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

        try {
            pqxx::nontransaction N(conn); // Use nontransaction for simple queries without updates

            // Prepare the query
            conn.prepare("query_orders", sql);

            // Execute the prepared query with the amount parameter
            pqxx::result R = N.exec_prepared("query_orders", "100.00");

            std::cout << "\nOrders over 100 zł:\n" << std::endl;
            for (const auto& row : R) {
                std::string name = row["user_name"].as<std::string>();
                std::string email = row["email"].as<std::string>();
                std::string product = row["product"].as<std::string>();
                // Retrieve amount as string to preserve full precision, then print
                std::string amount_str = row["amount"].as<std::string>();
                // Retrieve timestamp and format it
                // pqxx::result::operator[] returns a pqxx::field.
                // .as<std::string>() retrieves the value as a string.
                // We then parse it into a std::tm struct for formatting.
                std::string order_date_str = row["order_date"].as<std::string>();
                std::tm tm_date = {};
                std::istringstream ss(order_date_str);
                // Parse the timestamp string. The format depends on how PostgreSQL returns it.
                // Common format: "YYYY-MM-DD HH:MM:SS.ffffff"
                // We'll try to parse up to seconds for simplicity, or use the full string.
                // For direct printing, just use the string.
                // If precise formatting is needed, parse it.
                // For simplicity, let's just use the string from the database for now,
                // or use a more robust date/time library if available (e.g., C++20 chrono).
                // For older C++, direct string output is easiest.
                // Example of parsing if needed: ss >> std::get_time(&tm_date, "%Y-%m-%d %H:%M:%S");

                std::cout << name << " (" << email << ") ordered "
                    << product << " for " << amount_str << " zł on "
                    << order_date_str << std::endl;
            }
        }
        catch (const pqxx::sql_error& e) {
            std::cerr << "SQL Error querying and processing data: " << e.what() << std::endl;
            throw;
        }
    }
};

int main() {
    // Unique pointer to manage the connection's lifetime
    std::unique_ptr<pqxx::connection> conn_ptr = nullptr;
    try {
        // Connect to the database
        conn_ptr = DatabaseManager::connectToDb();
        // C++ unique_ptr automatically handles closing connection on exit from scope/exception

        // Setup schema
        DatabaseManager::setupSchema(*conn_ptr);
        // Insert sample data
        DatabaseManager::insertSampleData(*conn_ptr);
        // Query and process data
        DatabaseManager::queryAndProcess(*conn_ptr);

    }
    catch (const pqxx::sql_error& e) {
        std::cerr << "A database error occurred: " << e.what() << std::endl;
        return 1; // Indicate error
    }
    catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1; // Indicate error
    }

    return 0; // Indicate success
}
