#include <iostream>
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

connection connectToDb() {
    string conn_str = "host=localhost port=5432 dbname=master_thesis user=postgres password=9";
    return connection(conn_str);
}

void setupSchema(connection& conn) {
    work txn(conn);

    txn.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL
        )
    )");

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

void insertSampleData(connection& conn) {
    {
        work txn(conn);
        txn.exec(R"(
            INSERT INTO users (name, email)
            VALUES 
                ('Alicja', 'alice@example.com'),
                ('Bartek', 'bartek@example.com'),
                ('Celina', 'celina@example.com')
            ON CONFLICT (email) DO NOTHING
        )");
        txn.commit();
    }

    int userIdAlice = -1;
    {
        work txn(conn);
        result r = txn.exec("SELECT id FROM users WHERE email = 'alice@example.com'");
        if (!r.empty()) {
            userIdAlice = r[0][0].as<int>();
        }
    }

    if (userIdAlice == -1) return;

    long countOrders = 0;
    {
        work txn(conn);
        result r = txn.exec_params("SELECT COUNT(*) FROM orders WHERE user_id = $1", userIdAlice);
        if (!r.empty()) {
            countOrders = r[0][0].as<long>();
        }
    }

    if (countOrders == 0) {
        work txn(conn);
        txn.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            userIdAlice, "Laptop", "3200.00"
        );
        txn.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            userIdAlice, "Mouse", "120.00"
        );
        txn.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            userIdAlice, "Keyboard", "90.00"
        );
        txn.commit();
    }
}

void queryAndProcess(connection& conn) {
    string sql = R"(
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

    work txn(conn);
    result r = txn.exec_params(sql, "100.00");

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
}

int main() {
    try {
        connection conn = connectToDb();

        if (!conn.is_open()) {
            cerr << "Error: could not connect to database." << endl;
            return 1;
        }

        setupSchema(conn);
        insertSampleData(conn);
        queryAndProcess(conn);

    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
