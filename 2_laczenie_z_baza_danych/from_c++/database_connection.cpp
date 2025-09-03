#include <pqxx/pqxx>
#include <iostream>
#include <string>

using namespace std;
using namespace pqxx;

void setup_schema(work& tx) {
    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL
        );
    )");

    tx.exec(R"(
        CREATE TABLE IF NOT EXISTS orders (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id),
            product TEXT NOT NULL,
            amount NUMERIC(10,2),
            order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )");
}

void insert_sample_data(work& tx) {
    // insert users
    tx.exec(R"(
        INSERT INTO users (name, email)
        VALUES 
            ('Alicja', 'alice@example.com'),
            ('Bartek', 'bartek@example.com'),
            ('Celina', 'celina@example.com')
        ON CONFLICT (email) DO NOTHING;
    )");

    // take Alicja's id
    result r = tx.exec("SELECT id FROM users WHERE email = 'alice@example.com'");
    if (r.empty()) return;

    int user_id_alice = r[0]["id"].as<int>();

    // check if there are orders
    result cnt = tx.exec_params("SELECT COUNT(*) FROM orders WHERE user_id = $1", user_id_alice);
    if (cnt[0]["count"].as<long>() == 0) {
        tx.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            user_id_alice, "Laptop", 3200.00
        );
        tx.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            user_id_alice, "Mouse", 120.00
        );
        tx.exec_params(
            "INSERT INTO orders (user_id, product, amount) VALUES ($1, $2, $3)",
            user_id_alice, "Keyboard", 90.00
        );
    }
}

void query_and_process(work& tx) {
    result r = tx.exec_params(R"(
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

    cout << "Orders over 100 zl:\n\n";
    for (auto row : r) {
        cout << row["user_name"].c_str() << " (" << row["email"].c_str() << ") ordered "
            << row["product"].c_str() << " for " << row["amount"].c_str()
            << " zł on " << row["order_date"].c_str() << "\n";
    }
}

int main() {
    try {
        connection conn("host=localhost port=5432 dbname=master_thesis user=postgres password=9");

        if (!conn.is_open()) {
            cerr << "Cannot connect to database!\n";
            return 1;
        }

        work tx(conn);
        setup_schema(tx);
        insert_sample_data(tx);
        tx.commit(); // commit, setup + insert

        work tx2(conn); // new tran for SELECT
        query_and_process(tx2);
        tx2.commit();

    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
