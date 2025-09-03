#include <iostream>
#include <string>
#include <pqxx/pqxx>
#include <iomanip>

using namespace std;
using namespace pqxx;

connection* connectToDb() {
    try {
        // Create connection string
        string conn_str = "dbname=master_thesis user=postgres password=9 host=localhost port=5432";

        // Create and return connection
        return new connection(conn_str);
    }
    catch (const exception& e) {
        cerr << "Error connecting to database: " << e.what() << endl;
        throw;
    }
}

void setupSchema(connection* conn) {
    try {
        work txn(*conn);

        // Create users table
        txn.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "   id SERIAL PRIMARY KEY,"
            "   name TEXT NOT NULL,"
            "   email TEXT UNIQUE NOT NULL"
            ")"
        );

        // Create orders table
        txn.exec(
            "CREATE TABLE IF NOT EXISTS orders ("
            "   id SERIAL PRIMARY KEY,"
            "   user_id INTEGER REFERENCES users(id),"
            "   product TEXT NOT NULL,"
            "   amount NUMERIC(10,2),"
            "   order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ")"
        );

        txn.commit();
    }
    catch (const exception& e) {
        cerr << "Error setting up schema: " << e.what() << endl;
        throw;
    }
}

void insertSampleData(connection* conn) {
    try {
        work txn(*conn);

        // Insert users
        txn.exec(
            "INSERT INTO users (name, email) "
            "VALUES "
            "   ('Alicja', 'alice@example.com'),"
            "   ('Bartek', 'bartek@example.com'),"
            "   ('Celina', 'celina@example.com') "
            "ON CONFLICT (email) DO NOTHING"
        );

        // Get Alice's user ID
        result r = txn.exec("SELECT id FROM users WHERE email = 'alice@example.com'");
        if (r.empty()) {
            txn.abort();
            return;
        }

        int userIdAlice = r[0][0].as<int>();

        // Check if Alice has any orders
        r = txn.exec_params("SELECT COUNT(*) FROM orders WHERE user_id = $1", userIdAlice);
        long countOrders = r[0][0].as<long>();

        if (countOrders == 0) {
            // Insert orders for Alice
            txn.exec_params(
                "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                userIdAlice, "Laptop", 3200.00
            );
            txn.exec_params(
                "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                userIdAlice, "Mouse", 120.00
            );
            txn.exec_params(
                "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
                userIdAlice, "Keyboard", 90.00
            );
        }

        txn.commit();
    }
    catch (const exception& e) {
        cerr << "Error inserting sample data: " << e.what() << endl;
        throw;
    }
}

void queryAndProcess(connection* conn) {
    try {
        work txn(*conn);

        string sql =
            "SELECT "
            "   u.name AS user_name, "
            "   u.email, "
            "   o.product, "
            "   o.amount, "
            "   o.order_date "
            "FROM orders o "
            "JOIN users u ON o.user_id = u.id "
            "WHERE o.amount > $1 "
            "ORDER BY o.amount DESC";

        result r = txn.exec_params(sql, 100.00);

        cout << "Orders over 100 zł:\n" << endl;
        for (auto row : r) {
            string name = row["user_name"].as<string>();
            string email = row["email"].as<string>();
            string product = row["product"].as<string>();
            string amount = row["amount"].as<string>();
            string date = row["order_date"].as<string>();

            cout << name << " (" << email << ") ordered " << product
                << " for " << amount << " zł on " << date << endl;
        }

        txn.commit();
    }
    catch (const exception& e) {
        cerr << "Error querying data: " << e.what() << endl;
        throw;
    }
}

int main() {
    connection* conn = nullptr;

    try {
        conn = connectToDb();

        setupSchema(conn);
        insertSampleData(conn);
        queryAndProcess(conn);

        delete conn;
        return 0;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        if (conn) delete conn;
        return 1;
    }
}