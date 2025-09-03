#include <iostream>
#include <libpq-fe.h>
#include <string>
#include <vector>

struct Order {
    std::string user_name;
    std::string email;
    std::string product;
    double amount;
    std::string order_date;
};

PGconn* connect_to_db() {
    PGconn* conn = PQsetdbLogin(
        "localhost",    // host
        "5432",         // port
        nullptr,        // options
        nullptr,        // tty
        "master_thesis",// dbname
        "postgres",     // user
        "9"             // password
    );

    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << "\n";
        PQfinish(conn);
        return nullptr;
    }
    return conn;
}

void exec_no_result(PGconn* conn, const std::string& sql) {
    PGresult* res = PQexec(conn, sql.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        std::cerr << "SQL execution failed: " << PQerrorMessage(conn) << "\n";
        PQclear(res);
        PQfinish(conn);
        exit(1);
    }
    PQclear(res);
}

void setup_schema(PGconn* conn) {
    exec_no_result(conn,
        "CREATE TABLE IF NOT EXISTS users ("
        "id SERIAL PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "email TEXT UNIQUE NOT NULL);"
    );

    exec_no_result(conn,
        "CREATE TABLE IF NOT EXISTS orders ("
        "id SERIAL PRIMARY KEY,"
        "user_id INTEGER REFERENCES users(id),"
        "product TEXT NOT NULL,"
        "amount NUMERIC(10,2),"
        "order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP);"
    );
}

void insert_sample_data(PGconn* conn) {
    exec_no_result(conn,
        "INSERT INTO users (name, email) VALUES "
        "('Alicja', 'alice@example.com'),"
        "('Bartek', 'bartek@example.com'),"
        "('Celina', 'celina@example.com') "
        "ON CONFLICT (email) DO NOTHING;"
    );

    // Get Alice's user_id
    PGresult* res = PQexec(conn, "SELECT id FROM users WHERE email = 'alice@example.com'");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Failed to get user_id for Alice\n";
        PQclear(res);
        return;
    }
    int user_id_alice = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    // Check if Alice already has orders
    std::string count_query = "SELECT COUNT(*) FROM orders WHERE user_id = " + std::to_string(user_id_alice);
    res = PQexec(conn, count_query.c_str());
    int count = std::stoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    if (count == 0) {
        std::vector<std::string> inserts = {
            "INSERT INTO orders (user_id, product, amount) VALUES (" + std::to_string(user_id_alice) + ", 'Laptop', 3200.00)",
            "INSERT INTO orders (user_id, product, amount) VALUES (" + std::to_string(user_id_alice) + ", 'Mouse', 120.00)",
            "INSERT INTO orders (user_id, product, amount) VALUES (" + std::to_string(user_id_alice) + ", 'Keyboard', 90.00)"
        };
        for (const auto& q : inserts) {
            exec_no_result(conn, q);
        }
    }
}

void query_and_process(PGconn* conn) {
    std::string query =
        "SELECT u.name AS user_name, u.email, o.product, o.amount, o.order_date "
        "FROM orders o "
        "JOIN users u ON o.user_id = u.id "
        "WHERE o.amount > 100.0 "
        "ORDER BY o.amount DESC;";

    PGresult* res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Query failed: " << PQerrorMessage(conn) << "\n";
        PQclear(res);
        return;
    }

    int rows = PQntuples(res);
    int cols = PQnfields(res);

    std::cout << "Orders over 100 zl:\n\n";
    for (int i = 0; i < rows; ++i) {
        std::string user_name = PQgetvalue(res, i, 0);
        std::string email = PQgetvalue(res, i, 1);
        std::string product = PQgetvalue(res, i, 2);
        std::string amount = PQgetvalue(res, i, 3);
        std::string order_date = PQgetvalue(res, i, 4);

        std::cout << user_name << " (" << email << ") ordered "
            << product << " for " << amount << " zł on "
            << order_date << "\n";
    }

    PQclear(res);
}

int main() {
    PGconn* conn = connect_to_db();
    if (!conn) return 1;

    setup_schema(conn);
    insert_sample_data(conn);
    query_and_process(conn);

    PQfinish(conn);
    return 0;
}
