import psycopg2
from psycopg2 import sql
from psycopg2.extras import execute_batch
from decimal import Decimal
from datetime import datetime


def connect_to_db():
    try:
        conn = psycopg2.connect(
            host="localhost",
            port=5432,
            dbname="master_thesis",
            user="postgres",
            password="9"
        )
        return conn
    except psycopg2.Error as e:
        print(f"Error connecting to database: {e}")
        raise


def setup_schema(conn):
    try:
        with conn.cursor() as cursor:
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                )
            """)

            cursor.execute("""
                CREATE TABLE IF NOT EXISTS orders (
                    id SERIAL PRIMARY KEY,
                    user_id INTEGER REFERENCES users(id),
                    product TEXT NOT NULL,
                    amount NUMERIC(10,2),
                    order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                )
            """)
        conn.commit()
    except psycopg2.Error as e:
        conn.rollback()
        print(f"Error setting up schema: {e}")
        raise


def insert_sample_data(conn):
    try:
        with conn.cursor() as cursor:
            # Insert users
            cursor.execute("""
                INSERT INTO users (name, email)
                VALUES 
                    (%s, %s),
                    (%s, %s),
                    (%s, %s)
                ON CONFLICT (email) DO NOTHING
            """, ('Alicja', 'alice@example.com',
                  'Bartek', 'bartek@example.com',
                  'Celina', 'celina@example.com'))

            # Get Alice's user ID
            cursor.execute("SELECT id FROM users WHERE email = %s", ('alice@example.com',))
            user_id_alice = cursor.fetchone()

            if not user_id_alice:
                return

            user_id_alice = user_id_alice[0]

            # Check if Alice has any orders
            cursor.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
            count_orders = cursor.fetchone()[0]

            if count_orders == 0:
                # Insert orders for Alice
                orders = [
                    (user_id_alice, "Laptop", Decimal("3200.00")),
                    (user_id_alice, "Mouse", Decimal("120.00")),
                    (user_id_alice, "Keyboard", Decimal("90.00"))
                ]

                execute_batch(cursor,
                              "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                              orders)

        conn.commit()
    except psycopg2.Error as e:
        conn.rollback()
        print(f"Error inserting sample data: {e}")
        raise


def query_and_process(conn):
    try:
        with conn.cursor() as cursor:
            query = """
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

            cursor.execute(query, (Decimal("100.00"),))

            print("Orders over 100 zł:\n")
            for record in cursor:
                name, email, product, amount, date = record
                print(f"{name} ({email}) ordered {product} for {amount} zł on {date}")

    except psycopg2.Error as e:
        print(f"Error querying data: {e}")
        raise


def main():
    conn = None
    try:
        conn = connect_to_db()
        conn.autocommit = False

        setup_schema(conn)
        insert_sample_data(conn)
        query_and_process(conn)

    except Exception as e:
        print(f"Error: {e}")
    finally:
        if conn:
            conn.close()


if __name__ == "__main__":
    main()