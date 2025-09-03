import psycopg2
from psycopg2 import Error

def setup_schema(cur):
    """
    Sets up the database schema by creating 'users' and 'orders' tables
    if they do not already exist.

    Args:
        cur: A psycopg2 cursor object.
    """
    print("Setting up database schema...")
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
    print("Schema setup complete.")

def insert_sample_data(cur):
    """
    Inserts sample user and order data into the database.
    It checks for existing users/orders to prevent duplicate entries.

    Args:
        cur: A psycopg2 cursor object.
    """
    print("Inserting sample data...")
    # Insert users, handling conflicts on email
    cur.execute("""
        INSERT INTO users (name, email)
        VALUES
            ('Alicja', 'alice@example.com'),
            ('Bartek', 'bartek@example.com'),
            ('Celina', 'celina@example.com')
        ON CONFLICT (email) DO NOTHING;
    """)

    # Get Alicja's user ID
    cur.execute("SELECT id FROM users WHERE email = 'alice@example.com'")
    result = cur.fetchone()
    if result is None:
        print("Error: Alicja's user ID not found after insertion.")
        return

    user_id_alice = result[0]
    print(f"Alicja's user ID: {user_id_alice}")

    # Check if Alicja already has orders
    cur.execute("SELECT COUNT(*) FROM orders WHERE user_id = %s", (user_id_alice,))
    order_count = cur.fetchone()[0]

    if order_count == 0:
        print("Inserting orders for Alicja...")
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
        print("Orders inserted for Alicja.")
    else:
        print(f"Alicja already has {order_count} orders. Skipping order insertion.")
    print("Sample data insertion complete.")

def query_and_process(cur):
    """
    Queries the database for orders with an amount greater than a specified value
    and prints the results.

    Args:
        cur: A psycopg2 cursor object.
    """
    print("\nQuerying and processing data...")
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
    """, (100.0,)) # Use %s for parameter placeholders in psycopg2

    records = cur.fetchall()

    print("Orders over 100 zł:\n")
    if not records:
        print("No orders found matching the criteria.")
    for row in records:
        # Access columns by index or by name if using DictCursor
        user_name, email, product, amount, order_date = row
        print(f"{user_name} ({email}) ordered {product} for {amount:.2f} zł on {order_date}")
    print("\nQuery and processing complete.")

def main():
    """
    Main function to establish database connection, perform operations,
    and handle exceptions.
    """
    conn = None
    try:
        # Establish connection to PostgreSQL
        # Replace 'dbname', 'user', 'password', 'host', 'port' with your actual credentials
        conn = psycopg2.connect(
            dbname="master_thesis",
            user="postgres",
            password="9",
            host="localhost",
            port="5432"
        )
        conn.autocommit = False # Manage transactions manually

        # First transaction for schema setup and data insertion
        with conn.cursor() as cur:
            setup_schema(cur)
            insert_sample_data(cur)
        conn.commit() # Commit the changes for schema and data insertion
        print("First transaction committed (schema and data).")

        # Second transaction (or just a new cursor) for querying
        # For read-only operations, a new transaction isn't strictly necessary
        # but it mimics the C++ example's `tx2(conn)`
        with conn.cursor() as cur:
            query_and_process(cur)
        conn.commit() # Commit the read transaction (no changes, but good practice)
        print("Second transaction committed (query).")

    except (Exception, Error) as e:
        print(f"Error: {e}")
        if conn:
            conn.rollback() # Rollback any changes on error
            print("Transaction rolled back due to error.")
    finally:
        if conn:
            conn.close() # Close the connection
            print("Database connection closed.")

if __name__ == "__main__":
    main()
