import psycopg2
from psycopg2 import sql, Error


def setup_schema(conn):
    try:
        with conn.cursor() as cur:
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
            conn.commit()
    except Error as e:
        print(f"Error setting up schema: {e}")
        conn.rollback()


def insert_sample_data(conn):
    try:
        with conn.cursor() as cur:
            # Insert users
            cur.execute("""
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING;
            """)

            # Get Alicja's ID
            cur.execute("SELECT id FROM users WHERE email = 'alice@example.com'")
            result = cur.fetchone()
            if not result:
                return

            user_id_alice = result[0]

            # Check if orders already exist
            cur.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
            count = cur.fetchone()[0]

            if count == 0:
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
            conn.commit()
    except Error as e:
        print(f"Error inserting sample data: {e}")
        conn.rollback()


def query_and_process(conn):
    try:
        with conn.cursor() as cur:
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
            for row in cur:
                print(f"{row[0]} ({row[1]}) ordered {row[2]} for {row[3]} zł on {row[4]}")
    except Error as e:
        print(f"Error querying data: {e}")


def main():
    try:
        conn = psycopg2.connect(
            host="localhost",
            port="5432",
            dbname="master_thesis",
            user="postgres",
            password="9"
        )

        setup_schema(conn)
        insert_sample_data(conn)
        query_and_process(conn)

    except Error as e:
        print(f"Database error: {e}")
    finally:
        if conn:
            conn.close()


if __name__ == "__main__":
    main()