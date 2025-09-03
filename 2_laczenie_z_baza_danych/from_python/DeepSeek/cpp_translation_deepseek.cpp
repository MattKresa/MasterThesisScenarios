#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <memory>
#include <stdexcept>
#include <pqxx/pqxx>

using namespace pqxx;

void setup_schema(connection& conn) {
    work txn(conn);

    txn.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "    id SERIAL PRIMARY KEY,"
        "    name TEXT NOT NULL,"
        "    email TEXT UNIQUE NOT NULL"
        ");"
    );

    txn.exec(
        "CREATE TABLE IF NOT EXISTS orders ("
        "    id SERIAL PRIMARY KEY,"
        "    user_id INTEGER REFERENCES users(id),"
        "    product TEXT NOT NULL,"
        "    amount NUMERIC(10,2),"
        "    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");"
    );

    txn.commit();
}

void insert_sample_data(connection& conn) {
    work txn(conn);

    // Add users
    txn.exec(
        "INSERT INTO users (name, email) "
        "VALUES "
        "    ('Alicja', 'alice@example.com'),"
        "    ('Bartek', 'bartek@example.com'),"
        "    ('Celina', 'celina@example.com') "
        "ON CONFLICT (email) DO NOTHING;"
    );

    // Get Alice's ID
    result user_res = txn.exec("SELECT id FROM users WHERE email = 'alice@example.com'");
    if (user_res.empty()) {
        throw std::runtime_error("Alice not found in database");
    }
    int user_id_alice = user_res[0][0].as<int>();

    // Check if Alice has orders
    result count_res = txn.exec_params(
        "SELECT COUNT(*) FROM orders WHERE user_id = $1",
        user_id_alice
    );
    int order_count = count_res[0][0].as<int>();

    if (order_count == 0) {
        // Insert multiple orders
        const std::vector<std::tuple<int, std::string, double>> orders = {
            {user_id_alice, "Laptop", 3200.00},
            {user_id_alice, "Mouse", 120.00},
            {user_id_alice, "Keyboard", 90.00}
        };

        for (const auto& order : orders) {
            txn.exec_params(
                "INSERT INTO orders (user_id, product, amount) "
                "VALUES ($1, $2, $3)",
                std::get<0>(order),
                std::get<1>(order),
                std::get<2>(order)
            );
        }
    }

    txn.commit();
}

void query_and_process(connection& conn) {
    work txn(conn);

    result res = txn.exec_params(
        "SELECT "
        "    u.name AS user_name, "
        "    u.email, "
        "    o.product, "
        "    o.amount, "
        "    o.order_date "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.amount > $1 "
        "ORDER BY o.amount DESC",
        100.0
    );

    std::cout << "Orders over 100 zl:\n\n";
    for (const auto& row : res) {
        std::cout << row["user_name"].as<std::string>() << " ("
            << row["email"].as<std::string>() << ") ordered "
            << row["product"].as<std::string>() << " for "
            << row["amount"].as<double>() << " zł on "
            << row["order_date"].as<std::string>() << "\n";
    }
}

int main() {
    try {
        connection conn(
            "host=localhost "
            "port=5432 "
            "dbname=master_thesis "
            "user=postgres "
            "password=9"
        );

        conn.set_client_encoding("UTF8");

        setup_schema(conn);
        insert_sample_data(conn);
        query_and_process(conn);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}