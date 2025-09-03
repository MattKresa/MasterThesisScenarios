import psycopg2
from psycopg2.extras import RealDictCursor


def setup_schema(cur):
    cur.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id SERIAL PRIMARY KEY,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL
        );
    """)

    cur.execute("""
        CREATE TABLE IF NOT EXISTS orders (
            id SERIAL PRIMARY KEY,
            user_id INTEGER REFERENCES users(id),
            product TEXT NOT NULL,
            amount NUMERIC(10,2),
            order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    """)


def insert_sample_data(cur):
    # Insert users
    cur.execute("""
        INSERT INTO users (name, email)
        VALUES 
            ('Alicja', 'alice@example.com'),
            ('Bartek', 'bartek@example.com'),
            ('Celina', 'celina@example.com')
        ON CONFLICT (email) DO NOTHING;
    """)

    # Take Alicja's id
    cur.execute("SELECT id FROM users WHERE email = %s", ('alice@example.com',))
    row = cur.fetchone()
    if not row:
        return

    user_id_alice = row[0]

    # Check if orders already exist
    cur.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
    cnt = cur.fetchone()[0]

    if cnt == 0:
        cur.execute(
            "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
            (user_id_alice, "Laptop", 3200.00)
        )
        cur.execute(
            "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
            (user_id_alice, "Mouse", 120.00)
        )
        cur.execute(
            "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
            (user_id_alice, "Keyboard", 90.00)
        )


def query_and_process(cur):
    cur.execute("""
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

    print("Orders over 100 zł:\n")
    for row in cur.fetchall():
        user_name, email, product, amount, order_date = row
        print(f"{user_name} ({email}) ordered {product} for {amount} zł on {order_date}")


def main():
    try:
        conn = psycopg2.connect(
            host="localhost",
            port=5432,
            dbname="master_thesis",
            user="postgres",
            password="9"
        )

        with conn:
            with conn.cursor() as cur:
                setup_schema(cur)
                insert_sample_data(cur)

        with conn:
            with conn.cursor() as cur:
                query_and_process(cur)

    except Exception as e:
        print(f"Error: {e}")


if __name__ == "__main__":
    main()
