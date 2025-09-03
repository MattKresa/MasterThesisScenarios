import psycopg2
from psycopg2 import Error
from decimal import Decimal
from datetime import datetime

class DatabaseManager:
    """
    Manages database connections, schema setup, data insertion, and querying
    for a PostgreSQL database.
    """

    @staticmethod
    def connect_to_db():
        """
        Establishes a connection to the PostgreSQL database.

        Returns:
            psycopg2.connection: A database connection object.

        Raises:
            psycopg2.Error: If a connection error occurs.
        """
        url = "postgresql://localhost:5432/master_thesis"
        user = "postgres"
        password = "9"
        try:
            # Connect to the PostgreSQL database
            conn = psycopg2.connect(f"dbname={url.split('/')[-1]} user={user} password={password} host=localhost port=5432")
            print("Database connection established successfully.")
            return conn
        except Error as e:
            print(f"Error connecting to PostgreSQL database: {e}")
            raise

    @staticmethod
    def setup_schema(conn):
        """
        Sets up the 'users' and 'orders' tables in the database if they do not exist.

        Args:
            conn (psycopg2.connection): The database connection object.

        Raises:
            psycopg2.Error: If a SQL execution error occurs.
        """
        try:
            with conn.cursor() as cur:
                # Create users table
                cur.execute("""
                    CREATE TABLE IF NOT EXISTS users (
                        id SERIAL PRIMARY KEY,
                        name TEXT NOT NULL,
                        email TEXT UNIQUE NOT NULL
                    )
                """)
                # Create orders table
                cur.execute("""
                    CREATE TABLE IF NOT EXISTS orders (
                        id SERIAL PRIMARY KEY,
                        user_id INTEGER REFERENCES users(id),
                        product TEXT NOT NULL,
                        amount NUMERIC(10,2),
                        order_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                    )
                """)
            conn.commit() # Commit the changes to the database
            print("Database schema setup complete.")
        except Error as e:
            print(f"Error setting up schema: {e}")
            conn.rollback() # Rollback in case of error
            raise

    @staticmethod
    def insert_sample_data(conn):
        """
        Inserts sample user and order data into the database.
        It checks for existing users and orders to prevent duplicate insertions.

        Args:
            conn (psycopg2.connection): The database connection object.

        Raises:
            psycopg2.Error: If a SQL execution error occurs.
        """
        try:
            with conn.cursor() as cur:
                # Insert users
                cur.execute("""
                    INSERT INTO users (name, email)
                    VALUES
                        ('Alicja', 'alice@example.com'),
                        ('Bartek', 'bartek@example.com'),
                        ('Celina', 'celina@example.com')
                    ON CONFLICT (email) DO NOTHING
                """)
                conn.commit()
                print("Sample users inserted (or already exist).")

                # Get userId for 'alice@example.com'
                user_id_alice = -1
                cur.execute("SELECT id FROM users WHERE email = %s", ('alice@example.com',))
                result = cur.fetchone()
                if result:
                    user_id_alice = result[0]
                else:
                    print("Alice not found in users table, skipping order insertion.")
                    return

                # Check if orders for Alice already exist
                cur.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
                count_orders = cur.fetchone()[0]

                if count_orders == 0:
                    # Insert orders for Alice
                    orders_to_insert = [
                        (user_id_alice, "Laptop", Decimal("3200.00")),
                        (user_id_alice, "Mouse", Decimal("120.00")),
                        (user_id_alice, "Keyboard", Decimal("90.00"))
                    ]
                    cur.executemany(
                        "INSERT INTO orders (user_id, product, amount) VALUES (%s, %s, %s)",
                        orders_to_insert
                    )
                    conn.commit()
                    print("Sample orders for Alice inserted.")
                else:
                    print("Orders for Alice already exist, skipping insertion.")
        except Error as e:
            print(f"Error inserting sample data: {e}")
            conn.rollback()
            raise

    @staticmethod
    def query_and_process(conn):
        """
        Queries the database for orders with an amount greater than a specified value
        and prints the results.

        Args:
            conn (psycopg2.connection): The database connection object.

        Raises:
            psycopg2.Error: If a SQL execution error occurs.
        """
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
        try:
            with conn.cursor() as cur:
                # Execute the query with a parameterized amount
                cur.execute(sql, (Decimal("100.00"),))

                print("\nOrders over 100 zł:\n")
                for row in cur.fetchall():
                    user_name, email, product, amount, order_date = row
                    # Format the output similar to Java's printf
                    print(f"{user_name} ({email}) ordered {product} for {amount} zł on {order_date.strftime('%Y-%m-%d %H:%M:%S')}")
        except Error as e:
            print(f"Error querying and processing data: {e}")
            raise

if __name__ == "__main__":
    # Main execution block
    conn = None # Initialize conn to None
    try:
        # Connect to the database
        conn = DatabaseManager.connect_to_db()
        # Ensure autocommit is true, though psycopg2 typically commits on connection close
        # or explicit commit for DDL/DML. For DDL, it's often auto-committed.
        # For DML, we use conn.commit() explicitly after operations.

        # Setup database schema (create tables)
        DatabaseManager.setup_schema(conn)
        # Insert sample data
        DatabaseManager.insert_sample_data(conn)
        # Query and process data
        DatabaseManager.query_and_process(conn)

    except Error as e:
        # Catch and print any database-related errors
        print(f"An application error occurred: {e}")
    except Exception as e:
        # Catch any other unexpected errors
        print(f"An unexpected error occurred: {e}")
    finally:
        # Ensure the database connection is closed
        if conn:
            conn.close()
            print("Database connection closed.")

