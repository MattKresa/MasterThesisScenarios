import psycopg2
from psycopg2.extras import RealDictCursor

def connect_to_db():
    return psycopg2.connect(
        host="localhost",
        port=5432,
        database="master_thesis",
        user="postgres",
        password="9"
    )


def setup_schema(cursor):
    # Create two tables
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL
        );
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS orders (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id),
            product TEXT NOT NULL,
            amount NUMERIC(10,2),
            order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    """)


def insert_sample_data(cursor):
    # Add users
    cursor.execute("""
        INSERT INTO users (name, email)
        VALUES 
            ('Alicja', 'alice@example.com'),
            ('Bartek', 'bartek@example.com'),
            ('Celina', 'celina@example.com')
        ON CONFLICT (email) DO NOTHING;
    """)

    # Add orders
    cursor.execute("SELECT id FROM users WHERE email = 'alice@example.com'")
    user_id_alice = cursor.fetchone()['id']

    cursor.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
    if cursor.fetchone()['count'] == 0:
        cursor.executemany("""
            INSERT INTO orders (user_id, product, amount)
            VALUES (%s, %s, %s)
        """, [
            (user_id_alice, "Laptop", 3200.00),
            (user_id_alice, "Mouse", 120.00),
            (user_id_alice, "Keyboard", 90.00),
        ])


def query_and_process(cursor):
    # Get orders over 100zl
    cursor.execute("""
        SELECT
            u.name AS user_name,
            u.email,
            o.product,
            o.amount,
            o.order_date
        FROM orders o
        JOIN users u ON o.user_id = u.id
        WHERE o.amount > %s
        ORDER BY o.amount DESC;
    """, (100.0,))

    results = cursor.fetchall()

    print("Orders over 100 zl:\n")
    for row in results:
        print(
            f"{row['user_name']} ({row['email']}) ordered {row['product']} for {row['amount']} zł on {row['order_date']}")


def main():
    try:
        conn = connect_to_db()
        conn.autocommit = True

        with conn.cursor(cursor_factory=RealDictCursor) as cursor:
            setup_schema(cursor)
            insert_sample_data(cursor)
            query_and_process(cursor)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if conn:
            conn.close()


if __name__ == "__main__":
    main()