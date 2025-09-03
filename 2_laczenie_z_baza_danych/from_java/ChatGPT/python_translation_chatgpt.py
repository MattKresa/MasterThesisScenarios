import psycopg2
from decimal import Decimal

def connect_to_db():
    return psycopg2.connect(
        host="localhost",
        port=5432,
        dbname="master_thesis",
        user="postgres",
        password="9"
    )

def setup_schema(conn):
    with conn.cursor() as cur:
        cur.execute("""
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                name TEXT NOT NULL,
                email TEXT UNIQUE NOT NULL
            )
        """)

        cur.execute("""
            CREATE TABLE IF NOT EXISTS orders (
                id SERIAL PRIMARY KEY,
                user_id INTEGER REFERENCES users(id),
                product TEXT NOT NULL,
                amount NUMERIC(10,2),
                order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        """)
    conn.commit()

def insert_sample_data(conn):
    with conn.cursor() as cur:
        cur.execute("""
            INSERT INTO users (name, email)
            VALUES 
                ('Alicja', 'alice@example.com'),
                ('Bartek', 'bartek@example.com'),
                ('Celina', 'celina@example.com')
            ON CONFLICT (email) DO NOTHING
        """)

    conn.commit()

    # Get Alice's ID
    with conn.cursor() as cur:
        cur.execute("SELECT id FROM users WHERE email = 'alice@example.com'")
        row = cur.fetchone()
        if row is None:
            return
        user_id_alice = row[0]

    # Check if Alice has orders
    with conn.cursor() as cur:
        cur.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
        count_orders = cur.fetchone()[0]

    # Insert orders if none exist
    if count_orders == 0:
        with conn.cursor() as cur:
            cur.execute(
                "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                (user_id_alice, "Laptop", Decimal("3200.00"))
            )
            cur.execute(
                "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                (user_id_alice, "Mouse", Decimal("120.00"))
            )
            cur.execute(
                "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                (user_id_alice, "Keyboard", Decimal("90.00"))
            )
        conn.commit()

def query_and_process(conn):
    sql = """
        SELECT
            u.name AS user_name,
            u.email,
            o.product,
            o.amount,
            o.order_date
        FROM orders o
        JOIN users u ON o.user_id = u.id
        WHERE o.amount > %s
        ORDER BY o.amount DESC
    """
    with conn.cursor() as cur:
        cur.execute(sql, (Decimal("100.00"),))
        print("Orders over 100 zł:\n")
        for row in cur.fetchall():
            name, email, product, amount, date = row
            print(f"{name} ({email}) ordered {product} for {amount} zł on {date}")

def main():
    try:
        with connect_to_db() as conn:
            conn.autocommit = True
            setup_schema(conn)
            insert_sample_data(conn)
            query_and_process(conn)
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
