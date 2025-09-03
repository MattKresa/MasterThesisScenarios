#include <iostream> // For input/output operations (e.g., std::cout, std::cerr)
#include <string>   // For using std::string
#include <vector>   // For using std::vector (though not strictly needed in this version)
#include <pqxx/pqxx> // The main header for libpqxx, providing database connectivity and operations
#include <iomanip>  // For std::put_time to format timestamps

// Function to establish a connection to the PostgreSQL database.
// It returns a pqxx::connection object.
pqxx::connection connect_to_db() {
    // Connection string details: host, port, database name, user, and password.
    // Ensure these match your PostgreSQL setup.
    std::string conn_str = "host=localhost port=5432 dbname=master_thesis user=postgres password=9";
    
    // Attempt to create a connection object.
    // libpqxx will throw an exception if the connection fails.
    return pqxx::connection(conn_str);
}

// Function to set up the database schema (create tables if they don't exist).
// It takes a pqxx::connection object by reference.
void setup_schema(pqxx::connection& conn) {
    // pqxx::work represents a transaction. All operations within a work object
    // are atomic. If no error occurs, the transaction is committed when 'W' goes
    // out of scope or W.commit() is called.
    pqxx::work W(conn); // Start a transaction

    // SQL to create the 'users' table.
    // SERIAL PRIMARY KEY: auto-increments and serves as primary key.
    // TEXT NOT NULL: string type, cannot be null.
    // TEXT UNIQUE NOT NULL: string type, must be unique, cannot be null.
    W.exec0("CREATE TABLE IF NOT EXISTS users ("
            "id SERIAL PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "email TEXT UNIQUE NOT NULL"
            ");");

    // SQL to create the 'orders' table.
    // INTEGER REFERENCES users(id): foreign key constraint linking to the 'users' table.
    // NUMERIC(10,2): fixed-point number with 10 total digits and 2 digits after the decimal.
    // TIMESTAMP DEFAULT CURRENT_TIMESTAMP: stores date and time, defaults to current time on insert.
    W.exec0("CREATE TABLE IF NOT EXISTS orders ("
            "id SERIAL PRIMARY KEY,"
            "user_id INTEGER REFERENCES users(id),"
            "product TEXT NOT NULL,"
            "amount NUMERIC(10,2),"
            "order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");");

    // Commit the transaction. If this line is reached, the DDL statements were successful.
    W.commit();
    std::cout << "Schema setup complete." << std::endl;
}

// Function to insert sample data into the 'users' and 'orders' tables.
// It takes a pqxx::connection object by reference.
void insert_sample_data(pqxx::connection& conn) {
    pqxx::work W(conn); // Start a new transaction for data insertion

    // Insert sample users.
    // ON CONFLICT (email) DO NOTHING: If a user with the same email already exists,
    // do not insert and do not throw an error.
    W.exec0("INSERT INTO users (name, email) "
            "VALUES "
            "('Alicja', 'alice@example.com'),"
            "('Bartek', 'bartek@example.com'),"
            "('Celina', 'celina@example.com') "
            "ON CONFLICT (email) DO NOTHING;");

    // Fetch the user_id for 'alice@example.com'.
    // pqxx::result is a collection of rows returned by a query.
    pqxx::result r_alice = W.exec("SELECT id FROM users WHERE email = 'alice@example.com'");
    
    // Check if any row was returned.
    if (r_alice.empty()) {
        std::cerr << "Error: Alice's user ID not found. Cannot insert orders." << std::endl;
        W.abort(); // Abort the transaction if Alice's ID isn't found
        return;
    }
    
    // Get the 'id' column from the first (and only) row.
    // .as<int>() converts the field value to an integer.
    int user_id_alice = r_alice[0]["id"].as<int>();

    // Check if orders for Alice already exist to prevent duplicate insertions on re-run.
    pqxx::result r_count = W.exec_params("SELECT COUNT(*) FROM orders WHERE user_id = $1", user_id_alice);
    
    // Get the count from the first row, 'count' column.
    if (r_count[0]["count"].as<long>() == 0) {
        // Insert orders for Alice.
        // Use exec_params for parameterized queries to prevent SQL injection.
        // $1, $2, $3 are placeholders for parameters.
        W.exec_params("INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                      user_id_alice, "Laptop", 3200.00);
        W.exec_params("INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                      user_id_alice, "Mouse", 120.00);
        W.exec_params("INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                      user_id_alice, "Keyboard", 90.00); // This order will not be in the final query result (<100)
        std::cout << "Sample orders inserted for Alice." << std::endl;
    } else {
        std::cout << "Orders for Alice already exist, skipping insertion." << std::endl;
    }

    W.commit(); // Commit the transaction
    std::cout << "Sample data insertion complete." << std::endl;
}

// Function to query and process data from the database.
// It takes a pqxx::connection object by reference.
void query_and_process(pqxx::connection& conn) {
    pqxx::work W(conn); // Start a new transaction for querying

    // SQL query to get orders over a certain amount, joining 'orders' and 'users' tables.
    // $1 is a placeholder for the amount parameter.
    pqxx::result results = W.exec_params(
        "SELECT "
        "u.name AS user_name,"
        "u.email,"
        "o.product,"
        "o.amount,"
        "o.order_date "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.amount > $1 "
        "ORDER BY o.amount DESC;",
        100.0 // The value for $1
    );

    std::cout << "\nOrders over 100 zl:\n" << std::endl;

    // Iterate through the rows returned by the query.
    for (const auto& row : results) {
        // Access columns by name using row["column_name"].
        // .as<std::string>() converts the field value to a string.
        // .as<double>() converts the field value to a double.
        std::string user_name = row["user_name"].as<std::string>();
        std::string email = row["email"].as<std::string>();
        std::string product = row["product"].as<std::string>();
        double amount = row["amount"].as<double>();
        
        // For timestamp, it's often easiest to get it as a string directly from the DB.
        // PostgreSQL's default timestamp format is usually 'YYYY-MM-DD HH:MM:SS.microseconds'.
        std::string order_date_str = row["order_date"].as<std::string>();

        // Print the formatted output.
        std::cout << user_name << " (" << email << ") ordered "
                  << product << " for " << std::fixed << std::setprecision(2) << amount
                  << " zł on " << order_date_str << std::endl;
    }

    W.commit(); // Commit the transaction (read-only transactions can also be committed)
}

// Main function where the program execution begins.
int main() {
    pqxx::connection conn; // Declare connection object outside try-catch to ensure its scope
    try {
        // Connect to the database.
        conn = connect_to_db();
        std::cout << "Successfully connected to the database." << std::endl;

        // Call functions to perform database operations.
        setup_schema(conn);
        insert_sample_data(conn);
        query_and_process(conn);

    } catch (const pqxx::sql_error& e) {
        // Catch specific SQL errors from libpqxx.
        std::cerr << "SQL Error: " << e.what() << std::endl;
        std::cerr << "Query: " << e.query() << std::endl;
    } catch (const pqxx::usage_error& e) {
        // Catch usage errors (e.g., programming errors with libpqxx).
        std::cerr << "Libpqxx Usage Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Catch any other standard exceptions.
        std::cerr << "General Error: " << e.what() << std::endl;
    } catch (...) {
        // Catch any unknown exceptions.
        std::cerr << "An unknown error occurred." << std::endl;
    }

    // The connection object's destructor will automatically close the connection
    // when it goes out of scope, even if an exception occurred.
    // So, an explicit conn.close() is not strictly necessary here, but it's good
    // practice to be aware of the connection's lifecycle.
    if (conn.is_open()) {
        std::cout << "Closing database connection." << std::endl;
        // conn.disconnect(); // Explicitly disconnect if desired, otherwise destructor handles it.
    }

    return 0; // Indicate successful program execution
}
