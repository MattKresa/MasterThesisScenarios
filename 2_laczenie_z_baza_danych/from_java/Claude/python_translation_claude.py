import psycopg2
from psycopg2.extras import RealDictCursor
from decimal import Decimal
import sys


class DatabaseManager:

    @staticmethod
    def connect_to_db():
        """Connect to PostgreSQL database"""
        try:
            conn = psycopg2.connect(
                host="localhost",
                port=5432,
                database="master_thesis",
                user="postgres",
                password="9"
            )
            return conn
        except psycopg2.Error as e:
            raise Exception(f"Database connection failed: {e}")

    @staticmethod
    def setup_schema(conn):
        """Create tables if they don't exist"""
        with conn.cursor() as cursor:
            # Create users table
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS users (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    email TEXT UNIQUE NOT NULL
                )
            """)

            # Create orders table
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

    @staticmethod
    def insert_sample_data(conn):
        """Insert sample users and orders"""
        with conn.cursor() as cursor:
            # Insert users
            cursor.execute("""
                INSERT INTO users (name, email)
                VALUES 
                    ('Alicja', 'alice@example.com'),
                    ('Bartek', 'bartek@example.com'),
                    ('Celina', 'celina@example.com')
                ON CONFLICT (email) DO NOTHING
            """)

            # Get Alice's user ID
            cursor.execute("SELECT id FROM users WHERE email = %s", ('alice@example.com',))
            result = cursor.fetchone()

            if result is None:
                return

            user_id_alice = result[0]

            # Check if Alice already has orders
            cursor.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
            count_orders = cursor.fetchone()[0]

            if count_orders == 0:
                # Insert sample orders for Alice
                orders_data = [
                    (user_id_alice, "Laptop", Decimal("3200.00")),
                    (user_id_alice, "Mouse", Decimal("120.00")),
                    (user_id_alice, "Keyboard", Decimal("90.00"))
                ]

                cursor.executemany(
                    "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                    orders_data
                )

            conn.commit()

    @staticmethod
    def query_and_process(conn):
        """Query and display orders over 100 zł"""
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

        with conn.cursor(cursor_factory=RealDictCursor) as cursor:
            cursor.execute(sql, (Decimal("100.00"),))
            results = cursor.fetchall()

            print("Orders over 100 zl:\n")
            for row in results:
                print(f"{row['user_name']} ({row['email']}) ordered {row['product']} "
                      f"for {row['amount']} zł on {row['order_date']}")


def main():
    try:
        with DatabaseManager.connect_to_db() as conn:
            conn.autocommit = True

            DatabaseManager.setup_schema(conn)
            DatabaseManager.insert_sample_data(conn)
            DatabaseManager.query_and_process(conn)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()