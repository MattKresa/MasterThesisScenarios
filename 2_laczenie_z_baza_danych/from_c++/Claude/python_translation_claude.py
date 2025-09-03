import psycopg2
from psycopg2 import sql
import sys


def setup_schema(cursor):
    """Create tables if they don't exist"""
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
    """Insert sample users and orders"""
    # Insert users
    cursor.execute("""
        INSERT INTO users (name, email)
        VALUES 
            ('Alicja', 'alice@example.com'),
            ('Bartek', 'bartek@example.com'),
            ('Celina', 'celina@example.com')
        ON CONFLICT (email) DO NOTHING;
    """)

    # Get Alice's ID
    cursor.execute("SELECT id FROM users WHERE email = %s", ('alice@example.com',))
    result = cursor.fetchone()
    if not result:
        return

    user_id_alice = result[0]

    # Check if orders already exist
    cursor.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
    count = cursor.fetchone()[0]

    if count == 0:
        # Insert sample orders
        orders = [
            (user_id_alice, "Laptop", 3200.00),
            (user_id_alice, "Mouse", 120.00),
            (user_id_alice, "Keyboard", 90.00)
        ]

        for order in orders:
            cursor.execute(
                "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                order
            )


def query_and_process(cursor):
    """Query and display orders over 100 zł"""
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
        user_name, email, product, amount, order_date = row
        print(f"{user_name} ({email}) ordered {product} for {amount} zł on {order_date}")


def main():
    try:
        # Connect to PostgreSQL database
        conn = psycopg2.connect(
            host="localhost",
            port=5432,
            database="master_thesis",
            user="postgres",
            password="9"
        )

        # Setup schema and insert data
        with conn:
            with conn.cursor() as cursor:
                setup_schema(cursor)
                insert_sample_data(cursor)

        # Query and process results in a new transaction
        with conn:
            with conn.cursor() as cursor:
                query_and_process(cursor)

    except psycopg2.Error as e:
        print(f"Database error: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    finally:
        if 'conn' in locals():
            conn.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())